#ifndef _MECAB_DICTIONARY_REWRITER_H_
#define _MECAB_DICTIONARY_REWRITER_H_

#include <map>
#include <string>
#include <vector>

#include "mecab.h"
#include "mecab/common.h"
#include "mecab/freelist.h"

namespace MeCab {

class Iconv;

class RewritePattern {
 private:
  std::vector<std::string> spat_;
  std::vector<std::string> dpat_;

 public:
  bool set_pattern(const char* src, const char* dst);
  bool rewrite(size_t size, const char** input, std::string* output) const;
};

class RewriteRules : public std::vector<RewritePattern> {
 public:
  bool rewrite(size_t size, const char** input, std::string* output) const;
};

struct FeatureSet {
  std::string ufeature;
  std::string lfeature;
  std::string rfeature;
};

class DictionaryRewriter {
 private:
  RewriteRules unigram_rewrite_;
  RewriteRules left_rewrite_;
  RewriteRules right_rewrite_;
  std::map<std::string, FeatureSet> cache_;

 public:
  bool open(const char* filename, Iconv* iconv = 0);
  void clear();
  bool rewrite(const std::string& feature, std::string* ufeature, std::string* lfeature, std::string* rfeature) const;

  bool rewrite2(const std::string& feature, std::string* ufeature, std::string* lfeature, std::string* rfeature);
};

class POSIDGenerator {
 private:
  RewriteRules rewrite_;

 public:
  bool open(const char* filename, Iconv* iconv = 0);
  void clear() { rewrite_.clear(); }
  int id(const char* key) const;
};
}  // namespace MeCab

#endif  // _MECAB_DICTIONARY_REWRITER_H_