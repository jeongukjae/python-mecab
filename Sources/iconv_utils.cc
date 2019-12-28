#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "common.h"
#include "scoped_ptr.h"
#include "utils.h"

#include "char_property.h"
#include "iconv_utils.h"

namespace {

#ifdef HAVE_ICONV
const char* decode_charset_iconv(const char* str) {
  const int charset = MeCab::decode_charset(str);
  switch (charset) {
    case MeCab::UTF8:
      return "UTF-8";
    case MeCab::EUC_JP:
      return "EUC-JP";
    case MeCab::CP932:
      return "SHIFT-JIS";
    case MeCab::UTF16:
      return "UTF-16";
    case MeCab::UTF16LE:
      return "UTF-16LE";
    case MeCab::UTF16BE:
      return "UTF-16BE";
    default:
      std::cerr << "charset " << str << " is not defined, use " MECAB_DEFAULT_CHARSET;
      return MECAB_DEFAULT_CHARSET;
  }
  return MECAB_DEFAULT_CHARSET;
}
#endif
}  // namespace

namespace MeCab {
bool Iconv::open(const char* from, const char* to) {
  ic_ = 0;
#if defined HAVE_ICONV
  const char* from2 = decode_charset_iconv(from);
  const char* to2 = decode_charset_iconv(to);
  if (std::strcmp(from2, to2) == 0) {
    return true;
  }
  ic_ = 0;
  ic_ = iconv_open(to2, from2);
  if (ic_ == (iconv_t)(-1)) {
    ic_ = 0;
    return false;
  }
#else
  std::cerr << "iconv_open is not supported" << std::endl;
#endif

  return true;
}

bool Iconv::convert(std::string* str) {
  if (str->empty()) {
    return true;
  }
  if (ic_ == 0) {
    return true;
  }

#if defined HAVE_ICONV
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
  iconv(ic_, 0, &ilen, 0, &olen);  // reset iconv state
  while (ilen != 0) {
    if (iconv(ic_, (ICONV_CONST char**)&ibuf, &ilen, &obuf, &olen) == (size_t)-1) {
      return false;
    }
  }
  str->assign(obuf_org, olen_org - olen);
#endif

  return true;
}

Iconv::Iconv() : ic_(0) {}

Iconv::~Iconv() {
#if defined HAVE_ICONV
  if (ic_ != 0)
    iconv_close(ic_);
#endif
}
}  // namespace MeCab
