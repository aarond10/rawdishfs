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

#include "rpc/rpc.h"
#include "rpc/service_node.h"
#include "util/url.h"

#include <epoll_threadpool/eventmanager.h>
#include <epoll_threadpool/notification.h>

#include <string>
#include <list>

#include <gtest/gtest.h>

#include <sys/time.h>

namespace rpc {

using epoll_threadpool::EventManager;
using epoll_threadpool::Notification;
using rpc::ServiceNode;
using std::tr1::shared_ptr;
using std::list;
using std::string;

void GroupCallbackHelper(string expectedValue,
                         Notification *n,
                         const string& value, bool isAdded) {
  LOG(INFO) << "Callback triggered with value " << value << " and isAdded " << isAdded;
  if (expectedValue == value && isAdded) {
    n->signal();
  } else {
  }
}

TEST(ServiceNodeTest, Basics) {

  int port;
  EventManager em;

  em.start(4);

  shared_ptr<ServiceNode> node1(ServiceNode::create(&em, "127.0.0.1"));
  shared_ptr<ServiceNode> node2(ServiceNode::create(&em, "127.0.0.1"));
  shared_ptr<ServiceNode> node3(ServiceNode::create(&em, "127.0.0.1"));
  shared_ptr<ServiceNode> node4(ServiceNode::create(&em, "127.0.0.1"));
  shared_ptr<ServiceNode> node5(ServiceNode::create(&em, "127.0.0.1"));

  Notification n1, n2, n3, n4, n5;

  node1->addGroupCallback("test", bind(&GroupCallbackHelper, "n1", &n1, _1, _2));
  node2->addGroupCallback("test", bind(&GroupCallbackHelper, "n1", &n2, _1, _2));
  node3->addGroupCallback("test", bind(&GroupCallbackHelper, "n1", &n3, _1, _2));
  node4->addGroupCallback("test", bind(&GroupCallbackHelper, "n1", &n4, _1, _2));
  node5->addGroupCallback("test", bind(&GroupCallbackHelper, "n1", &n5, _1, _2));

  node1->addPeer("127.0.0.1", node2->port());
  node2->addPeer("127.0.0.1", node3->port());
  node3->addPeer("127.0.0.1", node4->port());
  node4->addPeer("127.0.0.1", node5->port());
  node5->addPeer("127.0.0.1", node1->port());

  LOG(INFO) << "Listening on ports "
            << node1->port() << ", "
            << node2->port() << ", "
            << node3->port() << ", "
            << node4->port() << ", "
            << node5->port();

  node1->addToGroup("test", "n1");
  node2->addToGroup("test", "n2");
  node3->addToGroup("test", "n3");
  node4->addToGroup("test", "n4");
  node5->addToGroup("test", "n5");

  // TODO: Test duplicate members. - Should probably reference count them.

  LOG(INFO) << "Waiting on group membership to be confirmed.";

  n1.wait();
  n2.wait();
  n3.wait();
  n4.wait();
  n5.wait();

  LOG(INFO) << "Freeing nodes.";

  node1.reset();
  node2.reset();
  node3.reset();
  node4.reset();
  node5.reset();

  usleep(10000);
}

}
