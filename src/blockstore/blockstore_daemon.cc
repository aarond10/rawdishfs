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

#include "blockstore.h"
#include "blockstore_daemon.h"
#include "rpc/rpc.h"
#include "util/url.h"

#include <epoll_threadpool/barrier.h>
#include <epoll_threadpool/eventmanager.h>
#include <epoll_threadpool/iobuffer.h>
#include <epoll_threadpool/notification.h>
#include <epoll_threadpool/tcp.h>

#include <string>
#include <vector>

#include <sys/time.h>

namespace blockstore {

using epoll_threadpool::Barrier;
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

namespace {

// The interval in seconds between incremental file processing attempts.
const double TIMER_INTERVAL = 1.0;
 
/**
 * Helper that stores a uint64 and then runs a callback
 */
void uint64Helper(uint64_t* result, function<void()> callback, uint64_t value) {
  *result = value;
  callback();
}

/**
 * Helper function called when storing blocks. When both primary and 
 * secondary locations for blocks have been probed for free space. This 
 * function chooses the BlockStore with the most free space and stores 
 * the block there.
 */
void putBlockHelper(
    const string &name, IOBuffer *data,
    BlockStore *bsA, BlockStore *bsB, 
    uint64_t *freeA, uint64_t *freeB,
    Future<bool> ret) {
  if (*freeA < *freeB) {
    ret.set(bsA->putBlock(name, data));
  } else {
    ret.set(bsB->putBlock(name, data));
  }
  delete freeA;
  delete freeB;
}
}

BlockStoreNode::BlockStoreNode(EventManager *em)
    : _em(em) {
  
  shared_ptr<TcpListenSocket> s;
  while(s == NULL) {
    _port = rand();
    s = TcpListenSocket::create(em, _port);
  }
  _rpc_server = RPCServer::create(s);
}

BlockStoreNode::~BlockStoreNode() {
  stop();
}

void BlockStoreNode::start() {
  EventManager::WallTime t = EventManager::currentTime();
  _em->enqueue(std::tr1::bind(&BlockStoreNode::onTimer, this),
               t + TIMER_INTERVAL);
}

void BlockStoreNode::stop() {
  _rpc_server.reset();
}

void BlockStoreNode::addPeer(const string &ip, uint16_t port) {
  _peers.insert(shared_ptr<Peer>(new Peer(ip, port)));
}

void BlockStoreNode::addBlockStore(uint64_t bsid, const string &pathname) {
  _blockstores[bsid] = shared_ptr<FileBlockStore>(new FileBlockStore(pathname));
}

void BlockStoreNode::removeBlockStore(uint64_t bsid) {
  if (_blockstores.find(bsid) != _blockstores.end()) {
    _blockstores.erase(bsid);
  }
}

uint64_t BlockStoreNode::getFreeSpace() const {
  return -1;
}

BlockStore *BlockStoreNode::_findBestLocation(size_t hash) {
  return NULL;
}

BlockStore *BlockStoreNode::_findNextBestLocation(size_t hash) {
  return NULL;
}

Future<bool> BlockStoreNode::putBlock(const string &name, IOBuffer *data) {

  uint64_t* freeA = new uint64_t();
  uint64_t* freeB = new uint64_t();
  Future<bool> ret;
  BlockStore* bsA = 
      _findBestLocation(std::tr1::hash<string>()(name));
  BlockStore* bsB = 
      _findNextBestLocation(std::tr1::hash<string>()(name));
  Barrier* barrier = new Barrier(2, 
      bind(&putBlockHelper, name, data, bsA, bsB, freeA, freeB, ret));
  bsA->numFreeBlocks()
      .addCallback(bind(&uint64Helper, freeA, barrier->callback(),
                   std::tr1::placeholders::_1));
  bsB->numFreeBlocks()
      .addCallback(bind(&uint64Helper, freeB, barrier->callback(),
                   std::tr1::placeholders::_1));
  return ret;
}

Future<IOBuffer *> BlockStoreNode::getBlock(const string &name) {
  // Find list of all candidate BlockStores.
  // If list is empty, search the best 3 candidate BS just in case the block
  // was added recently.

  return NULL;
}

void BlockStoreNode::setGCBloomFilter(BloomFilter *bloomfilter) {
}

vector<string> BlockStoreNode::getMissingBlocks() const {
  return vector<string>();
}

void BlockStoreNode::onTimer() {
  if(_peers.size() && _blockstores.size()) {
/*
    // First search for timed out peers and remove them from our connection list.
    time_t expire_time = time(0) - 60;
    list<Address> remove_list;
    for (map<Address, mp::shared_ptr<Peer> >::iterator i = peers->begin();
	i != peers->end();) {
      if (i->second->last_update() < expire_time) {
	remove_list.push_back(i->first);
      }
    }
    for (list<Address>::iterator i = remove_list.begin(); 
	i != remove_list.end(); ++i) {
      peers->erase(*i);
    }
	
    // Validate local blocks
    string key = _bs->next();

    mp::sync< mp::shared_ptr<util::BloomFilter> >::ref 
	gc_bloomfilter(_gc_bloomfilter);
    if (*gc_bloomfilter && !(*gc_bloomfilter)->mayContain(key)) {
      DVLOG(10) << "Removing " << key << " because it fails to match GC bloomfilter."; 
      _bs->remove(key);
    } else {
      string nextKey = getNextBlockName(key);
      if (!nextKey.empty()) {
	// This block key looks like a reed solomon chunk. 
	// Make sure the next block exists within known reach.
	int blockFound = false;
	for(map<Address, mp::shared_ptr<Peer> >::iterator i = 
	    peers->begin();
	    i != peers->end(); ++i) {
	  if (i->second->bloomfilter().mayContain(nextKey)) {
	    blockFound = true;
	    break;
	  }
	}
	if (!blockFound) {
	  mp::sync< set<string> >::ref missingBlocks(_missingBlocks);
	  missingBlocks->insert(key);
	}
      }
    }
*/
  }

  if (_rpc_server) {
    EventManager::WallTime t = EventManager::currentTime();
    _em->enqueue(std::tr1::bind(&BlockStoreNode::onTimer, this),
                 t + TIMER_INTERVAL);
  }
}

BlockStoreNode::Peer::Peer(const string &ip, uint16_t port) {
}

BlockStoreNode::Peer::~Peer() {
}

void BlockStoreNode::Peer::sendBSBloomFilter(uint64_t bsid, 
                                             const BloomFilter& bloomfilter) {
}

Future<bool> BlockStoreNode::Peer::setBlock(const string& name, 
                                            IOBuffer *data) {
}

Future<IOBuffer *> BlockStoreNode::Peer::getBlock(const string& name) {
}

}

// TODO(aarond10): Move main() to its own file.
int main(int argc, char *argv[]) {
  epoll_threadpool::EventManager em;
  blockstore::BlockStoreNode bsn(&em);

  // TODO(aarond10): Temporary hard-coded BlockStore
  bsn.addBlockStore(0x01234567, "./01234567/");
  bsn.addBlockStore(0x89abcdef, "./89abcdef/");
  bsn.addBlockStore(0x00112233, "./00112233/");
  bsn.addBlockStore(0x44556677, "./44556677/");
  bsn.addBlockStore(0x8899aabb, "./8899aabb/");
  bsn.addBlockStore(0xccddeeff, "./ccddeeff/");

  // TODO(aarond10): Temporary hard-coded peers
  bsn.addPeer("192.168.1.100", 12345);
  bsn.addPeer("192.168.1.101", 12345);
  bsn.addPeer("192.168.1.102", 12345);

  bsn.start();
  while (true) {
    sleep(1);
  }
  bsn.stop();

  return 0;
}

