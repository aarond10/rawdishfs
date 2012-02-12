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
#ifndef _RPC_RPC_H_
#define _RPC_RPC_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <tr1/functional>
#include <tr1/memory>

#include <msgpack.hpp>
#include <glog/logging.h>

#include <epoll_threadpool/future.h>
#include <epoll_threadpool/iobuffer.h>
#include <epoll_threadpool/tcp.h>

namespace rpc {

using epoll_threadpool::Future;
using epoll_threadpool::IOBuffer;
using epoll_threadpool::TcpListenSocket;
using epoll_threadpool::TcpSocket;

using std::map;
using std::set;
using std::string;
using std::tr1::bind;
using std::tr1::enable_shared_from_this;
using std::tr1::function;
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::vector;

using namespace std::tr1;
using namespace std::tr1::placeholders;

/**
 * Various helper functions used to serialize and deserialize arguments and
 * return values to msgpack format.
 */
namespace {

/**
 * Helper callback for Future that converts from type A to type 
 * msgpack::object.
 */
template<class A>
void serializeFuture(Future<A> src, Future<IOBuffer*> dst) {
  IOBuffer *buf = new IOBuffer();
  msgpack::pack(buf, src.get());
  dst.set(buf);
}

/**
 * Helper callback for Future that converts from type IOBuffer* to type A.
 */
template<class A>
void deserializeFuture(Future<A> dst, Future<IOBuffer*> src) {
  if (src.get()) {
    msgpack::unpacked msg;
    msgpack::unpack(&msg, src.get()->pulldown(src.get()->size()), src.get()->size());
    msgpack::object obj = msg.get();
    A a;
    obj.convert(&a);
    dst.set(a);
    delete src.get();
  } else {
    // We assume we can create an empty version of all return types signifying failure.
    dst.set(A());
  }
}

/**
 * Helper functions that converts 0-5 arguments into a valid msgpack object.
 */
IOBuffer* serializeArgs(uint64_t id, const string& name) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<>());
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}
template<class A0> 
IOBuffer* serializeArgs(uint64_t id, const string& name, A0 a0) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<A0>(a0));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}
template<class A0, class A1> 
IOBuffer* serializeArgs(uint64_t id, const string& name, A0 a0, A1 a1) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<A0, A1>(a0, a1));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}
template<class A0, class A1, class A2> 
IOBuffer* serializeArgs(uint64_t id, const string& name, A0 a0, A1 a1, A2 a2) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<A0, A1, A2>(a0, a1, a2));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}
template<class A0, class A1, class A2, class A3> 
IOBuffer* serializeArgs(uint64_t id, const string& name, A0 a0, A1 a1, A2 a2, A3 a3) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<A0, A1, A2, A3>(a0, a1, a2, a3));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}
template<class A0, class A1, class A2, class A3, class A4> 
IOBuffer* serializeArgs(uint64_t id, const string& name, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<A0, A1, A2, A3, A4>(a0, a1, a2, a3, a4));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}
template<class A0, class A1, class A2, class A3, class A4, class A5> 
IOBuffer* serializeArgs(uint64_t id, const string &name, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { 
  IOBuffer* buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<A0, A1, A2, A3, A4, A5>(a0, a1, a2, a3, a4, a5));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      id, name, msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  return buf;
}

/**
 * Helper functions that converts a msgpack object back into 0-5 arguments.
 */
