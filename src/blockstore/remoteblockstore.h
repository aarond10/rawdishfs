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
#ifndef _BLOCKSTORE_REMOTEBLOCKSTORE_H_
#define _BLOCKSTORE_REMOTEBLOCKSTORE_H_

#include "blockstore.h"
#include "rpc/rpc.h"

#include <stdint.h>
#include <stdio.h>


#include <string>
#include <tr1/functional>
#include <vector>

#include <epoll_threadpool/future.h>
#include <epoll_threadpool/iobuffer.h>

#include "util/bloomfilter.h"

namespace blockstore {

using std::string;
using std::tr1::function;
using std::tr1::shared_ptr;
using std::vector;
using epoll_threadpool::Future;
using epoll_threadpool::IOBuffer;
using util::BloomFilter;

using namespace std::tr1::placeholders;

namespace {
/**
 * Helper function that maps from vector<uint8_t> to IOBuffer*.
 */
Future<bool> FileBlockStorePutBlockHelper(
    FileBlockStore *bs, string name, vector<uint8_t> data) {
  LOG(INFO) << "FileBlockStorePutBlockHelper";
  IOBuffer* iobuffer = new IOBuffer((const char *)&data[0], data.size());
  return (*bs).putBlock(name, iobuffer);
}

void FileBlockStoreGetBlockHelperCallback(
    Future< vector<uint8_t> > dst, Future<IOBuffer*> src) {
  LOG(INFO) << "FileBlockStoreGetBlockHelperCallback with src " << src.get();
  if (src.get() == NULL) {
    dst.set(vector<uint8_t>());
  } else {
    vector<uint8_t> ret;
    ret.resize(src.get()->size());
    memcpy(&ret[0], src.get()->pulldown(src.get()->size()), src.get()->size());
    delete src.get();
    dst.set(ret);
  }
}

/**
 * Helper function that maps from vector<uint8_t> to IOBuffer*.
 */
Future< vector<uint8_t> > FileBlockStoreGetBlockHelper(
    FileBlockStore *bs, string name) {
  LOG(INFO) << "FileBlockStoreGetBlockHelper";
  Future< vector<uint8_t> > dst;
  Future<IOBuffer*> src = (*bs).getBlock(name);
  src.addCallback(bind(&FileBlockStoreGetBlockHelperCallback, dst, src));
  return dst;
}

/**
 * Helper function that maps from vector<uint8_t> to BloomFilter
 */
Future< vector<uint8_t> > FileBlockStoreBloomFilterHelper(FileBlockStore *bs) {
  LOG(INFO) << "FileBlockStoreBloomFilterHelper";
  Future< vector<uint8_t> > ret;
  ret.set((*bs).bloomfilter().get().serialize());
  return ret;
}

/**
 * Builds a BlockStore unique string so multiple BlockStore instances can be
 * run over a single RPC channel.
 */
string MakeString(const char *str, uint64_t bsid) {
  char buf[128];
  snprintf(buf, 127, "%s%lu", str, bsid);
  return buf;
}
} // end anonymous namespace

/**
 * Registers a BlockStore with an RPC server instance.
 * This is intended to be used in conjunction with RemoteBlockStore on the
 * client side. It creates per-blockstore unique RPC names to allow
 * multiple BlockStore instances to be shared on a single server endpoint.
 */
void RegisterRemoteBlockStore(shared_ptr<rpc::RPCServer> server,
                              FileBlockStore *blockstore, uint64_t bsid) {
  LOG(INFO) << "RegisterRemoteBlockStore";
  server->registerFunction<bool, string, vector<uint8_t> >(
      MakeString("putBlock", bsid), 
      bind(&FileBlockStorePutBlockHelper, blockstore, _1, _2));
  server->registerFunction<vector<uint8_t>, string>(
      MakeString("getBlock", bsid),
      bind(&FileBlockStoreGetBlockHelper, blockstore, _1));
  server->registerFunction<bool, string>(
      MakeString("removeBlock", bsid),
      bind(&FileBlockStore::removeBlock, blockstore, _1));
  server->registerFunction<uint64_t>(
      MakeString("blockSize", bsid),
      bind(&FileBlockStore::blockSize, blockstore));
  server->registerFunction<uint64_t>(
      MakeString("numFreeBlocks", bsid),
      bind(&FileBlockStore::numFreeBlocks, blockstore));
  server->registerFunction<uint64_t>(
      MakeString("numTotalBlocks", bsid),
      bind(&FileBlockStore::numTotalBlocks, blockstore));
  server->registerFunction< vector<uint8_t> >(
      MakeString("bloomfilter", bsid),
      bind(&FileBlockStoreBloomFilterHelper, blockstore));
}

/**
 * An implementation of the BlockStore interface that operates via RPC on
 * a remote instance. Each RemoteBlockStore takes a bsid, allowing multiple
 * BlockStore's to be hosted on a single RPC channel.
 */
class RemoteBlockStore : public BlockStore {
 public:
  RemoteBlockStore(shared_ptr<rpc::RPCClient> client, uint64_t bsid)
      : _bsid(bsid), _client(client) { }
  virtual ~RemoteBlockStore() { }

