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
#ifndef BLOCKSTORE_FILEBLOCKSTORE_H_
#define BLOCKSTORE_FILEBLOCKSTORE_H_

#include <stdint.h>
#include <dirent.h>

#include <list>
#include <string>
#include <set>
using namespace std;

#include "util/bloomfilter.h"

namespace blockstore {

/**
 * Stores raw, fixed size blocks as files on a disk. 
 * Blocks are keyed by ASCII string with no directory structure.
 * Intended to be used as a preliminary test version. More efficient methods
 * to follow.
 */
class FileBlockStore {
 public:
  FileBlockStore(const string &path, int blocksize=65536);
  virtual ~FileBlockStore() {
    if (_dir) {
      closedir(_dir);
      _dir = NULL;
    }
  }

  /**
   * Attempts to write a block to the block store.
   * @param key the key for this block
   * @param data the block of data "blocksize" long to write.
   * @return true if successful, false if not.
   */
  bool put(const string &key, const uint8_t *data);

  /**
   * Attempts to read a block from the block store.
   * @param key the key for this block
   * @param data pointer to target buffer of at least "blocksize" long.
   * @return true if successful, false if not found or error occurred.
   */
  bool get(const string &key, uint8_t *data);

  /**
   * Removes a previously stored block from disk.
   * @param key the key for this block
   * @return true if successful, false if not found or error occurred.
   */
  bool remove(const string &key);

  /**
   * Returns the size of a block in bytes.
   */
  uint32_t blockSize() const { return _blocksize; }

  /**
   * Gets the free block availability of this device.
   * @return the number of free "blocksize" blocks that can fit on this device.
   */
  uint32_t numFreeBlocks() const { return _freeBlocks; }

  /**
   * Gets the total number of blocks of storage in this device.
   * @return the total number of blocks that can fit on this device.
   */
  uint32_t numTotalBlocks() const { return _usedBlocks + _freeBlocks; }

  /**
   * Get a bloomfilter that remote hosts can use to try to determine if
   * we have a block or not before requesting it from us.
   * @return BloomFilter
   */
  const util::BloomFilter &bloomfilter() const { return _bloomfilter; }

  /**
   * Iterates through block in the store, reading them one at a time.
   * Returns an empty string when complete and auto-resets.
   */
  string next();

 private:
  /**
   * Reads through all files on disk and regenerates bloom filter
   * and block set used to speed up queries.
   */
  void regenerateBloomFilterAndBlockSet();
 
  DIR *_dir;
  int _blocksize;
  string _path;
  util::BloomFilter _bloomfilter;
  set<string> _blockset;
  uint32_t _freeBlocks;
  uint32_t _usedBlocks;
};
}

#endif
