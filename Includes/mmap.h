#ifndef MECAB_MMAP_H
#define MECAB_MMAP_H

#include <errno.h>
#include <sys/stat.h>

#include <string>

extern "C" {

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
}

#include "common.h"
#include "utils.h"

namespace MeCab {

template <class T>
class Mmap {
 private:
  T* text;
  size_t length;
  std::string fileName;
  whatlog what_;

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
  const char* what() { return what_.str(); }
  const char* file_name() { return fileName.c_str(); }
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
    CHECK_FALSE(::fread(text, sizeof(T), length, fd) >= 0) << "read() failed: " << filename;
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
        if ((fd2 = ::fopen(fileName.c_str(), "r+")) == NULL) {
          ::fwrite(text, sizeof(T), length, fd2);
          ::fclose(fd2);
        }
      }
      delete[] text;
    }

    text = 0;
  }

  Mmap() : text(NULL), fd(NULL) {}

  virtual ~Mmap() { this->close(); }
};
}  // namespace MeCab
#endif
