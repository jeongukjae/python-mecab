#ifndef _MECAB_DICTIONARY_REWRITER_H_
#define _MECAB_DICTIONARY_REWRITER_H_

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "mecab.h"
#include "mecab/common.h"
#include "mecab/freelist.h"
#include "mecab/iconv.h"
#include "mecab/scoped_ptr.h"

namespace MeCab {

namespace {
bool match_rewrite_pattern(const char* pat, const char* str) {
  if (pat[0] == '*' || std::strcmp(pat, str) == 0)
    return true;

  size_t len = std::strlen(pat);
  if (len >= 3 && pat[0] == '(' && pat[len - 1] == ')') {
    scoped_fixed_array<char, BUF_SIZE> buf;
    scoped_fixed_array<char*, BUF_SIZE> col;
    CHECK_DIE(len < buf.size() - 3) << "too long parameter";
    std::strncpy(buf.get(), pat + 1, buf.size());
    buf[len - 2] = '\0';
    const size_t n = tokenize(buf.get(), "|", col.get(), col.size());
    CHECK_DIE(n < col.size()) << "too long OR nodes";
    for (size_t i = 0; i < n; ++i) {
      if (std::strcmp(str, col[i]) == 0)
        return true;
    }
  }
  return false;
}
}  // namespace

class RewritePattern {
 private:
  std::vector<std::string> spat_;
  std::vector<std::string> dpat_;

 public:
  bool set_pattern(const char* src, const char* dst) {
    scoped_fixed_array<char, BUF_SIZE> buf;
    spat_.clear();
    dpat_.clear();

    std::strncpy(buf.get(), src, buf.size());
    tokenizeCSV(buf.get(), back_inserter(spat_), 512);

    std::strncpy(buf.get(), dst, buf.size());
    tokenizeCSV(buf.get(), back_inserter(dpat_), 512);

    return (spat_.size() && dpat_.size());
  }
  bool rewrite(size_t size, const char** input, std::string* output) const {
    if (spat_.size() > size)
      return false;
    for (size_t i = 0; i < spat_.size(); ++i) {
      if (!match_rewrite_pattern(spat_[i].c_str(), input[i]))
        return false;
    }

    output->clear();
    for (size_t i = 0; i < dpat_.size(); ++i) {
      std::string elm;
      const char* begin = dpat_[i].c_str();
      const char* end = begin + dpat_[i].size();
      for (const char* p = begin; p < end; ++p) {
        if (*p == '$') {
          ++p;
          size_t n = 0;
          for (; p < end; ++p) {
            switch (*p) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                n = 10 * n + (*p - '0');
                break;
              default:
                goto NEXT;
            }
          }
        NEXT:
          CHECK_DIE(n > 0 && (n - 1) < size) << " out of range: [" << dpat_[i] << "] " << n;
          elm += input[n - 1];
          if (p < end)
            elm += *p;
        } else {
          elm += *p;
        }
      }
      CHECK_DIE(escape_csv_element(&elm));
      *output += elm;
      if (i + 1 != dpat_.size())
        *output += ",";
    }

    return true;
  }
};

class RewriteRules : public std::vector<RewritePattern> {
 public:
  bool rewrite(size_t size, const char** input, std::string* output) const {
    for (size_t i = 0; i < this->size(); ++i) {
      if ((*this)[i].rewrite(size, input, output))
        return true;
    }
    return false;
  }

