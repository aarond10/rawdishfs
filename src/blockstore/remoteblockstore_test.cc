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
#include "fileblockstore.h"
#include "remoteblockstore.h"

#include "rpc/rpc.h"
#include "util/url.h"

#include <epoll_threadpool/eventmanager.h>
#include <epoll_threadpool/notification.h>
#include <epoll_threadpool/tcp.h>
#include <msgpack.hpp>

#include <string>
#include <vector>

#include <sys/time.h>
#include <gtest/gtest.h>

using blockstore::FileBlockStore;
using blockstore::RemoteBlockStore;
using epoll_threadpool::EventManager;
using epoll_threadpool::IOBuffer;
using epoll_threadpool::Notification;
using epoll_threadpool::TcpListenSocket;
using epoll_threadpool::TcpSocket;
using msgpack::sbuffer;
using rpc::RPCServer;
using rpc::RPCClient;
using std::tr1::shared_ptr;
using std::string;
using std::vector;

TEST(RemoteBlockStore, Basics) {

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

  mkdir("/tmp/bs", 0777);
  mkdir("/tmp/bs/1234", 0777);
  FileBlockStore *bs = new FileBlockStore("/tmp/bs/1234", 16);
  RegisterRemoteBlockStore(r, bs, 0);

  shared_ptr<RPCClient> c(new RPCClient(TcpSocket::connect(&em, "127.0.0.1", port)));
  c->start();
  RemoteBlockStore rbs(c, 0);

  const char *str = "0123456789abcde";
  Notification n;
  ASSERT_TRUE(rbs.putBlock("abc", new IOBuffer(str, 16)));

  IOBuffer *ret = rbs.getBlock("abc");
  ASSERT_TRUE(ret != NULL);
  ASSERT_EQ(16, ret->size());
  ASSERT_EQ(0, memcmp(str, ret->pulldown(ret->size()), ret->size()));
  delete ret;
  
  LOG(INFO) << "Remove block xxx: " << rbs.removeBlock("xxx");
  LOG(INFO) << "Block Size: " << rbs.blockSize();
  LOG(INFO) << "Free blocks: " << rbs.numFreeBlocks();
  LOG(INFO) << "Total blocks: " << rbs.numTotalBlocks();
  util::BloomFilter bf = rbs.bloomfilter();
  
  delete bs;
}

