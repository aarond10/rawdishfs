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
#include "lrucache.h"

#include <gtest/gtest.h>
#include <vector>

TEST(LRUCacheTest, BasicTests) {
  util::LRUCache lru1(3);
  std::vector<uint8_t> buf1, buf2;

  buf1.resize(10);
  strcpy(reinterpret_cast<char *>(&buf1[0]), "012345678");

  lru1.put("a", buf1);
  lru1.put("b", buf1);
  lru1.put("b", buf1);
  lru1.put("b", buf1);
  lru1.put("c", buf1);

  EXPECT_TRUE(lru1.get("a"));
  EXPECT_TRUE(lru1.get("b"));
  EXPECT_TRUE(lru1.get("c"));
  EXPECT_TRUE(!lru1.get("d"));
  lru1.put("d", buf1);
  EXPECT_TRUE(lru1.get("d"));
  EXPECT_TRUE(!lru1.get("a"));
  EXPECT_TRUE(lru1.get("b"));
  lru1.put("a", buf1);
  EXPECT_TRUE(lru1.get("a"));
  EXPECT_TRUE(lru1.get("b"));
  EXPECT_TRUE(!lru1.get("c"));
  EXPECT_TRUE(lru1.get("d"));
  lru1.invalidate("d");
  EXPECT_TRUE(!lru1.get("d"));
}
