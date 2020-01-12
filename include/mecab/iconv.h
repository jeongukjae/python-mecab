#ifndef _MECAB_ICONV_H_
#define _MECAB_ICONV_H_

#include <iconv.h>
#include <string>

#include "mecab/common.h"
#include "mecab/utils.h"

namespace MeCab {

namespace {

const char* decodeCharsetForIconv(const char* str) {
  const int charset = decode_charset(str);
  switch (charset) {
    case MeCab::UTF8:
      return "UTF-8";
    case MeCab::UTF16:
      return "UTF-16";
    case MeCab::UTF16LE:
      return "UTF-16LE";
    case MeCab::UTF16BE:
      return "UTF-16BE";
    default:
      std::cerr << "charset " << str << " is not defined, use " MECAB_DEFAULT_CHARSET << std::endl;
      return MECAB_DEFAULT_CHARSET;
  }
  return MECAB_DEFAULT_CHARSET;
}
}  // namespace

class Iconv {
 private:
  iconv_t conversionDescriptor;

 public:
  explicit Iconv() : conversionDescriptor(0) {}
  virtual ~Iconv() {
    if (conversionDescriptor != NULL)
      iconv_close(conversionDescriptor);
  }

  bool open(const char* from, const char* to) {
    conversionDescriptor = NULL;
    const char* from2 = decodeCharsetForIconv(from);
    const char* to2 = decodeCharsetForIconv(to);
    if (std::strcmp(from2, to2) == 0) {
      return true;
    }
    conversionDescriptor = NULL;
    conversionDescriptor = iconv_open(to2, from2);
    if (conversionDescriptor == (iconv_t)(-1)) {
      conversionDescriptor = NULL;
      return false;
    }

    return true;
  }

  bool convert(std::string* str) {
    if (str->empty())
      return true;
    if (conversionDescriptor == NULL)
      return true;

    size_t ilen = 0;
    size_t olen = 0;
    ilen = str->size();
    olen = ilen * 4;
    std::string tmp;
    tmp.reserve(olen);
    char* ibuf = const_cast<char*>(str->data());
    char* obuf_org = const_cast<char*>(tmp.data());
    char* obuf = obuf_org;
    std::fill(obuf, obuf + olen, 0);
    size_t olen_org = olen;
    iconv(conversionDescriptor, 0, &ilen, 0, &olen);  // reset iconv state
    while (ilen != 0) {
      if (iconv(conversionDescriptor, (char**)&ibuf, &ilen, &obuf, &olen) == (size_t)-1) {
        return false;
      }
    }
    str->assign(obuf_org, olen_org - olen);

    return true;
  }
};
}  // namespace MeCab

#endif  // _MECAB_ICONV_H_
