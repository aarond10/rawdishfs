#ifndef UTIL_LRUCACHE_H_
#define UTIL_LRUCACHE_H_

#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>
#include <tr1/unordered_map>
using namespace std;
using namespace std::tr1;

namespace util {

/**
 * LRU cache for arbitrary length byte vectors keyed by string.
 * TODO: Remove use of make_heap. May require rewrite of heap code.
 */
class LRUCache {
 public:
  LRUCache(int size=64) : _size(size), _timeCnt(1) {}
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
      BlockCacheData *b = _map[key];
      _map.erase(key);
      b->atime = 0;
      // TODO: Find a more efficient way to fix the heap?
      make_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      pop_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      _heap.pop_back();
      delete b;
    }
  }

  /**
   * Gets an item from the cache or returns NULL.
   */
  const vector<uint8_t> *get(const string &key) {
    unordered_map<string,BlockCacheData*>::iterator i = _map.find(key);
    if (i != _map.end()) {
      i->second->atime = _timeCnt++;
      // TODO: Find a more efficient way to fix the heap?
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
      BlockCacheData *b = new BlockCacheData();
      b->key = key;
      b->data = data;
      b->atime = _timeCnt++;
      _heap.push_back(b);
      push_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
      _map[key] = b;

      if (_heap.size() > _size) {
        pop_heap(_heap.begin(), _heap.end(), &LRUCache::Compare);
        BlockCacheData *b = _heap.back();
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
  static bool Compare(BlockCacheData *a, BlockCacheData *b) {
    return a->atime > b->atime;
  }

  size_t _size;
  vector<BlockCacheData *> _heap;
  unordered_map< string, BlockCacheData* > _map;
  uint64_t _timeCnt;
};
}

#endif
