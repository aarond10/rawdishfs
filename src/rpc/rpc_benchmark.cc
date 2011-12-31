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

#include <set>
#include <string>
#include <vector>

#include <sys/time.h>
#include <gtest/gtest.h>

using epoll_threadpool::CountingNotification;
using epoll_threadpool::EventManager;
using epoll_threadpool::Notification;
using epoll_threadpool::TcpListenSocket;
using epoll_threadpool::TcpSocket;
using msgpack::sbuffer;
using rpc::RPCServer;
using rpc::RPCClient;
using std::tr1::shared_ptr;
using std::set;
using std::string;
using std::vector;

pthread_mutex_t m;
int global_cnt = 0;

int addArgs2(int a, int b) {
  return a + b;
}


void checkValue(int expected, CountingNotification *n, int actual) {
  CHECK_EQ(expected, actual);
  n->signal();

  pthread_mutex_lock(&m);
  global_cnt++;
  pthread_mutex_unlock(&m);
  //LOG(INFO) << "global_cnt is " << global_cnt;
}

void StartRPCs(shared_ptr<RPCClient> client, CountingNotification *n, int numCalls) {
  for (int i = 0; i < numCalls; ++i) {
    client->call<int, int, int>("addArgs2", i, i+1).addCallback(
        std::tr1::bind(&checkValue, 2*i+1, n, std::tr1::placeholders::_1));
  }
}

TEST(RPCClient, Benchmark) {

  int port;
  EventManager em;

  const int kNumThreads = 16;
  em.start(kNumThreads);

  pthread_mutex_init(&m, 0);

  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(&em, port);
  }
  shared_ptr<RPCServer> r(RPCServer::create(s));
  s.reset();

  r->registerFunction<int, int, int>("addArgs2", &addArgs2);
  r->start();

  set< shared_ptr<RPCClient> > clients;

  const int kNumCalls = 1000000;
  CountingNotification *n[kNumThreads];
  shared_ptr<RPCClient> c[kNumThreads];

  LOG(INFO) << "Launching";
  for (int i = 0; i < kNumThreads; i++) {
    c[i].reset(new RPCClient(TcpSocket::connect(&em, "127.0.0.1", port)));
    n[i] = new CountingNotification(kNumCalls/kNumThreads);

    c[i]->start();
    em.enqueue(std::tr1::bind(&StartRPCs, c[i], n[i], kNumCalls/kNumThreads));
  }

  LOG(INFO) << "Waiting for completion";
  for (int i = 0; i < kNumThreads; i++) {
    LOG(INFO) << "Waiting... " << i;
    n[i]->wait();
    LOG(INFO) << "Done... " << i;
    delete n[i];
    c[i]->disconnect();
    c[i].reset();
  }
  LOG(INFO) << "Done";

  pthread_mutex_destroy(&m);
}
