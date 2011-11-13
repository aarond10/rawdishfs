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

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

namespace blockstore {

namespace {
/**
 * Combines a path with a key to generate a filename for a given block key.
 */
string get_fullpath(const string &path, const string &key) {
  string fullpath = path;
  if (fullpath[fullpath.size()-1] != '/')
    fullpath += "/";
  fullpath += key;
  return fullpath;
}
}

FileBlockStore::FileBlockStore(const string &path, int blocksize) {
  _dir = NULL;
  _blocksize = blocksize;
  _path = path;
  regenerateBloomFilterAndBlockSet();
}

bool FileBlockStore::put(const string &key, const uint8_t *data) {
  if(_freeBlocks <= 0) {
    return false;
  }
  int fd = open(get_fullpath(_path, key).c_str(), O_CREAT|O_TRUNC|O_WRONLY, 0777);
  if (fd == -1) {
    return false;
  }
  if (write(fd, data, _blocksize) != _blocksize) {
    return false;
  }
  fsync(fd);
  close(fd);
  _freeBlocks--;
  _usedBlocks++;
  _bloomfilter.set(key);
  _blockset.insert(key);
  return true;
}

bool FileBlockStore::get(const string &key, uint8_t *data) {
  int fd = open(get_fullpath(_path, key).c_str(), O_RDONLY);
  if (fd == -1) {
    return false;
  }
  if (read(fd, data, _blocksize) != _blocksize) {
    return false;
  }
  close(fd);
  return true;
}

bool FileBlockStore::remove(const string &key) {
  int r = unlink(get_fullpath(_path, key).c_str());
  if (r == 0) {
    //_freeBlocks++;
    regenerateBloomFilterAndBlockSet();
    return true;
  } else {
    return false;
  }
}

string FileBlockStore::next() {
  if (!_dir) {
    _dir = opendir(_path.c_str());
  }
  struct dirent *entry;
  while ((entry = readdir(_dir))) {
    if (entry->d_name[0] == '.' || entry->d_type != DT_REG) {
      continue;
    }
    return entry->d_name;
  }
  if (_dir) {
    closedir(_dir);
    _dir = NULL;
  }
  return "";
}

void FileBlockStore::regenerateBloomFilterAndBlockSet() {
  _bloomfilter.reset();
  _blockset.clear();
  _freeBlocks = 0;
  _usedBlocks = 0;

  struct statfs fs;
  if(!statfs(_path.c_str(), &fs)) {
    _freeBlocks = (fs.f_bavail*fs.f_bsize) / _blocksize;
  }

  DIR *d = opendir(_path.c_str());
  struct dirent *entry;
  if (!d) {
    return;
  }
  while ((entry = readdir(d))) {
    if (entry->d_name[0] == '.' || entry->d_type != DT_REG) {
      continue;
    }
    // TODO(aarond10): Check file length too? 
    _bloomfilter.set(entry->d_name);
    _blockset.insert(entry->d_name);
    _usedBlocks++;
  }
  closedir(d);
}
} 
