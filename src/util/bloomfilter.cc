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
#include "bloomfilter.h"

#include <glog/logging.h>

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
  for (int i = 0; i < 4; i++) {
    hash = hash ^ (seed & 0xff);
    hash *= FNV_prime;
    seed >>= 8;
  }
  // str
  while (*str) {
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
    if (!(_hash[p>>3] & 1 << (p & 0x7))) {
      return false;
    }
  }
  return true;
}

vector<uint8_t> BloomFilter::serialize() const {
  vector<uint8_t> ret(((_size+7)/8)+sizeof(uint32_t)+sizeof(uint32_t), 0);
  *reinterpret_cast<uint32_t *>(&ret[0]) = _seed;
  *reinterpret_cast<uint32_t *>(&ret[sizeof(uint32_t)]) = _size;
  memcpy(&ret[sizeof(uint32_t)*2], _hash, (_size+7)/8);
  return ret;
}

BloomFilter &BloomFilter::deserialize(const vector<uint8_t> &src) {
  if (src.size() < 8) {
    LOG(INFO) << "Invalid serialized bloomfilter of length: " << src.size();
    return *this;
  }
  uint32_t seed = *reinterpret_cast<const uint32_t *>(&src[0]);
  uint32_t size = *reinterpret_cast<const uint32_t *>(&src[sizeof(uint32_t)]);
  if (size < 0 || size > (100 * 1024 * 1024)) {
    LOG(INFO) << "Invalid serialized bloomfilter of reported size: " << size;
    return *this;
  }
  if (src.size() != (((size+7)/8)+sizeof(uint32_t)+sizeof(uint32_t))) {
    LOG(INFO) << "Size of vector doesn't match expected size (" << src.size()
        << " != " << (((size+7)/8)+sizeof(uint32_t)+sizeof(uint32_t)) << ")";
    return *this;
  }
  _seed = seed;
  _size = size;
  memcpy(_hash, &src[sizeof(uint32_t)*2], (_size+7)/8);
  return *this;
}
}
