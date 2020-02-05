#ifndef _MECAB_STREAM_WRAPPER_H_
#define _MECAB_STREAM_WRAPPER_H_

#include <cstring>
#include <fstream>
#include <iostream>

#include "mecab/utils.h"

namespace MeCab {

class istream_wrapper {
 private:
  std::istream* is_;

 public:
  std::istream& operator*() const { return *is_; }
  std::istream* operator->() const { return is_; }
  explicit istream_wrapper(const char* filename) : is_(0) {
    if (std::strcmp(filename, "-") == 0) {
      is_ = &std::cin;
    } else {
      is_ = new std::ifstream(filename);
    }
  }

  virtual ~istream_wrapper() {
    if (is_ != &std::cin)
      delete is_;
  }
};

class ostream_wrapper {
 private:
  std::ostream* os_;

 public:
  std::ostream& operator*() const { return *os_; }
  std::ostream* operator->() const { return os_; }
  explicit ostream_wrapper(const char* filename) : os_(0) {
    if (std::strcmp(filename, "-") == 0) {
      os_ = &std::cout;
    } else {
      os_ = new std::ofstream(filename);
    }
  }

  virtual ~ostream_wrapper() {
    if (os_ != &std::cout)
      delete os_;
  }
};
}  // namespace MeCab

#endif  // _MECAB_STREAM_WRAPPER_H_
