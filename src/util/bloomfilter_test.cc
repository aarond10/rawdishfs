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

