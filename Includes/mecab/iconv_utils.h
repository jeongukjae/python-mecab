#ifndef MECAB_ICONV_H
#define MECAB_ICONV_H

#if defined HAVE_ICONV
#include <iconv.h>
#endif

namespace MeCab {

class Iconv {
 private:
#ifdef HAVE_ICONV
  iconv_t ic_;
#else
  int ic_;
#endif

 public:
  explicit Iconv();
  virtual ~Iconv();
  bool open(const char* from, const char* to);
  bool convert(std::string*);
};
}  // namespace MeCab

#endif
