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
#include <stdio.h>
#include <sys/time.h>

#include "rpc/rpc.h"
#include "util/url.h"

int main(int argc, char *argv[]) {

/*  int opt;
  struct option long_opts[] = {
    {"get", required_argument, 0, 0},
    {"put", required_argument, 0, 0},
    {"remove", required_argument, 0, 0},
    {"addpeer", required_argument, 0, 0},
    {NULL, 0, 0, 0}
  };
  while ((opt = getopt_long_only(argc, argv, long_opts, &option_index);
  // TODO: Switch to getopts*/
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <host> <port> [get|put|remove|addpeer] ...\n", argv[0]);
    return 1;
  }

  string host = argv[1];
  uint16_t port = atoi(argv[2]);
  string cmd = argv[3];

  rpc::RPCClient client(rpc::TcpSocket::connect(host, port));

  if (cmd == "get") {
    if (argc < 6) {
      fprintf(stderr, "Usage: %s <host> <port> get <src_key> <dst_filename>\n", argv[0]);
      return 1;
    }
    string key = argv[4];
    string dest = argv[5];
    string ref = client.call("get", key).get<msgpack::type::raw_ref>().str();
    FILE *f = fopen(dest.c_str(), "wb");
    fwrite(ref.c_str(), 1, ref.size(), f);
    fclose(f);
  } else if (cmd == "put") {
    if (argc < 6) {
      fprintf(stderr, "Usage: %s <host> <port> put <src_filename> <dst_key>\n", argv[0]);
      return 1;
    }
    string src = argv[4];
    string key = argv[5];
    FILE *f = fopen(src.c_str(), "rb");
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    fprintf(stderr, "Uploading file of size: %d\n", s);
    fseek(f, 0, SEEK_SET);
    vector<uint8_t> data;
    data.resize(s);
    fread(&data[0], 1, s, f);
    fclose(f);
    fprintf(stderr, "Returned: %d\n", client.call("put", key, msgpack::type::raw_ref((char *)&data[0], s)).get<bool>());
  } else if (cmd == "remove") {
    if (argc < 5) {
      fprintf(stderr, "Usage: %s <host> <port> remove <key>\n", argv[0]);
      return 1;
    }
    string key = argv[4];
    fprintf(stderr, "Returned: %d\n", client.call("remove", key).get<bool>());
  } else if (cmd == "addpeer") {
    if (argc < 6) {
      fprintf(stderr, "Usage: %s <host> <port> addpeer <host_b> <port_b>\n", argv[0]);
      return 1;
    }
    string hostb = argv[4];
    uint16_t portb = atoi(argv[5]);
    fprintf(stderr, "Returned: %d\n", client.call("addpeer", hostb, portb).get<bool>());
  } else {
    fprintf(stderr, "Unknown command %s\n", cmd.c_str());
    return 1;
  }

  return 0;
}


