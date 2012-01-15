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

#include <epoll_threadpool/iobuffer.h>

#include <set>
#include <string>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

using epoll_threadpool::IOBuffer;
using std::set;
using std::string;

TEST(FileBlockStoreTest, BasicTests) {

  mkdir("/tmp/bs1", 0777);

  char buf1[16];
  IOBuffer *buf;
  memset(buf1, 0, sizeof(buf1));
  blockstore::FileBlockStore bs1("/tmp/bs1", 16);

  EXPECT_TRUE(bs1.getBlock("apple").get() == NULL);
  strcpy((char *)buf1, "apple");
  EXPECT_TRUE(bs1.putBlock("apple", new IOBuffer(buf1, 16)));
  //sync();
  buf = bs1.getBlock("apple");
  EXPECT_TRUE(buf != NULL);
  delete buf;

  uint32_t blocks_a = bs1.numTotalBlocks();
  uint32_t blocks_a2 = bs1.numFreeBlocks();

  EXPECT_TRUE(bs1.getBlock("banana").get() == NULL);
  strcpy((char *)buf1, "banana");
  EXPECT_TRUE(bs1.putBlock("banana", new IOBuffer(buf1, 16)));
  //sync();
  buf = bs1.getBlock("banana");
  EXPECT_TRUE(buf != NULL);
  EXPECT_TRUE(strcmp("banana",(char *)buf->pulldown(buf->size()))==0);
  delete buf;

  buf = bs1.getBlock("apple");
  EXPECT_TRUE(buf != NULL);
  EXPECT_TRUE(strcmp("apple",(char *)buf->pulldown(buf->size()))==0);
  delete buf;

  uint32_t blocks_b = bs1.numTotalBlocks();
  uint32_t blocks_b2 = bs1.numFreeBlocks();
  EXPECT_TRUE(blocks_a==blocks_b);
  EXPECT_TRUE(blocks_a2==(blocks_b2+1));

  set<string> expected_blocks;
  expected_blocks.insert("apple");
  expected_blocks.insert("banana");
  string key;
  while ((key = bs1.next()) != "") {
    EXPECT_TRUE(expected_blocks.find(key) != expected_blocks.end());
    expected_blocks.erase(key);
  }
  EXPECT_EQ(0, expected_blocks.size());
 
  EXPECT_TRUE(bs1.bloomfilter().get().mayContain("banana"));
  EXPECT_FALSE(bs1.bloomfilter().get().mayContain("carrot"));

  EXPECT_TRUE(bs1.removeBlock("apple"));
  EXPECT_TRUE(bs1.removeBlock("banana"));

  EXPECT_EQ(string(""), bs1.next());

  EXPECT_FALSE(bs1.bloomfilter().get().mayContain("apple"));
  EXPECT_FALSE(bs1.bloomfilter().get().mayContain("banana"));
}
