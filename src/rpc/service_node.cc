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

shared_ptr<P2PDiscoveryServiceNode> P2PDiscoveryServiceNode::create(
    EventManager* em, const string& host) {
  uint16_t port;
  shared_ptr<TcpListenSocket> s;
  while(s.get() == NULL) {
    port = (rand()%40000) + 1024;
    s = TcpListenSocket::create(em, port);
  }
  shared_ptr<P2PDiscoveryServiceNode> ret(
      new P2PDiscoveryServiceNode(em, host, port, s));
  ret->start();
  return ret;
}

shared_ptr<P2PDiscoveryServiceNode> P2PDiscoveryServiceNode::create(
    EventManager* em, const string& host, int port) {
  shared_ptr<TcpListenSocket> s(TcpListenSocket::create(em, port));
  if (s != NULL) {
    return shared_ptr<P2PDiscoveryServiceNode>(
        new P2PDiscoveryServiceNode(em, host, port, s));
  } else {
    return shared_ptr<P2PDiscoveryServiceNode>();
  }
}

P2PDiscoveryServiceNode::~P2PDiscoveryServiceNode() {
  _group_callbacks.clear();
  for (map< HostPortPair, shared_ptr<Peer> >::const_iterator i = _peers.begin();
       i != _peers.end(); ++i) {
    //i->second->setDisconnectCallback(NULL);
  }
  _peers.clear();
  pthread_mutex_destroy(&_mutex);
}

void P2PDiscoveryServiceNode::addPeer(const string& host, uint16_t port) {
  RPCAddPeer(host, port);
}

void P2PDiscoveryServiceNode::addToGroup(
    const string &group, const string &name) {
  pthread_mutex_lock(&_mutex);
  if (_groups[group].find(name) != _groups[group].end()) {
    _groups[group][name]++;
  } else {
    _groups[group][name] = 1;
    for (map< HostPortPair, shared_ptr<Peer> >::iterator i = _peers.begin();
       i != _peers.end(); ++i) {
      i->second->call<bool, string, string>("addToGroup", group, name);
    }
    for(list<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
        i != _group_callbacks[group].end(); ++i) {
      (*i)(name, true);
    }
  }
  pthread_mutex_unlock(&_mutex);
}

void P2PDiscoveryServiceNode::removeFromGroup(
    const string &group, const string &name) {
  pthread_mutex_lock(&_mutex);
  if (_groups[group].find(name) != _groups[group].end()) {
    if (_groups[group][name] == 1) {
      _groups[group].erase(name);
      for (map< HostPortPair, shared_ptr<Peer> >::iterator i = _peers.begin();
           i != _peers.end(); ++i) {
        i->second->call<bool, string, string>("removeFromGroup", group, name);
      }
      if (_group_callbacks.find(group) != _group_callbacks.end()) {
        for(list<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
            i != _group_callbacks[group].end(); ++i) {
          (*i)(name, false);
        }
      }
    }
  } else {
    _groups[group][name]--;
  }
  pthread_mutex_unlock(&_mutex);
}

void P2PDiscoveryServiceNode::addGroupCallback(
    const string& group, function<void(const string&, bool)> cb) {
  pthread_mutex_lock(&_mutex);
  _group_callbacks[group].push_back(cb);
  // Notify host of our group memberships
  for (map<string, int>::iterator i = _groups[group].begin();
       i != _groups[group].end(); ++i) {
    cb(i->first, true);
  }
  pthread_mutex_unlock(&_mutex);
}

P2PDiscoveryServiceNode::Peer::Peer(
    const HostPortPair& addr, shared_ptr<TcpSocket> s) : _addr(addr), RPCClient(s) {
}

void P2PDiscoveryServiceNode::RemovePeer(HostPortPair addr) {
  pthread_mutex_lock(&_mutex);
  shared_ptr<Peer> peer = _peers[addr];
  // TODO: Remove from any groups this peer is registered in.
  _peers.erase(addr);
  pthread_mutex_unlock(&_mutex);
}

