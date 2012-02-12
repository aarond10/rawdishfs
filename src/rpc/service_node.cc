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
#include "service_node.h"

namespace rpc {

namespace {
void DummyFunc(shared_ptr<RPCClient> client) {
  // Dummy function used to ensure we don't destroy out peer before we get a result.
}
}  // end anonymous namespace


shared_ptr<ServiceNode> ServiceNode::create(
    EventManager* em, const string& host) {
  uint16_t port;
  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(em, port);
  }
  return shared_ptr<ServiceNode>(
      new ServiceNode(em, host, port, s));
}

shared_ptr<ServiceNode> ServiceNode::create(
    EventManager* em, const string& host, int port) {
  shared_ptr<TcpListenSocket> s(TcpListenSocket::create(em, port));
  if (s != NULL) {
    return shared_ptr<ServiceNode>(
        new ServiceNode(em, host, port, s));
  } else {
    return shared_ptr<ServiceNode>();
  }
}

ServiceNode::ServiceNode(
    EventManager *em, 
    const string& host, int port, 
    shared_ptr<TcpListenSocket> s) : 
        _rpc_server(RPCServer::create(s)),
        _internal(new Internal(em, host, port)) {
  _rpc_server->registerFunction<bool, string, uint16_t>("addPeer",
      bind(&ServiceNode::Internal::RPCAddPeer, 
           _internal, _1, _2));
  _rpc_server->registerFunction<bool, string, string>("addToGroup",
      bind(&ServiceNode::Internal::RPCAddToGroup, 
           _internal, _1, _2));
  _rpc_server->registerFunction<bool, string, string>("removeFromGroup",
      bind(&ServiceNode::Internal::RPCRemoveFromGroup, 
           _internal, _1, _2));
  _rpc_server->start();
}

ServiceNode::~ServiceNode() {
  _internal->shutdown();
  _rpc_server.reset();
  _internal.reset();
}

ServiceNode::Internal::Internal(
    EventManager *em, const string& host, uint16_t port)
        : _em(em), _host(host), _port(port) {
  pthread_mutex_init(&_mutex, 0);
}

ServiceNode::Internal::~Internal() {
  pthread_mutex_lock(&_mutex);
  _group_callbacks.clear();
  for (map< HostPortPair, shared_ptr<RPCClient> >::const_iterator i = _peers.begin();
       i != _peers.end(); ++i) {
    i->second->setDisconnectCallback(NULL);
    i->second->disconnect();
  }
  _peers.clear();
  pthread_mutex_unlock(&_mutex);
  pthread_mutex_destroy(&_mutex);
}

void ServiceNode::Internal::shutdown() {
  _peers.clear();
}

void ServiceNode::Internal::addToGroup(
    const string &group, const string &name) {
  pthread_mutex_lock(&_mutex);

  // Notify other peers. We do this even if reference count > 1 so they all
  // have the same state.
  for (map< HostPortPair, shared_ptr<RPCClient> >::iterator i = _peers.begin();
     i != _peers.end(); ++i) {
    i->second->call<bool, string, string>("addToGroup", group, name)
        .addCallback(bind(&DummyFunc, i->second));
  }

  // Group membership is reference counted. Events will only fire the first
  // time a member is added
  if (_groups[group].find(name) != _groups[group].end()) {
    _groups[group][name]++;
  } else {
    _groups[group][name] = 1;
    // Trigger local callbacks.
    for(vector<function<void(const string&, bool)> >::iterator i = 
        _group_callbacks[group].begin();
        i != _group_callbacks[group].end(); ++i) {
      (*i)(name, true);
    }
  }
  pthread_mutex_unlock(&_mutex);
}

void ServiceNode::Internal::removeFromGroup(
    const string &group, const string &name) {
  pthread_mutex_lock(&_mutex);

  // Notify other peers.
  for (map< HostPortPair, shared_ptr<RPCClient> >::iterator i = _peers.begin();
       i != _peers.end(); ++i) {
    i->second->call<bool, string, string>("removeFromGroup", group, name)
        .addCallback(bind(&DummyFunc, i->second));
  }

  // Trigger local event if reference count went to zero.
  if (_groups[group].find(name) != _groups[group].end()) {
    if (_groups[group][name] == 1) {
      _groups[group].erase(name);
      if (_group_callbacks.find(group) != _group_callbacks.end()) {
        for(vector<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
            i != _group_callbacks[group].end(); ++i) {
          (*i)(name, false);
        }
      }
    }
  } else {
    if (_groups[group][name] > 1) {
      _groups[group][name]--;
    } else {
      DLOG(ERROR) << "Reference count decremented below zero.";
    }
  }
  pthread_mutex_unlock(&_mutex);
}

