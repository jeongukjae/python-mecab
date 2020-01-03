#ifndef _MECAB_ICONV_H_
#define _MECAB_ICONV_H_

#include <iconv.h>
#include <string>

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

#endif  // _MECAB_ICONV_H_
