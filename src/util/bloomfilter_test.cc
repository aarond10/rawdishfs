#include "bloomfilter.h"

#include <gtest/gtest.h>

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

TEST(BloomFilter, FalseNegatives) {
  util::BloomFilter bf1;

  bf1.set("apple");
  bf1.set("banana");
  bf1.set("carrot");

  EXPECT_TRUE(bf1.mayContain("apple"));
  EXPECT_TRUE(bf1.mayContain("banana"));
  EXPECT_TRUE(bf1.mayContain("carrot"));
}

TEST(BloomFilter, FalsePositives) {
  util::BloomFilter bf1;

  bf1.set("dumplings");
  bf1.set("eggs");
  bf1.set("fish");

  // Note: It *is* possible for false positives to occur but the probability is
  // extremely low. This test is perhaps best done statistically?
  EXPECT_TRUE(!bf1.mayContain("apple"));
  EXPECT_TRUE(!bf1.mayContain("banana"));
  EXPECT_TRUE(!bf1.mayContain("carrot"));
}

TEST(BloomFilter, SerializeDeserialize) {
  vector<uint8_t> buf;
  util::BloomFilter bf1, bf2;

  bf1.set("apple");
  bf1.set("banana");
  bf1.set("carrot");
  buf = bf1.serialize();
  //printf("Bloomfilter serialized to %d bytes\n", (int)buf.size());
  bf2.deserialize(buf);
  EXPECT_TRUE(bf2.mayContain("apple"));
  EXPECT_TRUE(bf2.mayContain("banana"));
  EXPECT_TRUE(bf2.mayContain("carrot"));
}

TEST(BloomFilter, SerializeDeserializeTruncated) {
  vector<uint8_t> buf;
  util::BloomFilter bf1, bf2;

  bf1.set("apple");
  bf1.set("banana");
  bf1.set("carrot");
  buf = bf1.serialize();
  buf.resize(buf.size() - 4);
  bf2.deserialize(buf);
  EXPECT_FALSE(bf2.mayContain("apple"));
  EXPECT_FALSE(bf2.mayContain("banana"));
  EXPECT_FALSE(bf2.mayContain("carrot"));
}

TEST(BloomFilter, Reset) {
  util::BloomFilter bf1;

  bf1.set("apple");
  bf1.set("banana");
  bf1.set("carrot");
  bf1.reset();
  EXPECT_TRUE(!bf1.mayContain("apple"));
  EXPECT_TRUE(!bf1.mayContain("banana"));
  EXPECT_TRUE(!bf1.mayContain("carrot"));
}

TEST(BloomFilter, BadData) {
  util::BloomFilter bf1;
  vector<uint8_t> baddata;
  bf1.set("apple");

  // Very short buffer.
  for(int i = 0; i < 3; i++) {
    baddata.push_back(rand());
  }
  bf1.deserialize(baddata);
  EXPECT_TRUE(bf1.mayContain("apple"));  // No change. (TODO: Report error?)
  EXPECT_TRUE(!bf1.mayContain("banana"));

  // Longer buffer.
  for(int i = 0; i < 5000; i++) {
    baddata.push_back(rand());
  }
  bf1.deserialize(baddata);
  EXPECT_TRUE(bf1.mayContain("apple"));  // No change. (TODO: Report error?)
  EXPECT_TRUE(!bf1.mayContain("banana"));
}

TEST(BloomFilter, Benchmark) {
  // TODO: Move this to a separate program
  util::BloomFilter bf1;
  //struct timeval start, stop;

  //gettimeofday(&start, 0);
  for (int i = 0; i < 300000; i++) {
    bf1.set("apple");
    bf1.set("banana");
    bf1.set("carrot");
  }
  //gettimeofday(&stop, 0);
  //printf("Took %ldms to add 300000 items to filter.\n", (stop.tv_sec - start.tv_sec)*1000 + (stop.tv_usec - start.tv_usec)/1000);
}

