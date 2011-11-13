#include "bloomfilter.h"

#include <stdio.h>
#include <string.h>

namespace util {

namespace {

/**
 * Helper function that hashes a string given some seed for the hash function.
 * The algorithm here is FNV-1a hash algorithm
 */
uint64_t hash(const char *str, int seed) {
  const uint64_t FNV_offset_basis = 14695981039346656037UL;
  const uint64_t FNV_prime = 1099511628211;
  uint64_t hash = FNV_offset_basis;
  // seed
  for (int i = 0;i < 4; i++) {
    hash = hash ^ (seed & 0xff);
    hash *= FNV_prime;
    seed >>= 8;
  }
  // str
  while(*str) {
    hash = hash ^ (uint8_t)*(str++);
    hash *= FNV_prime;
  }
  return hash;
}
}

BloomFilter::BloomFilter() {
  _hash = NULL;
  _size = _seed = 0;
  reset();
}

BloomFilter::~BloomFilter() {
  if (_hash) {
    delete [] _hash;
  }
}

void BloomFilter::reset(int size, uint32_t seed) {
  if (_hash) {
    delete [] _hash;
  }
  _size = size;
  _hash = new uint8_t[(size+7)/8];
  memset(_hash, 0, (size+7)/8);
  _seed = seed;
}

void BloomFilter::set(const string &key) {
  for (int i = 0; i < 6; i++) {
    uint32_t p = hash(key.c_str(), _seed + i) % _size;
    _hash[p>>3] |= 1 << (p & 0x7); 
  }
}

bool BloomFilter::mayContain(const string &key) const {
  for (int i = 0; i < 6; i++) {
    uint32_t p = hash(key.c_str(), _seed + i) % _size;
    if(!(_hash[p>>3] & 1 << (p & 0x7))) {
      return false;
    }
  }
  return true;
}

vector<uint8_t> BloomFilter::serialize() const {
  vector<uint8_t> ret(((_size+7)/8)+sizeof(uint32_t)+sizeof(uint32_t), 0);
  *(uint32_t *)(&ret[0]) = _seed;
  *(uint32_t *)(&ret[sizeof(uint32_t)]) = _size;
  memcpy(&ret[sizeof(uint32_t)*2], _hash, (_size+7)/8);
  return ret;
}

BloomFilter &BloomFilter::deserialize(const vector<uint8_t> &src) {
  if (src.size() < 8) {
    fprintf(stderr, "Invalid serialized bloomfilter of length: %d\n", src.size());
    return *this;
  }
  uint32_t seed = *(uint32_t *)(&src[0]);
  uint32_t size = *(uint32_t *)(&src[sizeof(uint32_t)]);
  if (size < 0 || size > (100 * 1024 * 1024)) {
    fprintf(stderr, "Invalid serialized bloomfilter of reported size: %d\n", size);
    return *this;
  }
  if (src.size() != (((size+7)/8)+sizeof(uint32_t)+sizeof(uint32_t))) {
    fprintf(stderr, "Size of vector doesn't match expected size (%d != %d).\n", 
        src.size(), (((size+7)/8)+sizeof(uint32_t)+sizeof(uint32_t)));
    return *this;
  }
  _seed = seed;
  _size = size;
  memcpy(_hash, &src[sizeof(uint32_t)*2], (_size+7)/8);
  return *this;
}

}
