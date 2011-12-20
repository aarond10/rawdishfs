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
#ifndef _BLOCKSTORE_BLOCKSTORE_H_
#define _BLOCKSTORE_BLOCKSTORE_H_

#include <stdint.h>

#include <string>
#include <tr1/functional>

#include <epoll_threadpool/future.h>
#include <epoll_threadpool/iobuffer.h>

#include "util/bloomfilter.h"

namespace blockstore {

using std::string;
using std::tr1::function;
using epoll_threadpool::Future;
using epoll_threadpool::IOBuffer;
using util::BloomFilter;

/**
 * Interface for BlockStore. Used by both concrete classes and proxy objects
 * that operate over the network.
 */
class BlockStore {
 public:
  virtual ~BlockStore() { }

  /**
   * Attempts to write a block to the block store.
   * @param key the key for this block
   * @param data the block of data up to "blocksize" long to write.
   * @returns true if successful, false if not.
   * @note Ownership of data is transfered to the function.
   */
  virtual Future<bool> putBlock(const string &key, IOBuffer *data) = 0;

  /**
   * Attempts to read a block from the block store.
   * @param key the key for this block
   * @returns an IOBuffer containing the requested data or
   *        NULL if data not found.
   * @note Ownership of data is passed to the callback.
   */
  virtual Future<IOBuffer *> getBlock(const string &key) = 0;

  /**
   * Removes a previously stored block from disk.
   * @param key the key for this block
   * @returns true on success, false on failure.
   */
  virtual Future<bool> removeBlock(const string &key) = 0;

  /**
   * Gets the size of blocks on this device.
   * @returns the size of a block in bytes or -1 on error.
   */
  virtual Future<uint64_t> blockSize() const = 0;

  /**
   * Gets the free block availability of this device.
   * @returns the num free blocks or -1 on error.
   */
  virtual Future<uint64_t> numFreeBlocks() const = 0;

  /**
   * Gets the total number of blocks of storage in this device.
   * @returns total blocks on this device or -1 on error.
   */
  virtual Future<uint64_t> numTotalBlocks() const = 0;

  /**
   * Get a bloomfilter that remote hosts can use to try to determine if
   * we have a block or not before requesting it from us.
   * @return BloomFilter
   */
  virtual Future<BloomFilter> bloomfilter() = 0;
};
}
#endif
