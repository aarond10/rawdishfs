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

#include "util/url.h"

#include <epoll_threadpool/eventmanager.h>
#include <epoll_threadpool/notification.h>
#include <epoll_threadpool/tcp.h>
#include <msgpack.hpp>

#include <string>
#include <vector>

#include <sys/time.h>
#include <gtest/gtest.h>

using epoll_threadpool::EventManager;
using epoll_threadpool::Notification;
using epoll_threadpool::TcpListenSocket;
using epoll_threadpool::TcpSocket;
using msgpack::sbuffer;
using rpc::RPCServer;
using rpc::RPCClient;
using std::tr1::shared_ptr;
using std::string;
using std::vector;

int complexArgs(const vector<int> &in) {
  int ret = 0;
  for (vector<int>::const_iterator i = in.begin(); i != in.end(); ++i) {
    ret += *i;
  }
  return ret;
}

vector<int> complexRet() {
  vector<int> ret;
  ret.push_back(1);
  ret.push_back(2);
  ret.push_back(3);
  return ret;
}

void checkComplexRet(const vector<int> &vec, Notification *n) {
  ASSERT_EQ(3, vec.size());
  EXPECT_EQ(1, vec[0]);
  EXPECT_EQ(2, vec[1]);
  EXPECT_EQ(3, vec[2]);
  n->signal();
}

int addArgs0() {
  return 11;
}

int addArgs1(int a) {
  return a + 1;
}

int addArgs2(int a, int b) {
  return a + b;
}

int addArgs3(int a, int b, int c) {
  return a + b + c;
}

int addArgs4(int a, int b, int c, int d) {
  return a + b + c + d;
}

int addArgs5(int a, int b, int c, int d, int e) {
  return a + b + c + d + e;
}

string toUpperStr(string in) {
  for (size_t i = 0; i < in.size(); ++i) {
    in[i] = ::toupper(in[i]);
  }
  return in;
}

void checkData(const vector<int> &data, Notification *n) {
  ASSERT_EQ(3, data.size());
  ASSERT_EQ(10, data[0]);
  ASSERT_EQ(20, data[1]);
  ASSERT_EQ(30, data[2]);
  n->signal();
}

template<class A>
void checkValue(A a, A b, Notification *n) {
  ASSERT_EQ(a,b);
  n->signal();
}

TEST(RPCClient, Construction) {

  int port;
  EventManager em;

  em.start(4);

  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(&em, port);
  }
  shared_ptr<RPCServer> r(RPCServer::create(s));
  s.reset();
}

void acceptConnectionCallback(shared_ptr<RPCServer::Connection> c) {
  c->start();
}

TEST(RPCServer, UnknownClientConnect) {

  int port;
  EventManager em;

  em.start(4);

  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(&em, port);
  }
  shared_ptr<RPCServer> r(RPCServer::create(s));
  s.reset();

  r->setAcceptCallback(std::tr1::bind(&acceptConnectionCallback, std::tr1::placeholders::_1));
  r->start();

  LOG(INFO) << "Connecting...";

  shared_ptr<TcpSocket> t(TcpSocket::connect(&em, "127.0.0.1", port));
  Notification n;
  t->setDisconnectCallback(std::tr1::bind(&Notification::signal, &n));
  t->start();
  n.wait();
}

TEST(RPCClient, PreCallDisconnect) {

  int port;
  EventManager em;

  em.start(4);

  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(&em, port);
  }
  shared_ptr<RPCServer> r(RPCServer::create(s));
  s.reset();

  r->setAcceptCallback(std::tr1::bind(&acceptConnectionCallback, std::tr1::placeholders::_1));
  r->start();

  RPCClient c(TcpSocket::connect(&em, "127.0.0.1", port));
  Notification n;
  c.setDisconnectCallback(std::tr1::bind(&Notification::signal, &n));
  c.start();
  n.wait();
}

