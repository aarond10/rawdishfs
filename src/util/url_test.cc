#include "url.h"

#include <gtest/gtest.h>

TEST(URLTest, BasicTests) {
  struct TestCase {
    string in;
    string out;
    string scheme;
    string host;
    int port;
    string path;
    bool valid;
  };

  TestCase testcases[] = {
    {"somestring", "http://somestring/", "http", "somestring", 80, "", true},
    {"http://somestring:80", "http://somestring/", "http", "somestring", 80, "", true},
    {"http://somestring:80/", "http://somestring/", "http", "somestring", 80, "", true},

    {"rawdish://somestring", "rawdish://somestring:80/", "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring/", "rawdish://somestring:80/", "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring:80", "rawdish://somestring:80/", "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring:80/", "rawdish://somestring:80/", "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring:90/", "rawdish://somestring:90/", "rawdish", "somestring", 90, "", true},
    {"rawdish://somestring:90/path", "rawdish://somestring:90/path", "rawdish", "somestring", 90, "path", true},
    {"rawdish://somestring:90/path/path", "rawdish://somestring:90/path/path", "rawdish", "somestring", 90, "path/path", true},
    {"rawdish://bad:host:90/path/path", "rawdish://bad:host:90/path/path", "rawdish", "bad:host", 90, "path/path", false},
    {"rawdish://somestring:0/path/path", "rawdish://somestring:0/path/path", "rawdish", "somestring", 0, "path/path", false},
    {"rawdish://somestring:-1/path/path", "rawdish://somestring:-1/path/path", "rawdish", "somestring", -1, "path/path", false},
    {"rawdish://somestring:70000/path/path", "rawdish://somestring:70000/path/path", "rawdish", "somestring", 70000, "path/path", false},
    {"rawdish://somestring:badport/path/path", "rawdish://somestring:0/path/path", "rawdish", "somestring", 0, "path/path", false}
  };

  util::URL url("http://somesite.com/path");
  util::URL url2(url);
  util::URL url3;
  url3 = url;

  EXPECT_EQ(url, url2);
  EXPECT_EQ(url, url3);

  for (size_t i = 0; i < sizeof(testcases)/sizeof(*testcases); ++i) {
    //printf("Processing: %s\n", testcases[i].out.c_str());
    util::URL url(testcases[i].in);
    EXPECT_EQ(url.toString(), testcases[i].out);
    EXPECT_EQ(url.scheme(), testcases[i].scheme);
    EXPECT_EQ(url.host(), testcases[i].host);
    EXPECT_EQ(url.port(), testcases[i].port);
    EXPECT_EQ(url.path(), testcases[i].path);
    EXPECT_EQ(url.valid(), testcases[i].valid);
  }
}