template<class A>
Future<IOBuffer*> deserializeArgs(function<Future<A>()> func, IOBuffer* args) {
  Future<A> src = func();
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}
template<class A, class A0>
Future<IOBuffer*> deserializeArgs(function<Future<A>(A0)> func, IOBuffer* args) {
  msgpack::unpacked msg;
  msgpack::unpack(&msg, args->pulldown(args->size()), args->size());
  msgpack::type::tuple<A0> tup;
  msg.get().convert(&tup);
  Future<A> src = func(tup.template get<0>());
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}
template<class A, class A0, class A1>
Future<IOBuffer*> deserializeArgs(function<Future<A>(A0, A1)> func, IOBuffer* args) {
  msgpack::unpacked msg;
  msgpack::unpack(&msg, args->pulldown(args->size()), args->size());
  msgpack::type::tuple<A0, A1> tup;
  msg.get().convert(&tup);
  Future<A> src = func(tup.template get<0>(), tup.template get<1>());
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}
template<class A, class A0, class A1, class A2>
Future<IOBuffer*> deserializeArgs(function<Future<A>(A0, A1, A2)> func, IOBuffer* args) {
  msgpack::unpacked msg;
  msgpack::unpack(&msg, args->pulldown(args->size()), args->size());
  msgpack::type::tuple<A0, A1, A2> tup;
  msg.get().convert(&tup);
  Future<A> src = func(tup.template get<0>(),
                       tup.template get<1>(), 
                       tup.template get<2>());
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}
template<class A, class A0, class A1, class A2, class A3>
Future<IOBuffer*> deserializeArgs(function<Future<A>(A0, A1, A2, A3)> func, IOBuffer* args) {
  msgpack::unpacked msg;
  msgpack::unpack(&msg, args->pulldown(args->size()), args->size());
  msgpack::type::tuple<A0, A1, A2, A3> tup;
  msg.get().convert(&tup);
  Future<A> src = func(tup.template get<0>(),
                       tup.template get<1>(), 
                       tup.template get<2>(), 
                       tup.template get<3>());
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}
template<class A, class A0, class A1, class A2, class A3, class A4>
Future<IOBuffer*> deserializeArgs(function<Future<A>(A0, A1, A2, A3, A4)> func, IOBuffer* args) {
  msgpack::unpacked msg;
  msgpack::unpack(&msg, args->pulldown(args->size()), args->size());
  msgpack::type::tuple<A0, A1, A2, A3, A4> tup;
  msg.get().convert(&tup);
  Future<A> src = func(tup.template get<0>(),
                       tup.template get<1>(), 
                       tup.template get<2>(), 
                       tup.template get<3>(), 
                       tup.template get<4>());
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}
template<class A, class A0, class A1, class A2, class A3, class A4, class A5>
Future<IOBuffer*> deserializeArgs(function<Future<A>(A0, A1, A2, A3, A4, A5)> func, IOBuffer* args) {
  msgpack::unpacked msg;
  msgpack::unpack(&msg, args->pulldown(args->size()), args->size());
  msgpack::type::tuple<A0, A1, A2, A3, A4, A5> tup;
  msg.get().convert(&tup);
  Future<A> src = func(tup.template get<0>(),
                       tup.template get<1>(), 
                       tup.template get<2>(), 
                       tup.template get<3>(), 
                       tup.template get<4>(), 
                       tup.template get<5>());
  Future<IOBuffer*> dst;
  src.addCallback(bind(&serializeFuture<A>, src, dst));
  delete args;
  return dst;
}

} // end anonymous namespace 

/**
 * Runs an RPC server on a given TcpListenSocket
 * This class takes ownership of the provided TcpListenSocket.
 * RPC functions should be exposed by calling registerFunction().
 * Both immediate and asynchronous callback functions are supported.
 * Inbound connections formed are not owned by this class. An accept
 * callback can be used to keep track of these connections. If
 * not provided, the default behaviour is to store a reference to
 * all connections in the RPCServer itself, destroying them when
 * the server is shut down.
 */
class RPCServer {
 private:
  typedef function<Future<IOBuffer*>(IOBuffer*)> RPCFunc;

 public:
  /**
   * Represents a single connection to a single client endpoint.
   * The only thing a user can do with a connection is start accepting
   * traffic and eventually disconnect it. It is exposed via 
   * RPCServer::setOnAcceptCallback to allow users to do their own
   * client management.
   */
  class Connection {
   public:
    virtual ~Connection();

    void start();
    void disconnect();
    void setDisconnectCallback(function<void()> f);

   private:
    friend class RPCServer;
    Connection(shared_ptr< map<string, RPCFunc> > funcs, 
               shared_ptr<TcpSocket> s);

    class Internal : public enable_shared_from_this<Internal> {
     public:
      Internal(shared_ptr< map<string, RPCFunc> > funcs, 
               shared_ptr<TcpSocket> s);
      virtual ~Internal();

