#ifndef _MECAB_STRINGBUFFER_H
#define _MECAB_STRINGBUFFER_H

#include <string>

#include "mecab/common.h"
#include "utils.h"

#define DEFAULT_ALLOC_SIZE BUF_SIZE

namespace MeCab {

#define _ITOA(n)              \
  do {                        \
    char fbuf[64];            \
    itoa(n, fbuf);            \
    return this->write(fbuf); \
  } while (0)
#define _UITOA(n)             \
  do {                        \
    char fbuf[64];            \
    uitoa(n, fbuf);           \
    return this->write(fbuf); \
  } while (0)
#define _DTOA(n)              \
  do {                        \
    char fbuf[64];            \
    dtoa(n, fbuf);            \
    return this->write(fbuf); \
  } while (0)

class StringBuffer {
 private:
  size_t size_;
  size_t alloc_size_;
  char* ptr_;
  bool is_delete_;
  bool error_;
  bool reserve(size_t length) {
    if (!is_delete_) {
      error_ = (size_ + length >= alloc_size_);
      return (!error_);
    }

    if (size_ + length >= alloc_size_) {
      if (alloc_size_ == 0) {
        alloc_size_ = DEFAULT_ALLOC_SIZE;
        ptr_ = new char[alloc_size_];
      }
      size_t len = size_ + length;
      do {
        alloc_size_ *= 2;
      } while (len >= alloc_size_);
      char* new_ptr = new char[alloc_size_];
      std::memcpy(new_ptr, ptr_, size_);
      delete[] ptr_;
      ptr_ = new_ptr;
    }

    return true;
  }

 public:
  explicit StringBuffer() : size_(0), alloc_size_(0), ptr_(0), is_delete_(true), error_(false) {}
  explicit StringBuffer(char* _s, size_t _l) : size_(0), alloc_size_(_l), ptr_(_s), is_delete_(false), error_(false) {}

  virtual ~StringBuffer() {
    if (is_delete_) {
      delete[] ptr_;
      ptr_ = 0;
    }
  }

  StringBuffer& write(char str) {
    if (reserve(1)) {
      ptr_[size_] = str;
      ++size_;
    }
    return *this;
  }
  StringBuffer& write(const char* str, size_t length) {
    if (reserve(length)) {
      std::memcpy(ptr_ + size_, str, length);
      size_ += length;
    }
    return *this;
  }
  StringBuffer& write(const char* str) { return this->write(str, std::strlen(str)); }
  StringBuffer& operator<<(double n) { _DTOA(n); }
  StringBuffer& operator<<(short int n) { _ITOA(n); }
  StringBuffer& operator<<(int n) { _ITOA(n); }
  StringBuffer& operator<<(long int n) { _ITOA(n); }
  StringBuffer& operator<<(unsigned short int n) { _UITOA(n); }
  StringBuffer& operator<<(unsigned int n) { _UITOA(n); }
  StringBuffer& operator<<(unsigned long int n) { _UITOA(n); }
#ifdef HAVE_UNSIGNED_LONG_LONG_INT
  StringBuffer& operator<<(unsigned long long int n) { _UITOA(n); }
#endif

  StringBuffer& operator<<(char n) { return this->write(n); }

  StringBuffer& operator<<(unsigned char n) { return this->write(n); }

  StringBuffer& operator<<(const char* n) { return this->write(n); }

  StringBuffer& operator<<(const std::string& n) { return this->write(n.c_str()); }

  void clear() { size_ = 0; }
  const char* str() const { return error_ ? 0 : const_cast<const char*>(ptr_); }
};
}  // namespace MeCab

#endif  // _MECAB_STRINGBUFFER_H
