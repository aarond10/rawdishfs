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
#include "url.h"

#include <gtest/gtest.h>
#include <string>

TEST(URLTest, BasicTests) {
  struct TestCase {
    std::string in;
    std::string out;
    std::string scheme;
    std::string host;
    int port;
    std::string path;
    bool valid;
  };

  TestCase testcases[] = {
    {"somestring", "http://somestring/",
      "http", "somestring", 80, "", true},
    {"http://somestring:80", "http://somestring/",
      "http", "somestring", 80, "", true},
    {"http://somestring:80/", "http://somestring/",
      "http", "somestring", 80, "", true},

    {"rawdish://somestring", "rawdish://somestring:80/",
      "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring/", "rawdish://somestring:80/",
      "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring:80", "rawdish://somestring:80/",
      "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring:80/", "rawdish://somestring:80/",
      "rawdish", "somestring", 80, "", true},
    {"rawdish://somestring:90/", "rawdish://somestring:90/",
      "rawdish", "somestring", 90, "", true},
    {"rawdish://somestring:90/path", "rawdish://somestring:90/path",
      "rawdish", "somestring", 90, "path", true},
    {"rawdish://somestring:90/path/path", "rawdish://somestring:90/path/path",
      "rawdish", "somestring", 90, "path/path", true},
    {"rawdish://bad:host:90/path/path", "rawdish://bad:host:90/path/path",
      "rawdish", "bad:host", 90, "path/path", false},
    {"rawdish://somestring:0/path/path", "rawdish://somestring:0/path/path",
      "rawdish", "somestring", 0, "path/path", false},
    {"rawdish://somestring:-1/path/path", "rawdish://somestring:-1/path/path",
      "rawdish", "somestring", -1, "path/path", false},
    {"rawdish://somestring:70000/path/path",
      "rawdish://somestring:70000/path/path", "rawdish", "somestring",
      70000, "path/path", false},
    {"rawdish://somestring:badport/path/path",
      "rawdish://somestring:0/path/path", "rawdish", "somestring",
      0, "path/path", false}
  };

  util::URL url("http://somesite.com/path");
  util::URL url2(url);
  util::URL url3;
  url3 = url;

  EXPECT_EQ(url, url2);
  EXPECT_EQ(url, url3);

  for (size_t i = 0; i < sizeof(testcases)/sizeof(*testcases); ++i) {
    util::URL url(testcases[i].in);
    EXPECT_EQ(url.toString(), testcases[i].out);
    EXPECT_EQ(url.scheme(), testcases[i].scheme);
    EXPECT_EQ(url.host(), testcases[i].host);
    EXPECT_EQ(url.port(), testcases[i].port);
    EXPECT_EQ(url.path(), testcases[i].path);
    EXPECT_EQ(url.valid(), testcases[i].valid);
  }
}
