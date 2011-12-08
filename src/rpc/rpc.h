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

#include <epoll_threadpool/iobuffer.h>
#include <epoll_threadpool/tcp.h>

namespace rpc {

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
 public:
  typedef function<
      void(IOBuffer*, function<void(const msgpack::object&)>)> RPCFunc;

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

      void responseCallback(uint64_t id, const msgpack::object& obj);
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
   * Registers an RPC function that returns its result immediately.
   */
  template<class A>
  void registerFunction(const char* name, function<A()> f);
  template<class A, class B>
  void registerFunction(const char* name, function<A(B)> f);
  template<class A, class B, class C>
  void registerFunction(const char* name, function<A(B,C)> f);
  template<class A, class B, class C, class D>
  void registerFunction(const char* name, function<A(B,C,D)> f);
  template<class A, class B, class C, class D, class E>
  void registerFunction(const char* name, function<A(B,C,D,E)> f);
  template<class A, class B, class C, class D, class E, class F>
  void registerFunction(const char* name, function<A(B,C,D,E,F)> f);

  /**
   * Registers a function that returns its result asynchronously via a 
   * callback. The callback function provided must be called exactly once.
   */
  template<class A>
  void registerFunctionAsync(const char* name, function<void(function<void(A)>)> f);
  template<class A, class B>
  void registerFunctionAsync(const char* name, function<void(B, function<void(A)>)> f);
  template<class A, class B, class C>
  void registerFunctionAsync(const char* name, function<void(B, C, function<void(A)>)> f);
  template<class A, class B, class C, class D>
  void registerFunctionAsync(const char* name, function<void(B, C, D, function<void(A)>)> f);
  template<class A, class B, class C, class D, class E>
  void registerFunctionAsync(const char* name, function<void(B, C, D, E, function<void(A)>)> f);
  template<class A, class B, class C, class D, class E, class F>
  void registerFunctionAsync(const char* name, function<void(B, C, D, E, F, function<void(A)>)> f);

 private:
  RPCServer(shared_ptr<TcpListenSocket> s);
  void onAccept(shared_ptr<TcpSocket> s);

  shared_ptr<TcpListenSocket> _socket;
  shared_ptr< map< string, RPCFunc > > _funcs;
  set< shared_ptr<Connection> > _connections;
  function<void(shared_ptr<Connection> conn)> _acceptCallback;
};

namespace {
  // Helper functions used to convert A(...) functions into void(..., function<void(A)>).
  template<class A> 
  void immediateToCallback(function<A()> func, function<void(A)> cb) { 
    cb(func()); 
  }
  template<class A, class B> 
  void immediateToCallback(function<A(B)> func, B b, function<void(A)> cb) { 
    cb(func(b)); 
  }
  template<class A, class B, class C> 
  void immediateToCallback(function<A(B,C)> func, B b, C c, function<void(A)> cb) { 
    cb(func(b, c)); 
  }
  template<class A, class B, class C, class D> 
  void immediateToCallback(function<A(B, C, D)> func, B b, C c, D d, function<void(A)> cb) { 
    cb(func(b, c, d)); 
  }
  template<class A, class B, class C, class D, class E> 
  void immediateToCallback(function<A(B, C, D, E)> func, B b, C c, D d, E e, function<void(A)> cb) { 
    cb(func(b, c, d, e)); 
  }
  template<class A, class B, class C, class D, class E, class F> 
  void immediateToCallback(function<A(B, C, D, E, F)> func, B b, C c, D d, E e, F f, function<void(A)> cb) { 
    cb(func(b, c, d, e, f)); 
  }

  // Helper function used to convert a void(A) callback into a void(const msgpack::object&) callback.
  template<class A> void msgpackRet(function<void(const msgpack::object&)> func, A ret) {
    shared_ptr<msgpack::zone> z(new msgpack::zone());
    msgpack::object::with_zone obj(z.get());
    obj << ret;
    //msgpack::object obj(ret);
    func(obj);
  }

