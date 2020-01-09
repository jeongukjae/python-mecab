#ifndef _MECAB_COMMON_H_
#define _MECAB_COMMON_H_

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#define COPYRIGHT \
  "MeCab: Yet Another Part-of-Speech and Morphological Analyzer\n\
\nCopyright(C) 2001-2012 Taku Kudo\nCopyright(C) 2004-2008 Nippon Telegraph and Telephone Corporation\n"

#define PACKAGE "mecab"
#define VERSION "0.996"

#define MECAB_DEFAULT_RC "/usr/local/etc/mecabrc"

#define DIC_VERSION 102

#define SYS_DIC_FILE "sys.dic"
#define UNK_DEF_FILE "unk.def"
#define UNK_DIC_FILE "unk.dic"
#define MATRIX_DEF_FILE "matrix.def"
#define MATRIX_FILE "matrix.bin"
#define CHAR_PROPERTY_DEF_FILE "char.def"
#define CHAR_PROPERTY_FILE "char.bin"
#define FEATURE_FILE "feature.def"
#define REWRITE_FILE "rewrite.def"
#define LEFT_ID_FILE "left-id.def"
#define RIGHT_ID_FILE "right-id.def"
#define POS_ID_FILE "pos-id.def"
#define MODEL_DEF_FILE "model.def"
#define MODEL_FILE "model.bin"
#define DICRC "dicrc"
#define BOS_KEY "BOS/EOS"

#define DEFAULT_MAX_GROUPING_SIZE 24

#define CHAR_PROPERTY_DEF_DEFAULT "DEFAULT 1 0 0\nSPACE   0 1 0\n0x0020 SPACE\n"
#define UNK_DEF_DEFAULT "DEFAULT,0,0,0,*\nSPACE,0,0,0,*\n"
#define MATRIX_DEF_DEFAULT "1 1\n0 0 0\n"

#define MECAB_DEFAULT_CHARSET "UTF-8"

#define NBEST_MAX 512
#define NODE_FREELIST_SIZE 512
#define PATH_FREELIST_SIZE 2048
#define MIN_INPUT_BUFFER_SIZE 8192
#define MAX_INPUT_BUFFER_SIZE (8192 * 640)
#define BUF_SIZE 8192

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

namespace MeCab {
class die {
 public:
  die() {}
  ~die() {
    std::cerr << std::endl;
    exit(-1);
  }
  int operator&(std::ostream&) { return 0; }
};

class wlog {
 public:
  wlog() {}
  ~wlog() { std::cerr << std::endl; }
  bool operator&(std::ostream&) { return false; }
};
}  // namespace MeCab

#define CHECK_FALSE(condition) \
  if (condition) {             \
  } else                       \
    return wlog() & std::cerr << __FILE__ << "(" << __LINE__ << ") [" << #condition << "] "

#define CHECK_DIE(condition) \
  (condition) ? 0 : die() & std::cerr << __FILE__ << "(" << __LINE__ << ") [" << #condition << "] "

#endif  // _MECAB_COMMON_H_
