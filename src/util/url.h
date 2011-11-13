#ifndef UTIL_URL_H_
#define UTIL_URL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string>
using namespace std;

namespace util {

/**
 * Provides very basic string<-->URL conversion and manipulation of
 * individual URL components.
 */
class URL {
 private:
  string _scheme;
  string _host;
  int    _port;
  string _path;
  bool   _valid;

 public:
  URL() {}
  URL(const string& s) { parse(s); }
  URL(const URL& u) { *this = u; }
  virtual ~URL() {}

  URL& operator=(const URL &other) {
    _scheme = other._scheme;
    _host = other._host;
    _port = other._port;
    _path = other._path;
    _valid = other._valid;
    return *this;
  }

  bool operator==(const URL &other) const {
    return this->toString() == other.toString();
  }

  void parse(string s) {
    _valid = true;
    if (s.find("://") != string::npos) {
      _scheme = s.substr(0, s.find("://"));
      s = s.substr(s.find("://")+3, string::npos);
    } else {
      _scheme = "http";
    }
    if (s.find("/") != string::npos) {
      _path = s.substr(s.find("/")+1, string::npos);
      s = s.substr(0, s.find("/"));
    } else {
      _path = "";
    }
    if (s.rfind(":") != string::npos) {
      _port = atoi(s.substr(s.rfind(":")+1, string::npos).c_str());
      if (_port <= 0 || _port > 65535) {
        _valid = false;
      }
      s = s.substr(0, s.rfind(":"));
    } else {
      _port = 80;
    }
    if (s.find(":") != string::npos) {
        _valid = false;
    }
    _host = s;
  }

  const string toString() const {
    char buf[32];
    buf[31] = 0;
    snprintf(buf, sizeof(buf)-1, "%d", _port);

    return _scheme + "://" + _host +
        ((_scheme == "http" && _port == 80) ? "" : (string(":") + buf)) + "/" +
        _path;
  }
  operator string() const { return this->toString(); }

  // Methods to access and/or change a URLs components
  const string& scheme() const { return _scheme; }
  string& scheme() { return _scheme; }
  const string& host() const { return _host; }
  string& host() { return _host; }
  const int port() const { return _port; }
  int& port() { return _port; }
  const string& path() const { return _path; }
  string& path() { return _path; }
  bool valid() const { return _valid; }

};
}

#endif
