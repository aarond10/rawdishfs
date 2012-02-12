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
#ifndef _RPC_SERVICE_NODE_H_
#define _RPC_SERVICE_NODE_H_

#include "epoll_threadpool/eventmanager.h"
#include "rpc/rpc.h"

using epoll_threadpool::EventManager;
using std::list;
using std::map;

namespace rpc {

/**
 * Encapsulates functionality to run a full-mesh RPC-based P2P node.
 * The node acts as an RPCServer so you can add your own custom RPC methods
 * to it. It also exports a "DirectoryService" that allows for the
 * registration of services provided by the local node. The contents of
 * this directory are shared with all other peers with eventual consistency
 * semantics. (We provide no timing or ordering guarentees)
 */
class ServiceNode {
 public:
  /**
   * Given a publically addressable IP and local port to listen on,
   * creates a new P2P Discovery Service Node.
   */
  static shared_ptr<ServiceNode> create(
    EventManager* em, const string& host);
  static shared_ptr<ServiceNode> create(
      EventManager* em, const string& host, int port);
  virtual ~ServiceNode();

  /**
   * Returns the hostname we're listening on for incoming connections.
   */
  const string& host() const { return _internal->host(); }

  /**
   * Returns the port we're listening on for incoming connections.
   */
  uint16_t port() const { return _internal->port(); }

  /**
   * Connects to a given peer. Only a single peer is required to
   * connect to the entire network as peers will share their addresses with
   * each other.
   */
  void addPeer(const string& host, uint16_t port) {
    _internal->RPCAddPeer(host, port);
  }

  /**
   * Adds an item to a group. Either strings may be anything.
   * The network + processing cost is O(N) in number of peers.
   */
  void addToGroup(const string &group, const string &name) {
    _internal->addToGroup(group, name);
  }

  /**
   * Removes an item to a group. Does nothing if the group and/or item
   * don't exist. This will also be done automatically if the peer
   * disconnects.
   * The network + processing cost is O(N) in number of peers.
   */
  void removeFromGroup(const string &group, const string &name) {
    _internal->removeFromGroup(group, name);
  }

  /**
   * Registers a callback to be triggered whenever a member is added or
   * removed from the given group. The name of the member and true
   * or false is passed to the callback reflecting whether the event is
   * an add or a remove respectively.
   */
  void addGroupCallback(
      const string& group, function<void(const string&, bool)> cb) {
    _internal->addGroupCallback(group, cb);
  }

  /**
   * Deregisters all callbacks for a specific group.
   */
  void removeGroupCallback(const string& group) {
    _internal->removeGroupCallback(group);
  }

 protected:
  ServiceNode(EventManager *em, const string& host, int port, 
              shared_ptr<TcpListenSocket> s);

  /**
   * Internal class that does most of our work. We reference count 
   * this rather than ourself to avoid circular reference.
   */
  class Internal : public enable_shared_from_this<Internal> {
   public:
    Internal(EventManager* em, const string& host, uint16_t port);
    virtual ~Internal();

    /**
     * Kills all peer connections.
     */
    void shutdown();

    const string& host() const { return _host; }
    uint16_t port() const { return _port; }
    void addToGroup(const string& group, const string& name);
    void removeFromGroup(const string& group, const string& name);
    void addGroupCallback(const string& group, 
                          function<void(const string&, bool)> cb);
    void removeGroupCallback(const string& group);
   private:
    typedef std::pair<string, uint16_t> HostPortPair;

    pthread_mutex_t _mutex;
    EventManager *_em;
    string _host;
    uint16_t _port;

    map<HostPortPair, shared_ptr<RPCClient> > _peers;
    map<string, map<string, int> > _groups;
    map<string, vector< function<void(const string&, bool)> > > _group_callbacks;

    /**
     * Callback method triggered when a connection is lost.
     */
    void RemovePeer(HostPortPair addr);

   public:
    /**
     * RPC Functions
     */
    Future<bool> RPCAddPeer(string host, uint16_t port);
    Future<bool> RPCAddToGroup(string group, string value);
    Future<bool> RPCRemoveFromGroup(string group, string value);
  };

 private:
  shared_ptr<RPCServer> _rpc_server;
  shared_ptr<Internal> _internal;
};


}  // end rpc namespace
#endif
