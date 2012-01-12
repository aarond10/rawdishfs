/*
 Copyright (c) 2011 Aaron Drew
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. Neither the name of the copyright holders nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "rpc.h"

#include <epoll_threadpool/iobuffer.h>
#include <epoll_threadpool/notification.h>
#include <epoll_threadpool/tcp.h>

#include <glog/logging.h>
#include <msgpack.hpp>

namespace rpc {

RPCServer::RPCServer(shared_ptr<TcpListenSocket> s) 
    : _funcs(new map<string, RPCFunc>()), _socket(s) {
}

RPCServer::~RPCServer() {
}

void RPCServer::start() {
  _socket->setAcceptCallback(std::tr1::bind(
      &RPCServer::onAccept, this, std::tr1::placeholders::_1));
}

void RPCServer::setAcceptCallback(
    std::tr1::function<void(shared_ptr<Connection>)> cb) {
  _acceptCallback = cb;
  _connections.clear();
}

void RPCServer::onAccept(shared_ptr<TcpSocket> s) {
  shared_ptr<Connection> r(new Connection(_funcs, s));
  if (_acceptCallback) {
    _acceptCallback(r);
  } else {
    _connections.insert(r);
    r->start();
  }
}

RPCServer::Connection::Connection(
    shared_ptr< map<string, RPCFunc> > funcs, shared_ptr<TcpSocket> s) 
        : _internal(new Internal(funcs, s)) {
}

RPCServer::Connection::~Connection() {
  _internal->disconnect();
}

void RPCServer::Connection::start() {
  _internal->start();
}

void RPCServer::Connection::disconnect() {
  _internal->disconnect();
}

void RPCServer::Connection::setDisconnectCallback(
    std::tr1::function<void()> cb) {
  _internal->setDisconnectCallback(cb);
}

RPCServer::Connection::Internal::Internal(
    shared_ptr< map< string, RPCFunc > > funcs, shared_ptr<TcpSocket> s)
        : _funcs(funcs), _socket(s) {
}

RPCServer::Connection::Internal::~Internal() {
}

void RPCServer::Connection::Internal::start() {
  _socket->setReceiveCallback(std::tr1::bind(
      &RPCServer::Connection::Internal::onReceive, 
          shared_from_this(), std::tr1::placeholders::_1));
  _socket->setDisconnectCallback(std::tr1::bind(
      &RPCServer::Connection::Internal::onDisconnect, shared_from_this()));
  _socket->start();
}

void RPCServer::Connection::Internal::disconnect() {
  _socket->setReceiveCallback(NULL);
  _socket->setDisconnectCallback(NULL);
  _socket->disconnect();
}

void RPCServer::Connection::Internal::responseCallback(
    uint64_t id, Future<IOBuffer*> obj) {
  msgpack::type::tuple<uint64_t, msgpack::type::raw_ref> ret(id, 
      msgpack::type::raw_ref(
          obj.get()->pulldown(obj.get()->size()),
          obj.get()->size()));
  IOBuffer *buf = new IOBuffer();
  msgpack::pack(*buf, ret);
  delete obj.get();
  _socket->write(buf);
}

void RPCServer::Connection::Internal::deferredRPCCall(
    uint64_t id,
    function<Future<IOBuffer*>(IOBuffer*)> func,
    IOBuffer* args) {
  Future<IOBuffer*> ret = func(args);
  ret.addCallback(bind(&Connection::Internal::responseCallback, this, id, ret));
}

void RPCServer::Connection::Internal::onReceive(IOBuffer *buf) {
  // Note that this will never be called simultaneously from two threads so
  // we're safe to process buffers here. 
  const int buf_size = buf->size();
  const char *data = buf->pulldown(buf_size);
  if (data) {
    _pac.reserve_buffer(buf_size);
    if (_pac.buffer_capacity() < buf_size) {
      LOG(ERROR) << "buf->size(): " << buf_size
                 << ", _pac.buffer_capacity(): " << _pac.buffer_capacity();
      return;
    }
    memcpy(_pac.buffer(), data, buf_size);
    _pac.buffer_consumed(buf_size);
    buf->consume(buf_size);
  }
  msgpack::unpacked result;
  while (_pac.next(&result)) {
    msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req;
    result.get().convert(&req);
    uint64_t id = req.get<0>();
    string name = req.get<1>();
    if (_funcs->find(name) != _funcs->end()) {
      // Its bad mojo to do processing from the onReceive handler since its
      // blocking further reads so we enqueue the function on a worker
      // thread. Unfortuntely, msgpack hasn't been written with decent support
      // for deep copying of objects so we pass a buffer around here that needs
      // to be unpacked in the other thread.
      _socket->getEventManager()->enqueue(bind(
          &Connection::Internal::deferredRPCCall, 
          this,
          id, 
          _funcs->at(name), 
          new IOBuffer(req.get<2>().ptr, req.get<2>().size)));
    } else {
      LOG(ERROR) << "Unknown RPC method: " << name << ". Disconnecting.";
      _socket->getEventManager()->enqueue(
          std::tr1::bind(&Connection::Internal::disconnect, this));
    }
  }
}

void RPCServer::Connection::Internal::setDisconnectCallback(
    std::tr1::function<void()> cb) {
  _disconnectCallback = cb;
}

void RPCServer::Connection::Internal::onDisconnect() {
  if (_disconnectCallback) {
    _disconnectCallback();
  }
}

RPCClient::RPCClient(shared_ptr<TcpSocket> s)
    : _internal(new Internal(s)) {
}

RPCClient::~RPCClient() {
  _internal->disconnect();
}

void RPCClient::start() {
  _internal->start();
}

void RPCClient::disconnect() {
  _internal->disconnect();
}

void RPCClient::setDisconnectCallback(std::tr1::function<void()> callback) {
  _internal->setDisconnectCallback(callback);
}

RPCClient::Internal::Internal(shared_ptr<TcpSocket> s)
    : _socket(s), _reqId(0) {
}

RPCClient::Internal::~Internal() {
}

void RPCClient::Internal::start() {
  _socket->setReceiveCallback(
      std::tr1::bind(&RPCClient::Internal::onReceive, 
          shared_from_this(), std::tr1::placeholders::_1));
  _socket->setDisconnectCallback(
      std::tr1::bind(&RPCClient::Internal::onDisconnect, 
          shared_from_this()));
  _socket->start();
}

void RPCClient::Internal::disconnect() {
  _socket->setReceiveCallback(NULL);
  _socket->setDisconnectCallback(NULL);
  _socket->disconnect();
  for(map< uint64_t, function<void(IOBuffer*)> >::iterator i = 
      _respCallbacks.begin(); i != _respCallbacks.end(); i++) {
    LOG(WARNING) << "Pending callbacks for RPCClient will be aborted.";
    // TODO: Notify aborted RPC calls?.
  }
  _respCallbacks.clear();
}

void RPCClient::Internal::setDisconnectCallback(std::tr1::function<void()> callback) {
  _disconnectCallback = callback;
}

void RPCClient::Internal::onReceive(IOBuffer *buf) {
  const int buf_size = buf->size();
  const char *data = buf->pulldown(buf_size);
  if (data) {
    _pac.reserve_buffer(buf_size);
    if (_pac.buffer_capacity() < buf_size) {
      LOG(ERROR) << "buf->size(): " << buf_size 
                 << ", _pac.buffer_capacity(): " 
                 << _pac.buffer_capacity();
      return;
    }
    memcpy(_pac.buffer(), data, buf_size);
    _pac.buffer_consumed(buf_size);
    buf->consume(buf_size);
  }
  msgpack::unpacked result;
  while (_pac.next(&result)) {
    msgpack::type::tuple<uint64_t, msgpack::type::raw_ref> req;
    result.get().convert(&req);
    uint64_t id = req.get<0>();
    if (_respCallbacks.find(id) != _respCallbacks.end()) {
      // Its bad mojo to do processing from the onReceive handler since its
      // blocking further reads so we enqueue the function on a worker
      // thread. Unfortuntely, msgpack hasn't been written with decent support
      // for deep copying of objects so we pass a buffer around here that needs
      // to be unpacked in the other thread.
      _socket->getEventManager()->enqueue(std::tr1::bind(
          _respCallbacks[id], 
          new IOBuffer(req.get<1>().ptr, req.get<1>().size)));
      _respCallbacks.erase(id);
    } else {
      LOG(ERROR) << "Unknown RPC response for ID: " << id;
    }
  }
}

void RPCClient::Internal::onDisconnect() {
  if (_disconnectCallback) {
    _disconnectCallback();
  }
}

}