void ServiceNode::Internal::addGroupCallback(
    const string& group, function<void(const string&, bool)> cb) {
  pthread_mutex_lock(&_mutex);
  _group_callbacks[group].push_back(cb);
  // Notify cb of our group memberships so far so they have a complete
  // snapshot.
  for (map<string, int>::iterator i = _groups[group].begin();
       i != _groups[group].end(); ++i) {
    cb(i->first, true);
  }
  pthread_mutex_unlock(&_mutex);
}

void ServiceNode::Internal::removeGroupCallback(const string& group) {
  pthread_mutex_lock(&_mutex);
  _group_callbacks.erase(group);
  pthread_mutex_unlock(&_mutex);
}

void ServiceNode::Internal::RemovePeer(HostPortPair addr) {
  pthread_mutex_lock(&_mutex);
  DLOG(INFO) << "Internal::RemovePeer( " << addr.first << ":" << addr.second << ")";
  shared_ptr<RPCClient> peer = _peers[addr];
  // TODO: Remove from any groups this peer is registered in.
  _peers.erase(addr);
  pthread_mutex_unlock(&_mutex);
}

Future<bool> ServiceNode::Internal::RPCAddPeer(string host, uint16_t port) {
  HostPortPair addr(host, port);
  if (_peers.find(addr) != _peers.end()) {
    // Already connected.
    return true;
  } else {
    shared_ptr<TcpSocket> s = TcpSocket::connect(_em, host, port);
    if (s == NULL) {
      DLOG(INFO) << "Failed to connect to peer at " << host << ":" << port;
      return false;
    }
    pthread_mutex_lock(&_mutex);

    shared_ptr<RPCClient> peer(new RPCClient(s));
    peer->setDisconnectCallback(
        bind(&ServiceNode::Internal::RemovePeer, shared_from_this(), addr));
    _peers[addr] = peer;

    peer->start();
    Future<bool> ret = peer->call<bool, string, uint16_t>(
        "addPeer", _host, _port);

    ret.addCallback(bind(&DummyFunc, peer));

    DLOG(INFO) << "Node on port " << _port << " now has " 
               << _peers.size() << " neighbors.";

    // Hook up the host with our other peers.
    for (map< HostPortPair, shared_ptr<RPCClient> >::const_iterator i = _peers.begin();
         i != _peers.end(); ++i) {
      if (addr != i->first) {
        peer->call<bool, string, uint16_t>(
          "addPeer", i->first.first, i->first.second)
              .addCallback(bind(&DummyFunc, peer));
      }
    }

    // Notify host of our group memberships
    for (map<string, map< string, int> >::iterator i = _groups.begin();
         i != _groups.end(); ++i) {
      for (map<string, int>::iterator j = i->second.begin();
         j != i->second.end(); ++j) {
        peer->call<bool, string, string>("addToGroup", i->first, j->first)
            .addCallback(bind(&DummyFunc, peer));
      }
    }
    pthread_mutex_unlock(&_mutex);
    return ret;
  }
}

Future<bool> ServiceNode::Internal::RPCAddToGroup(string group, string value) {
  pthread_mutex_lock(&_mutex);
  if (_groups[group].find(value) != _groups[group].end()) {
    _groups[group][value]++;
  } else {
    _groups[group][value] = 1;
    if (_group_callbacks.find(group) != _group_callbacks.end()) {
      for(vector<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
          i != _group_callbacks[group].end(); ++i) {
        (*i)(value, true);
      }
    }
  }
  pthread_mutex_unlock(&_mutex);
  return true;
}

Future<bool> ServiceNode::Internal::RPCRemoveFromGroup(string group, string value) {
  pthread_mutex_lock(&_mutex);
  if (_groups[group].find(value) != _groups[group].end()) {
    if (_groups[group][value] == 1) {
      _groups[group].erase(value);
      if (_group_callbacks.find(group) != _group_callbacks.end()) {
        for(vector<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
            i != _group_callbacks[group].end(); ++i) {
          (*i)(value, false);
        }
      }
    }
  } else {
    _groups[group][value]--;
  }
  pthread_mutex_unlock(&_mutex);
  return true;
}

}  // end rpc namespace