  /**
   * Attempts to write a block to the block store.
   * @param key the key for this block
   * @param data the block of data up to "blocksize" long to write.
   * @returns true if successful, false if not.
   * @note Ownership of data is transfered to the function.
   */
  virtual Future<bool> putBlock(const string &key, IOBuffer *data) {
    vector<uint8_t> datavec;
    datavec.resize(data->size());
    memcpy(&datavec[0], data->pulldown(data->size()), data->size());
    delete data;
    return _client->call<bool, string, vector<uint8_t> >(
        MakeString("putBlock", _bsid), key, datavec);
  }

  /**
   * Attempts to read a block from the block store.
   * @param key the key for this block
   * @returns an IOBuffer containing the requested data or
   *        NULL if data not found.
   * @note Ownership of data is passed to the callback.
   */
  virtual Future<IOBuffer *> getBlock(const string &key) {
    Future<IOBuffer *> ret;
    Future< vector<uint8_t> > proxy_ret = 
        _client->call<vector<uint8_t>, string>(
            MakeString("getBlock", _bsid), key);
    proxy_ret.addCallback(bind(&getBlockHelper, proxy_ret, ret));
    return ret;
  }

  /**
   * Removes a previously stored block from disk.
   * @param key the key for this block
   * @returns true on success, false on failure.
   */
  virtual Future<bool> removeBlock(const string &key) {
    return _client->call<bool, const string &>(
        MakeString("removeBlock", _bsid), key);
  }

  /**
   * Gets the size of blocks on this device.
   * @returns the size of a block in bytes or -1 on error.
   */
  virtual Future<uint64_t> blockSize() const {
    return _client->call<uint64_t>(MakeString("blockSize", _bsid));
  }

  /**
   * Gets the free block availability of this device.
   * @returns the num free blocks or -1 on error.
   */
  virtual Future<uint64_t> numFreeBlocks() const {
    return _client->call<uint64_t>(MakeString("numFreeBlocks", _bsid));
  }

  /**
   * Gets the total number of blocks of storage in this device.
   * @returns total blocks on this device or -1 on error.
   */
  virtual Future<uint64_t> numTotalBlocks() const {
    return _client->call<uint64_t>(MakeString("numTotalBlocks", _bsid));
  }

  /**
   * Get a bloomfilter that remote hosts can use to try to determine if
   * we have a block or not before requesting it from us.
   * @return BloomFilter
   */
  virtual Future<BloomFilter> bloomfilter() {
    Future<BloomFilter> ret;
    Future< vector<uint8_t> > proxy_ret = 
        _client->call< vector<uint8_t> >(MakeString("bloomfilter", _bsid));
    proxy_ret.addCallback(bind(&bloomfilterHelper, proxy_ret, ret));
    return ret;
  }

 private:
  uint64_t _bsid;
  shared_ptr<rpc::RPCClient> _client;

  /**
   * Helper function to map from vector to IOBuffer *
   */
  static void getBlockHelper(
      Future< vector<uint8_t> > proxy_ret, Future<IOBuffer *> ret) {
    LOG(INFO) << "Attempting deserialization";
    if (proxy_ret.get().size() == 0) {
      ret.set(NULL);
    } else {
      IOBuffer *buf = new IOBuffer((const char *)(&proxy_ret.get()[0]), 
                                   proxy_ret.get().size());
      ret.set(buf);
    }
  }

  /**
   * Helper function to map from vector to BloomFilter
   */
  static void bloomfilterHelper(
      Future< vector<uint8_t> > proxy_ret, Future<BloomFilter> ret) {
    LOG(INFO) << "Attempting deserialization";
    ret.set(BloomFilter());
    return;
    if (proxy_ret.get().size() == 0) {
      LOG(ERROR) << "Invalid BloomFilter response.";
      ret.set(BloomFilter());
    } else {
      BloomFilter bf;
      bf.deserialize(proxy_ret.get());
      ret.set(bf);
    }
  }

};
}
#endif