      void start();
      void disconnect();
      void setDisconnectCallback(function<void()> f);

      void responseCallback(uint64_t id, Future<IOBuffer*> obj);
      void deferredRPCCall(uint64_t id, 
          function<Future<IOBuffer*>(IOBuffer*)> func,
          IOBuffer* args);
      void onReceive(IOBuffer *buf);
      void onDisconnect();

      static void cleanup(shared_ptr<Internal> ptr) {
        ptr.reset();
      }

      shared_ptr<TcpSocket> _socket;
      msgpack::unpacker _pac;
      shared_ptr< map< string, RPCFunc > > _funcs;
      function<void()> _disconnectCallback;
    };
    shared_ptr<Internal> _internal;
  };

  /**
   * Consumes a TcpListenSocket, using it to run
   * an RPC service.
   */
  static shared_ptr<RPCServer> create(shared_ptr<TcpListenSocket> s) {
    return shared_ptr<RPCServer>(new RPCServer(s));
  }
  virtual ~RPCServer();

  /**
   * Start the RPC server accepting connections.
   */
  void start();

  /**
   * Registers a callback to notify when new connections arrive.
   * If this is used, it is the callbacks responsibility to keep a
   * reference to connections. Dereferenced connections will be dropped.
   */
  void setAcceptCallback(function<void(shared_ptr<Connection>)> f);

  /**
   * Registers RPC functions. Functions *must* return results via a Future.
   */
  template<class A>
  void registerFunction(const string name, function<Future<A>()> f);
  template<class A, class B>
  void registerFunction(const string name, function<Future<A>(B)> f);
  template<class A, class B, class C>
  void registerFunction(const string name, function<Future<A>(B,C)> f);
  template<class A, class B, class C, class D>
  void registerFunction(const string name, function<Future<A>(B,C,D)> f);
  template<class A, class B, class C, class D, class E>
  void registerFunction(const string name, function<Future<A>(B,C,D,E)> f);
  template<class A, class B, class C, class D, class E, class F>
  void registerFunction(const string name, function<Future<A>(B,C,D,E,F)> f);

 protected:
  RPCServer(shared_ptr<TcpListenSocket> s);

 private:
  void onAccept(shared_ptr<TcpSocket> s);

  shared_ptr<TcpListenSocket> _socket;
  shared_ptr< map< string, RPCFunc > > _funcs;
  set< shared_ptr<Connection> > _connections;
  function<void(shared_ptr<Connection> conn)> _acceptCallback;
};

template<class A>
void RPCServer::registerFunction(
    const string name, function<Future<A>()> f) {
  (*_funcs)[name] = bind(&deserializeArgs<A>, f, _1);
}
template<class A, class B>
void RPCServer::registerFunction(const string name, 
    function<Future<A>(B)> f) {
  (*_funcs)[name] = bind(&deserializeArgs<A, B>, f, _1);
}
template<class A, class B, class C>
void RPCServer::registerFunction(
    const string name, function<Future<A>(B, C)> f) {
  (*_funcs)[name] = bind(&deserializeArgs<A, B, C>, f, _1);
}
template<class A, class B, class C, class D>
void RPCServer::registerFunction(
    const string name, function<Future<A>(B, C, D)> f) {
  (*_funcs)[name] = bind(&deserializeArgs<A, B, C, D>, f, _1);
}
template<class A, class B, class C, class D, class E>
void RPCServer::registerFunction(
    const string name, function<Future<A>(B, C, D, E)> f) {
  (*_funcs)[name] = bind(&deserializeArgs<A, B, C, D, E>, f, _1);
}
template<class A, class B, class C, class D, class E, class F>
void RPCServer::registerFunction(
    const string name, function<Future<A>(B, C, D, E, F)> f) {
  (*_funcs)[name] = bind(&deserializeArgs<A, B, C, D, E, F>, f, _1);
}

/**
 * Connects to a given RPC server and maintains the connection.
 * Will optionally notify on disconnect and provide a blocking
 * RPC call endpoint.
 */