void missingFuncCallback(string dummy) {
  // This should never get called.
  ASSERT_TRUE(false);
}

TEST(RPCClient, ConstructionConnectBadCall) {

  int port;
  EventManager em;

  em.start(4);

  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(&em, port);
  }
  shared_ptr<RPCServer> r(RPCServer::create(s));
  s.reset();
  r->start();

  Notification n;

  RPCClient c(TcpSocket::connect(&em, "127.0.0.1", port));
  c.setDisconnectCallback(std::tr1::bind(&Notification::signal, &n));
  c.start();
  c.call<string>("missingFunc", std::tr1::bind(&missingFuncCallback, std::tr1::placeholders::_1));
  n.wait();

  c.disconnect();
}

TEST(RPCClient, Basics) {

  int port;
  EventManager em;

  em.start(4);

  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(&em, port);
  }
  shared_ptr<RPCServer> r(RPCServer::create(s));
  s.reset();

  r->registerFunction<string, string>("toUpper", &toUpperStr);
  r->registerFunction< int, vector<int> >("complexArgs", &complexArgs);
  r->registerFunction< vector<int> >("complexRet", &complexRet);
  r->registerFunction<int>("addArgs0", &addArgs0);
  r->registerFunction<int, int>("addArgs1", &addArgs1);
  r->registerFunction<int, int, int>("addArgs2", &addArgs2);
  r->registerFunction<int, int, int, int>("addArgs3", &addArgs3);
  r->registerFunction<int, int, int, int, int>("addArgs4", &addArgs4);
  r->registerFunction<int, int, int, int, int, int>("addArgs5", &addArgs5);

  r->start();

  Notification n_disconnect;
  RPCClient c(TcpSocket::connect(&em, "127.0.0.1", port));
  c.setDisconnectCallback(std::tr1::bind(&Notification::signal, &n_disconnect));
  c.start();
  // Make sure trivial call works on string data types.
  Notification na,nb,nc;
  c.call<string,string>("toUpper","string", std::tr1::bind(&checkValue<string>, string("STRING"), std::tr1::placeholders::_1, &na));

  vector<int> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(5);
  c.call< int, vector<int> >("complexArgs", vec, std::tr1::bind(&checkValue<int>, 15, std::tr1::placeholders::_1, &nb));

  c.call< vector<int> >("complexRet", std::tr1::bind(&checkComplexRet, std::tr1::placeholders::_1, &nc));

  // These will get serialized and sent sequentially to the server. There is no guarantee about order of execution on server side though.
  Notification n0,n1,n2,n3,n4,n5;
  c.call<int>("addArgs0", std::tr1::bind(&checkValue<int>, 11, std::tr1::placeholders::_1, &n0));
  c.call<int, int>("addArgs1", 1, std::tr1::bind(&checkValue<int>, 2, std::tr1::placeholders::_1, &n1));
  c.call<int, int, int>("addArgs2", 1, 2, std::tr1::bind(&checkValue<int>, 3, std::tr1::placeholders::_1, &n2));
  c.call<int, int, int, int>("addArgs3", 1, 2, 3, std::tr1::bind(&checkValue<int>, 6, std::tr1::placeholders::_1, &n3));
  c.call<int, int, int, int, int>("addArgs4", 1, 2, 3, 4, std::tr1::bind(&checkValue<int>, 10, std::tr1::placeholders::_1, &n4));
  c.call<int, int, int, int, int, int>("addArgs5", 1, 2, 3, 4, 5, std::tr1::bind(&checkValue<int>, 15, std::tr1::placeholders::_1, &n5));

  na.wait();
  nb.wait();
  nc.wait();
  n0.wait();
  n1.wait();
  n2.wait();
  n3.wait();
  n4.wait();
  n5.wait();

  c.disconnect();
  n_disconnect.wait();
}

// TODO: Test what happens when we leave an RPCServer connected to an RPCClient and go out of scope. Client should close then server.
// TODO: Test what happens when we delete a server with active client.
