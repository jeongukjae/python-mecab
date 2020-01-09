#ifndef _MECAB_MMAP_H_
#define _MECAB_MMAP_H_

#include <errno.h>
#include <sys/stat.h>

#include <string>

#include "mecab/common.h"
#include "mecab/utils.h"

namespace MeCab {

template <class T>
class Mmap {
 private:
  T* text;
  size_t length;
  std::string fileName;

  FILE* fd;
  std::string flag;

 public:
  T& operator[](size_t n) { return *(text + n); }
  const T& operator[](size_t n) const { return *(text + n); }
  T* begin() { return text; }
  const T* begin() const { return text; }
  T* end() { return text + size(); }
  const T* end() const { return text + size(); }
  size_t size() { return length / sizeof(T); }
  std::string file_name() { return fileName; }
  size_t file_size() { return length; }
  bool empty() { return (length == 0); }

  bool open(const char* filename, const char* mode = "r") {
    this->close();
    struct stat st;
    int fileDescriptor;
    fileName = std::string(filename);
    flag = std::string(mode);

    CHECK_FALSE(flag.compare("r") == 0 || flag.compare("r+") == 0)
        << "unknown open mode: " << filename << " mode: " << flag << std::endl;

    flag += "b";

    CHECK_FALSE((fd = ::fopen(filename, flag.c_str())) != NULL) << "open failed: " << filename;

    CHECK_FALSE((fileDescriptor = ::fileno(fd)) >= 0) << "cannot get file descriptor: " << filename;
    CHECK_FALSE(::fstat(fileDescriptor, &st) >= 0) << "failed to get file size: " << filename;

    length = st.st_size;

    text = new T[length];
    CHECK_FALSE(::fread(text, 1, length, fd) >= 0) << "read() failed: " << filename;
    ::fclose(fd);
    fd = NULL;

    return true;
  }

  void close() {
    if (fd != NULL) {
      ::fclose(fd);
      fd = NULL;
    }

    if (text) {
      if (flag.compare("r+b")) {
        FILE* fd2;
        if ((fd2 = ::fopen(fileName.c_str(), "wb")) != NULL) {
          ::fwrite(text, 1, length, fd2);
          ::fclose(fd2);
        }
      }
      delete[] text;
    }

    text = 0;
  }

  Mmap() : text(NULL), length(0), fd(NULL) {}
  virtual ~Mmap() { this->close(); }
};
}  // namespace MeCab

#endif  // _MECAB_MMAP_H_