  void appendRewriteRule(char* str) {
    char* col[3];
    const size_t n = tokenize2(str, " \t", col, 3);
    CHECK_DIE(n >= 2) << "format error: " << str;
    this->resize(this->size() + 1);
    std::string tmp;
    if (n >= 3) {
      tmp = col[1];
      tmp += ' ';
      tmp += col[2];
      col[1] = const_cast<char*>(tmp.c_str());
    }
    this->back().set_pattern(col[0], col[1]);
  }
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
  bool open(const char* filename, Iconv* iconv = 0) {
    std::ifstream ifs(filename);
    CHECK_DIE(ifs) << "no such file or directory: " << filename;
    int append_to = 0;
    std::string line;
    while (std::getline(ifs, line)) {
      if (iconv)
        iconv->convert(&line);
      if (line.empty() || line[0] == '#')
        continue;
      if (line == "[unigram rewrite]") {
        append_to = 1;
      } else if (line == "[left rewrite]") {
        append_to = 2;
      } else if (line == "[right rewrite]") {
        append_to = 3;
      } else {
        CHECK_DIE(append_to != 0) << "no sections found";
        char* str = const_cast<char*>(line.c_str());
        switch (append_to) {
          case 1:
            unigram_rewrite_.appendRewriteRule(str);
            break;
          case 2:
            left_rewrite_.appendRewriteRule(str);
            break;
          case 3:
            right_rewrite_.appendRewriteRule(str);
            break;
        }
      }
    }
    return true;
  }
  void clear() { cache_.clear(); }
  // without cache
  bool rewrite(const std::string& feature, std::string* ufeature, std::string* lfeature, std::string* rfeature) const {
    scoped_fixed_array<char, BUF_SIZE> buf;
    scoped_fixed_array<char*, BUF_SIZE> col;
    CHECK_DIE(feature.size() < buf.size() - 1) << "too long feature";
    std::strncpy(buf.get(), feature.c_str(), buf.size() - 1);
    const size_t n = tokenizeCSV(buf.get(), col.get(), col.size());
    CHECK_DIE(n < col.size()) << "too long CSV entities";
    return (unigram_rewrite_.rewrite(n, const_cast<const char**>(col.get()), ufeature) &&
            left_rewrite_.rewrite(n, const_cast<const char**>(col.get()), lfeature) &&
            right_rewrite_.rewrite(n, const_cast<const char**>(col.get()), rfeature));
  }

  bool rewrite2(const std::string& feature, std::string* ufeature, std::string* lfeature, std::string* rfeature) {
    std::map<std::string, FeatureSet>::const_iterator it = cache_.find(feature);
    if (it == cache_.end()) {
      if (!rewrite(feature, ufeature, lfeature, rfeature))
        return false;
      FeatureSet f;
      f.ufeature = *ufeature;
      f.lfeature = *lfeature;
      f.rfeature = *rfeature;
      cache_.insert(std::pair<std::string, FeatureSet>(feature, f));
    } else {
      *ufeature = it->second.ufeature;
      *lfeature = it->second.lfeature;
      *rfeature = it->second.rfeature;
    }

    return true;
  }
};

class POSIDGenerator {
 private:
  RewriteRules rewrite_;

 public:
  bool open(const char* filename, Iconv* iconv = 0) {
    std::ifstream ifs(filename);
    if (!ifs) {
      std::cerr << filename << " is not found. minimum setting is used" << std::endl;
      rewrite_.resize(1);
      rewrite_.back().set_pattern("*", "1");
      return true;
    }

    std::string line;
    char* col[2];
    while (std::getline(ifs, line)) {
      if (iconv)
        iconv->convert(&line);
      const size_t n = tokenize2(const_cast<char*>(line.c_str()), " \t", col, 2);
      CHECK_DIE(n == 2) << "format error: " << line;
      for (char* p = col[1]; *p; ++p) {
        CHECK_DIE(*p >= '0' && *p <= '9') << "not a number: " << col[1];
      }
      rewrite_.resize(rewrite_.size() + 1);
      rewrite_.back().set_pattern(col[0], col[1]);
    }
    return true;
  }
  void clear() { rewrite_.clear(); }
  int id(const char* feature) const {
    scoped_fixed_array<char, BUF_SIZE> buf;
    scoped_fixed_array<char*, BUF_SIZE> col;
    CHECK_DIE(std::strlen(feature) < buf.size() - 1) << "too long feature";
    std::strncpy(buf.get(), feature, buf.size() - 1);
    const size_t n = tokenizeCSV(buf.get(), col.get(), col.size());
    CHECK_DIE(n < col.size()) << "too long CSV entities";
    std::string tmp;
    if (!rewrite_.rewrite(n, const_cast<const char**>(col.get()), &tmp)) {
      return -1;
    }
    return std::atoi(tmp.c_str());
  }
};
}  // namespace MeCab

#endif  // _MECAB_DICTIONARY_REWRITER_H_
