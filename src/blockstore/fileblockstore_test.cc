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
#include "fileblockstore.h"
#include "util/bloomfilter.h"

#include <gtest/gtest.h>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

TEST(FileBlockStoreTest, BasicTests) {

  mkdir("/tmp/bs1", 0777);

  uint8_t buf1[16];
  memset(buf1, 0, sizeof(buf1));
  blockstore::FileBlockStore bs1("/tmp/bs1", 16);

  EXPECT_TRUE(!bs1.get("apple", buf1));
  strcpy((char *)buf1, "apple");
  EXPECT_TRUE(bs1.put("apple", buf1));
  sync();
  EXPECT_TRUE(bs1.get("apple", buf1));

  uint32_t blocks_a = bs1.numTotalBlocks();
  uint32_t blocks_a2 = bs1.numFreeBlocks();

  EXPECT_TRUE(!bs1.get("banana", buf1));
  strcpy((char *)buf1, "banana");
  EXPECT_TRUE(bs1.put("banana", buf1));
  EXPECT_TRUE(bs1.get("banana", buf1));
  EXPECT_TRUE(strcmp("banana",(char *)buf1)==0);

  EXPECT_TRUE(bs1.get("apple", buf1));
  EXPECT_TRUE(strcmp("apple",(char *)buf1)==0);

  uint32_t blocks_b = bs1.numTotalBlocks();
  uint32_t blocks_b2 = bs1.numFreeBlocks();
  EXPECT_TRUE(blocks_a==blocks_b);
  EXPECT_TRUE(blocks_a2==(blocks_b2+1));

  EXPECT_EQ(string("banana"), bs1.next());
  EXPECT_EQ(string("apple"), bs1.next());
  EXPECT_EQ(string(""), bs1.next());
 
  EXPECT_TRUE(bs1.bloomfilter().mayContain("banana"));
  EXPECT_TRUE(!bs1.bloomfilter().mayContain("carrot"));

  EXPECT_TRUE(bs1.remove("apple"));
  EXPECT_TRUE(bs1.remove("banana"));

  EXPECT_EQ(string(""), bs1.next());

  EXPECT_TRUE(!bs1.bloomfilter().mayContain("apple"));
  EXPECT_TRUE(!bs1.bloomfilter().mayContain("banana"));
}