  // Helper functions used to deserialize function arguments and execute functions.
  template<class A> static void msgpackArgs(function<void(function<void(A)>)> f, 
      IOBuffer* argsBuf, function<void(const msgpack::object&)> cb) { 
    DCHECK_EQ(0, argsBuf->size());
    f(bind(&msgpackRet<A>, cb, placeholders::_1)); 
    delete argsBuf;
  }
  template<class A, class B> static void msgpackArgs(function<void(B, function<void(A)>)> f, 
      IOBuffer* argsBuf, function<void(const msgpack::object&)> cb) { 
    msgpack::unpacked msg;
    msgpack::unpack(&msg, argsBuf->pulldown(argsBuf->size()), argsBuf->size());
    msgpack::type::tuple<B> tup;
    msg.get().convert(&tup);
    f(tup.template get<0>(), bind(&msgpackRet<A>, cb, placeholders::_1));
    delete argsBuf;
  }
  template<class A, class B, class C> static void msgpackArgs(function<void(B, C, function<void(A)>)> f, 
      IOBuffer* argsBuf, function<void(const msgpack::object&)> cb) { 
    msgpack::unpacked msg;
    msgpack::unpack(&msg, argsBuf->pulldown(argsBuf->size()), argsBuf->size());
    msgpack::type::tuple<B, C> tup;
    msg.get().convert(&tup);
    f(tup.template get<0>(), 
        tup.template get<1>(), 
        bind(&msgpackRet<A>, cb, placeholders::_1));
    delete argsBuf;
  }
  template<class A, class B, class C, class D> static void msgpackArgs(function<void(B, C, D, function<void(A)>)> f, 
      IOBuffer* argsBuf, function<void(const msgpack::object&)> cb) { 
    msgpack::unpacked msg;
    msgpack::unpack(&msg, argsBuf->pulldown(argsBuf->size()), argsBuf->size());
    msgpack::type::tuple<B, C, D> tup;
    msg.get().convert(&tup);
    f(tup.template get<0>(), 
        tup.template get<1>(), 
        tup.template get<2>(), 
        bind(&msgpackRet<A>, cb, placeholders::_1));
    delete argsBuf;
  }
  template<class A, class B, class C, class D, class E> static void msgpackArgs(function<void(B, C, D, E, function<void(A)>)> f, 
      IOBuffer* argsBuf, function<void(const msgpack::object&)> cb) { 
    msgpack::unpacked msg;
    msgpack::unpack(&msg, argsBuf->pulldown(argsBuf->size()), argsBuf->size());
    msgpack::type::tuple<B, C, D, E> tup;
    msg.get().convert(&tup);
    f(tup.template get<0>(), 
        tup.template get<1>(), 
        tup.template get<2>(), 
        tup.template get<3>(), 
        bind(&msgpackRet<A>, cb, placeholders::_1));
    delete argsBuf;
  }
  template<class A, class B, class C, class D, class E, class F> static void msgpackArgs(function<void(B, C, D, E, F, function<void(A)>)> f, 
      IOBuffer* argsBuf, function<void(const msgpack::object&)> cb) { 
    msgpack::unpacked msg;
    msgpack::unpack(&msg, argsBuf->pulldown(argsBuf->size()), argsBuf->size());
    msgpack::type::tuple<B, C, D, E, F> tup;
    msg.get().convert(&tup);
    f(tup.template get<0>(), 
        tup.template get<1>(), 
        tup.template get<2>(), 
        tup.template get<3>(), 
        tup.template get<4>(), 
        bind(&msgpackRet<A>, cb, placeholders::_1));
    delete argsBuf;
  }
}
// Expose it all.

template<class A>
void RPCServer::registerFunction(const char* name, function<A()> f) {
  registerFunctionAsync<A>(name, bind(&immediateToCallback<A>, f, 
    placeholders::_1));
}
template<class A, class B>
void RPCServer::registerFunction(const char* name, function<A(B)> f) {
  registerFunctionAsync<A,B>(name, bind(&immediateToCallback<A, B>, f, 
    placeholders::_1, placeholders::_2));
}
template<class A, class B, class C>
void RPCServer::registerFunction(const char* name, function<A(B,C)> f) {
  registerFunctionAsync<A,B,C>(name, bind(&immediateToCallback<A, B, C>, f, 
    placeholders::_1, placeholders::_2, placeholders::_3));
}
template<class A, class B, class C, class D>
void RPCServer::registerFunction(const char* name, function<A(B,C,D)> f) {
  registerFunctionAsync<A,B,C,D>(name, bind(&immediateToCallback<A, B, C, D>, f, 
    placeholders::_1, placeholders::_2, placeholders::_3, 
    placeholders::_4));
}
template<class A, class B, class C, class D, class E>
void RPCServer::registerFunction(const char* name, function<A(B,C,D,E)> f) {
  registerFunctionAsync<A,B,C,D,E>(name, bind(&immediateToCallback<A, B, C, D, E>, f, 
    placeholders::_1, placeholders::_2, placeholders::_3, 
    placeholders::_4, placeholders::_5));
}
template<class A, class B, class C, class D, class E, class F>
void RPCServer::registerFunction(const char* name, function<A(B,C,D,E,F)> f) {
  registerFunctionAsync<A,B,C,D,E,F>(name, bind(&immediateToCallback<A, B, C, D, E, F>, f, 
      placeholders::_1, placeholders::_2, placeholders::_3, 
      placeholders::_4, placeholders::_5, placeholders::_6));
}

