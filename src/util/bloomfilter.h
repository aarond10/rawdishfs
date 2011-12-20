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
#ifndef _UTIL_BLOOMFILTER_H_
#define _UTIL_BLOOMFILTER_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace util {

using std::string;
using std::vector;

class BloomFilter {
 public:
  BloomFilter();
  BloomFilter(const BloomFilter& other);
  BloomFilter& operator=(const BloomFilter& other);
  virtual ~BloomFilter();

  /**
   * Creates a bloom filter of a given size.
   * If seed is given, this is used to mutate the hash
   * function to prevent the same false positives from
   * appearing in different filters.
   */
  void reset(int size = (1 << 20), uint32_t seed = 0);

  /**
   * Sets a value in the bloom filter
   */
  void set(const string &key);

  /**
   * Checks a value in our bloom filter.
   * Returns false if the value is not found, true if it MIGHT
   * be in the set.
   */
  bool mayContain(const string &key) const;

  /**
   * Serialises this bloom filter down to a byte array.
   */
  vector<uint8_t> serialize() const;

  /**
   * Deserialises a serialised bloom filter.
   */
  BloomFilter &deserialize(const vector<uint8_t> &src);

 private:
  uint32_t _seed;
  uint32_t _size;
  uint8_t *_hash;
};
}
#endif
