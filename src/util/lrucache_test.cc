#include "lrucache.h"

#include <gtest/gtest.h>

TEST(LRUCacheTest, BasicTests) {
  util::LRUCache lru1(3);
  vector<uint8_t> buf1 ,buf2;

  buf1.resize(10);
  strcpy((char *)&buf1[0], "012345678");

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
