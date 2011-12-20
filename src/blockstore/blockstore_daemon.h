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
#ifndef _BLOCKSTORE_BLOCKSTORE_DAEMON_H_
#define _BLOCKSTORE_BLOCKSTORE_DAEMON_H_

#include "blockstore/fileblockstore.h"
#include "rpc/rpc.h"
#include "util/bloomfilter.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <sys/time.h>

namespace epoll_threadpool {
  class EventManager;
  class IOBuffer;
}

namespace blockstore {

using epoll_threadpool::EventManager;
using epoll_threadpool::IOBuffer;
using rpc::RPCServer;
using rpc::RPCServer;
using rpc::RPCClient;
using std::tr1::function;
using std::tr1::shared_ptr;
using std::map;
using std::set;
using std::string;
using std::vector;
using util::BloomFilter;

/**
 * Represents a single node in a full-mesh network of BlockStore nodes. 
 * This class takes care of maintaining bloom filters for each of the
 * BlockStore's in the network, performing incremental checking of blocks,
 * garbage collection and maintaining connections with peers.
 */
class BlockStoreNode {
 public:
  BlockStoreNode(EventManager *em);
  virtual ~BlockStoreNode();

  /**
   * Kicks off the daemon.
   */
  void start();

  /**
   * Stops the daemon.
   */
  void stop();

  /**
   * Returns the TCP port that the node listens on for peering with
   * fellow nodes.
   */
  uint16_t port() const { return _port; }

  /**
   * Attempts to add a new peer at the given ip:port location to the current
   * network of nodes. Note that this need be done for only a single node
   * on the current network. Peer discovery will then kick in to find all
   * other nodes.
   */
  void addPeer(const string &ip, uint16_t port);

  /**
   * Adds a BlockStore to be managed by the node.
   * Each BlockStore is currently represented as files on a locally mounted 
   * filesystem. Blocks are considered equal size (kBlockSize bytes) and
   * each BlockStore is also equally sized (kBlockStoreSize). 
   */
  void addBlockStore(uint64_t bsid, const string &pathname);

  /**
   * Removes a BlockStore from the set managed by this node.
   * Note: This should rarely, if ever be required.
   */
  void removeBlockStore(uint64_t bsid);

  /**
   * Returns the approximate number of free bytes in the whole network. This
   * is equivalent to the sum of all the known garbage collectable blocks
   * in all BlockStore's in the network plus all unallocated free space.
   */
  uint64_t getFreeSpace() const;

  /**
   * Adds a block of data to the network.
   * @param name string an arbitrary label for the data block.
   * @note We currently don't account for space taken up by block names.
   * @param data IOBuffer must be equal or less than the block size of the 
   *             network. Ownership of this parameter is passed to the 
   *             function.
   * @returns true on success, false on error. 
   * @note This function takes ownership of data.
   */
  Future<bool> putBlock(const string &name, IOBuffer *data);

  /**
   * Fetches and returns a block of data from the network.
   * The fetch may take some time so this function works asynchronously,
   * returning the data via callback when available. In the case of error,
   * NULL will be returned. If an IOBuffer is returned, ownership is passed
   * to the caller.
   */
  Future<IOBuffer *> getBlock(const string &name);

  /**
   * Sets a garbage collection bloom filter.
   * Any block names that are not in the filter will be marked as a candidate
   * for being overwritten. The ownership of bloomfilter will be passed to
   * the function.
   */
  void setGCBloomFilter(BloomFilter *bloomfilter);

  /**
   * In the process of incremental checking, a node may come across blocks for
   * which it can't find the next in sequence. The names of these nodes are
   * stored in a list. This function returns the list and then clears it.
   */
  vector<string> getMissingBlocks() const;

 private:
  /**
   * Manages communication with a node's peer.
   */
  class Peer {
   public:
    Peer(const string &ip, uint16_t port);
    virtual ~Peer();

    /**
     * Tells a peer roughly how many free bytes we think we have on local
     * BlockStores.
     */
    //void sendBSFreeSpace(uint64_t bytes);

    /**
     * Sends a local BlockStore BloomFilter to the peer. 
     * This is not *strictly* required but without doing this, file access
     * will be very slow and network usage extremely high for each block.
     * Note that the only case in which we get requests for blocks that are
     * NOT in a provided BloomFilter are when the requested block matches NO 
     * BloomFilters. This will happen for very new blocks and missing blocks.
     */
    void sendBSBloomFilter(uint64_t bsid, const BloomFilter& bloomfilter);

    /**
     * Asks a peer explicitly to store a block.
     * When this is called we will already have an appropriate BlockStore in
     * mind but the peer itself will decide the most appropriate location for
     * us.
     */
    Future<bool> setBlock(const string& name, IOBuffer *data);

    /**
     * Explicitly requests a block from a given peer. This will never recurse.
     */
    Future<IOBuffer *> getBlock(const string& name);

    /**
     * Get free space for a given BlockStore.
     */

   private:
    shared_ptr<RPCClient> _client;
  };

  BlockStore *_findBestLocation(size_t hash);
  BlockStore *_findNextBestLocation(size_t hash);

  EventManager *_em;
  uint16_t _port;
  shared_ptr<RPCServer> _rpc_server;
  set< shared_ptr<Peer> > _peers;
  map< uint64_t, shared_ptr<FileBlockStore> > _blockstores;

  /**
   * Called periodically to incrementally check the state of stored blocks
   * and perform garbage collection.
   */
  void onTimer();

};

}
#endif