Future<bool> P2PDiscoveryServiceNode::RPCAddPeer(string host, uint16_t port) {
  HostPortPair addr(host, port);
  if (_peers.find(addr) != _peers.end()) {
    // Already connected.
    return true;
  } else {
    shared_ptr<TcpSocket> s = TcpSocket::connect(_em, host, port);
    if (socket == NULL) {
      DLOG(INFO) << "Failed to connect to peer at " << host << ":" << port;
      return false;
    }
    shared_ptr<Peer> peer(new Peer(HostPortPair(host, port), s));
    peer->setDisconnectCallback(
        bind(&P2PDiscoveryServiceNode::RemovePeer, shared_from_this(), addr));
    pthread_mutex_lock(&_mutex);
    _peers[addr] = peer;
    pthread_mutex_unlock(&_mutex);
    peer->start();
    Future<bool> ret = peer->call<bool, string, uint16_t>(
        "addPeer", _host, _port);

    pthread_mutex_lock(&_mutex);

    DLOG(INFO) << "Node on port " << _port << " now has " 
               << _peers.size() << " neighbors.";

    // Hook up the host with our other peers.
    for (map< HostPortPair, shared_ptr<Peer> >::const_iterator i = _peers.begin();
         i != _peers.end(); ++i) {
      if (addr != i->first) {
        peer->call<bool, string, uint16_t>(
          "addPeer", i->first.first, i->first.second);
      }
    }

    // Notify host of our group memberships
    for (map<string, map< string, int> >::iterator i = _groups.begin();
         i != _groups.end(); ++i) {
      for (map<string, int>::iterator j = i->second.begin();
         j != i->second.end(); ++j) {
        peer->call<bool, string, string>("addToGroup", i->first, j->first);
      }
    }
    pthread_mutex_unlock(&_mutex);
    return ret;
  }
}

Future<bool> P2PDiscoveryServiceNode::RPCAddToGroup(string group, string value) {
  pthread_mutex_lock(&_mutex);
  if (_groups[group].find(value) != _groups[group].end()) {
    _groups[group][value]++;
  } else {
    _groups[group][value] = 1;
    if (_group_callbacks.find(group) != _group_callbacks.end()) {
      for(list<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
          i != _group_callbacks[group].end(); ++i) {
        (*i)(value, true);
      }
    }
  }
  pthread_mutex_unlock(&_mutex);
  return true;
}

Future<bool> P2PDiscoveryServiceNode::RPCRemoveFromGroup(string group, string value) {
  pthread_mutex_lock(&_mutex);
  if (_groups[group].find(value) != _groups[group].end()) {
    if (_groups[group][value] == 1) {
      _groups[group].erase(value);
      if (_group_callbacks.find(group) != _group_callbacks.end()) {
        for(list<function<void(const string&, bool)> >::iterator i = _group_callbacks[group].begin();
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

P2PDiscoveryServiceNode::P2PDiscoveryServiceNode(
    EventManager *em, 
    const string& host, int port, 
    shared_ptr<TcpListenSocket> s) : 
        _em(em), _host(host), _port(port), RPCServer(s) {
  pthread_mutex_init(&_mutex, 0);
}

void P2PDiscoveryServiceNode::start() {
  registerFunction<bool, string, uint16_t>("addPeer",
      bind(&P2PDiscoveryServiceNode::RPCAddPeer, shared_from_this(), _1, _2));
  registerFunction<bool, string, string>("addToGroup",
      bind(&P2PDiscoveryServiceNode::RPCAddToGroup, shared_from_this(), _1, _2));
  registerFunction<bool, string, string>("removeFromGroup",
      bind(&P2PDiscoveryServiceNode::RPCRemoveFromGroup, shared_from_this(), _1, _2));
}

}  // end rpc namespace
