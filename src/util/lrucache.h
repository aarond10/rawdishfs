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
#ifndef _UTIL_LRUCACHE_H_
#define _UTIL_LRUCACHE_H_

#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>
#include <tr1/unordered_map>

namespace util {

using std::string;
using std::make_heap;
using std::push_heap;
using std::pop_heap;
using std::tr1::unordered_map;
using std::vector;

/**
 * LRU cache for arbitrary length byte vectors keyed by string.
 * TODO(aarond10): Remove use of make_heap. May require rewrite of heap code.
 */
class LRUCache {
 public:
  explicit LRUCache(int size = 64) : _size(size), _timeCnt(1) {}
  virtual ~LRUCache() {
    while (_heap.size()) {
      delete _heap.back();
      _heap.pop_back();
    }
    _map.clear();
  }

  /**
   * Removes an item from the cache if present.
   */
  void invalidate(const string &key) {
    if (_map.find(key) != _map.end()) {
      BlockCacheData* b = _map[key];
      _map.erase(key);
      b->atime = 0;
      // TODO(aarond10): Find a more efficient way to fix the heap?
      make_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      pop_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      _heap.pop_back();
      delete b;
    }
  }

  /**
   * Gets an item from the cache or returns NULL.
   */
  const vector<uint8_t>* get(const string &key) {
    unordered_map<string, BlockCacheData*>::iterator i = _map.find(key);
    if (i != _map.end()) {
      i->second->atime = _timeCnt++;
      // TODO(aarond10): Find a more efficient way to fix the heap?
      make_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      return &i->second->data;
    } else {
      return NULL;
    }
  }

  /**
   * Adds an item to the cache, potentially invalidating
   * an older item in the process.
   */
  void put(const string &key, const vector<uint8_t> &data) {
    // Replacing an element already in the cache?
    if (_map.find(key) != _map.end()) {
      _map[key]->atime = _timeCnt++;
      _map[key]->data = data;
      return;
    } else {
      BlockCacheData* b = new BlockCacheData();
      b->key = key;
      b->data = data;
      b->atime = _timeCnt++;
      _heap.push_back(b);
      push_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      _map[key] = b;

      if (_heap.size() > _size) {
        pop_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
        BlockCacheData* b = _heap.back();
        _heap.pop_back();
        _map.erase(b->key);
        delete b;
      }
    }
  }

 private:
  struct BlockCacheData {
    string key;
    vector<uint8_t> data;
    uint64_t atime;
  };
  static bool Compare(BlockCacheData* a, BlockCacheData* b) {
    return a->atime > b->atime;
  }

  size_t _size;
  vector<BlockCacheData*> _heap;
  unordered_map< string, BlockCacheData* > _map;
  uint64_t _timeCnt;
};
}

#endif
