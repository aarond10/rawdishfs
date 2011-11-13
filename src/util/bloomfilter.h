#ifndef UTIL_BLOOMFILTER_H_
#define UTIL_BLOOMFILTER_H_

#include <stdint.h>

#include <string>
#include <vector>
using namespace std;

namespace util {

class BloomFilter {
 public:
  BloomFilter();
  virtual ~BloomFilter();

  /**
   * Creates a bloom filter of a given size.
   * If seed is given, this is used to mutate the hash
   * function to prevent the same false positives from
   * appearing in different filters.
   */
  void reset(int size=(1 << 20), uint32_t seed=0);

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