template<class A>
void RPCServer::registerFunctionAsync(const char* name, 
    function<void(function<void(A)>)> f) {
  (*_funcs)[name] = bind(&msgpackArgs<A>, f, 
          placeholders::_1, placeholders::_2);
}
template<class A, class B>
void RPCServer::registerFunctionAsync(const char* name, 
    function<void(B, function<void(A)>)> f) {
  (*_funcs)[name] = bind(&msgpackArgs<A, B>, f,
      placeholders::_1, placeholders::_2);
}
template<class A, class B, class C>
void RPCServer::registerFunctionAsync(const char* name, 
    function<void(B, C, function<void(A)>)> f) {
  (*_funcs)[name] = bind(&msgpackArgs<A, B, C>, f, 
      placeholders::_1, placeholders::_2);
}
template<class A, class B, class C, class D>
void RPCServer::registerFunctionAsync(const char* name, 
    function<void(B, C, D, function<void(A)>)> f) {
  (*_funcs)[name] = bind(&msgpackArgs<A, B, C, D>, f, 
      placeholders::_1, placeholders::_2);
}
template<class A, class B, class C, class D, class E>
void RPCServer::registerFunctionAsync(const char* name, 
    function<void(B, C, D, E, function<void(A)>)> f) {
  (*_funcs)[name] = bind(&msgpackArgs<A, B, C, D, E>, f, 
      placeholders::_1, placeholders::_2);
}
template<class A, class B, class C, class D, class E, class F>
void RPCServer::registerFunctionAsync(const char* name, 
    function<void(B, C, D, E, F, function<void(A)>)> f) {
  (*_funcs)[name] = bind(&msgpackArgs<A, B, C, D, E, F>, f, 
      placeholders::_1, placeholders::_2);
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
  void call(const char* name, function<void(A)> f);
  template<class A, class B>
  void call(const char* name, B a1, function<void(A)> f);
  template<class A, class B, class C>
  void call(const char* name, B a1, C a2, function<void(A)> f);
  template<class A, class B, class C, class D>
  void call(const char* name, B a1, C a2, D a3, function<void(A)> f);
  template<class A, class B, class C, class D, class E>
  void call(const char* name, B a1, C a2, D a3, E a4, function<void(A)> f);
  template<class A, class B, class C, class D, class E, class F>
  void call(const char* name, B a1, C a2, D a3, E a4, F a5, function<void(A)> f);

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

namespace {
  template<class A> void msgpackResp(function<void(A)> f, IOBuffer* argsBuf) {
    msgpack::unpacked msg;
    msgpack::unpack(&msg, argsBuf->pulldown(argsBuf->size()), argsBuf->size());
    msgpack::object obj = msg.get();
    A a;
    obj.convert(&a);
    f(a);
    delete argsBuf;
  }
}

template<class A>
void RPCClient::call(const char* name, function<void(A)> callback) {
  _internal->_respCallbacks[_internal->_reqId] = 
      bind(&msgpackResp<A>, callback, placeholders::_1);
  IOBuffer *buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      _internal->_reqId++, string(name), msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  _internal->_socket->write(buf);
}
template<class A, class B>
void RPCClient::call(const char* name, B a0, 
    function<void(A)> callback) {
  _internal->_respCallbacks[_internal->_reqId] = 
      bind(&msgpackResp<A>, callback, placeholders::_1);
  IOBuffer *buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<B>(a0));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      _internal->_reqId++, string(name), msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  _internal->_socket->write(buf);
}
template<class A, class B, class C>
void RPCClient::call(const char* name, B a0, C a1,
    function<void(A)> callback) {
  _internal->_respCallbacks[_internal->_reqId] = 
      bind(&msgpackResp<A>, callback, placeholders::_1);
  IOBuffer *buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<B,C>(a0, a1));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      _internal->_reqId++, string(name), msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  _internal->_socket->write(buf);
}
template<class A, class B, class C, class D>
void RPCClient::call(const char* name, B a0, C a1, D a2,
    function<void(A)> callback) {
  _internal->_respCallbacks[_internal->_reqId] = 
      bind(&msgpackResp<A>, callback, placeholders::_1);
  IOBuffer *buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<B,C,D>(a0, a1, a2));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      _internal->_reqId++, string(name), msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  _internal->_socket->write(buf);
}
template<class A, class B, class C, class D, class E>
void RPCClient::call(const char* name, B a0, C a1, D a2, E a3,
    function<void(A)> callback) {
  _internal->_respCallbacks[_internal->_reqId] = 
      bind(&msgpackResp<A>, callback, placeholders::_1);
  IOBuffer *buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<B,C,D,E>(a0, a1, a2, a3));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      _internal->_reqId++, string(name), msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  _internal->_socket->write(buf);
}
template<class A, class B, class C, class D, class E, class F>
void RPCClient::call(const char* name, B a0, C a1, D a2, E a3, F a4,
    function<void(A)> callback) {
  _internal->_respCallbacks[_internal->_reqId] = 
      bind(&msgpackResp<A>, callback, placeholders::_1);
  IOBuffer *buf = new IOBuffer();
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, msgpack::type::tuple<B,C,D,E,F>(a0, a1, a2, a3, a4));
  msgpack::type::tuple<uint64_t, string, msgpack::type::raw_ref> req(
      _internal->_reqId++, string(name), msgpack::type::raw_ref(sbuf.data(), sbuf.size()));
  msgpack::pack(buf, req);
  _internal->_socket->write(buf);
}
}
#endif