class RPCClient {
 public:
  RPCClient(shared_ptr<TcpSocket> s);
  virtual ~RPCClient();

  /**
   * This function must be called to initiate event processing on this client.
   * This gives the user a chance to set up callbacks, etc.
   */
  void start();

  /**
   * Disconnects the socket. No further calls will be possible after calling
   * this function.
   */
  void disconnect();

  template<class A>
  Future<A> call(const string name);
  template<class A, class B>
  Future<A> call(const string name, B a1);
  template<class A, class B, class C>
  Future<A> call(const string name, B a1, C a2);
  template<class A, class B, class C, class D>
  Future<A> call(const string name, B a1, C a2, D a3);
  template<class A, class B, class C, class D, class E>
  Future<A> call(const string name, B a1, C a2, D a3, E a4);
  template<class A, class B, class C, class D, class E, class F>
  Future<A> call(const string name, B a1, C a2, D a3, E a4, F a5);

  void setDisconnectCallback(function<void()> callback);

 private:
  class Internal : public enable_shared_from_this<Internal> {
   public:
    Internal(shared_ptr<TcpSocket> s);
    virtual ~Internal();

    void start();
    void disconnect();
    void setDisconnectCallback(function<void()> callback);
    void onReceive(IOBuffer *buf);
    void onDisconnect();

   //private:
    function<void()> _disconnectCallback;
    shared_ptr<TcpSocket> _socket;
    msgpack::unpacker _pac;
    uint64_t _reqId;
    map< uint64_t, function<void(IOBuffer*)> > _respCallbacks;
  };
  shared_ptr<Internal> _internal;
};

template<class A>
Future<A> RPCClient::call(const string name) {
  Future<A> ret;
  _internal->_respCallbacks[_internal->_reqId] =
      bind(&deserializeFuture<A>, ret, placeholders::_1);
  _internal->_socket->write(serializeArgs(
      _internal->_reqId++, name));
  return ret;
}
template<class A, class A0>
Future<A> RPCClient::call(const string name, A0 a0) {
  Future<A> ret;
  _internal->_respCallbacks[_internal->_reqId] =
      bind(&deserializeFuture<A>, ret, placeholders::_1);
  _internal->_socket->write(serializeArgs<A0>(
      _internal->_reqId++, name, a0));
  return ret;
}
template<class A, class A0, class A1>
Future<A> RPCClient::call(const string name, A0 a0, A1 a1) {
  Future<A> ret;
  _internal->_respCallbacks[_internal->_reqId] =
      bind(&deserializeFuture<A>, ret, placeholders::_1);
  _internal->_socket->write(serializeArgs<A0, A1>(
      _internal->_reqId++, name, a0, a1));
  return ret;
}
template<class A, class A0, class A1, class A2>
Future<A> RPCClient::call(const string name, A0 a0, A1 a1, A2 a2) {
  Future<A> ret;
  _internal->_respCallbacks[_internal->_reqId] =
      bind(&deserializeFuture<A>, ret, placeholders::_1);
  _internal->_socket->write(serializeArgs<A0, A1, A2>(
      _internal->_reqId++, name, a0, a1, a2));
  return ret;
}
template<class A, class A0, class A1, class A2, class A3>
Future<A> RPCClient::call(const string name, A0 a0, A1 a1, A2 a2, A3 a3) {
  Future<A> ret;
  _internal->_respCallbacks[_internal->_reqId] =
      bind(&deserializeFuture<A>, ret, placeholders::_1);
  _internal->_socket->write(serializeArgs<A0, A1, A2>(
      _internal->_reqId++, name, a0, a1, a2, a3));
  return ret;
}
template<class A, class A0, class A1, class A2, class A3, class A4>
Future<A> RPCClient::call(const string name, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
  Future<A> ret;
  _internal->_respCallbacks[_internal->_reqId] =
      bind(&deserializeFuture<A>, ret, placeholders::_1);
  _internal->_socket->write(serializeArgs<A0, A1, A2, A3, A4>(
      _internal->_reqId++, name, a0, a1, a2, a3, a4));
  return ret;
}
}  // end rpc namespace
#endif
