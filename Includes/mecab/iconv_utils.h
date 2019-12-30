#ifndef MECAB_ICONV_H
#define MECAB_ICONV_H

#include <iconv.h>

namespace MeCab {

class Iconv {
 private:
  iconv_t ic_;

 public:
  explicit Iconv();
  virtual ~Iconv();
  bool open(const char* from, const char* to);
  bool convert(std::string*);
};
}  // namespace MeCab

#endif
