#ifndef _MECAB_CONTEXT_ID_H_
#define _MECAB_CONTEXT_ID_H_

#include <map>
#include <string>
#include <vector>

#include "mecab/iconv.h"
#include "mecab/param.h"

namespace MeCab {

class ContextID {
 private:
  std::map<std::string, int> left_;
  std::map<std::string, int> right_;
  std::string left_bos_;
  std::string right_bos_;

 public:
  void clear();
  void add(const char* l, const char* r);
  void addBOS(const char* l, const char* r);
  bool save(const char* lfile, const char* rfile);
  bool build();
  bool open(const char* lfile, const char* rfile, Iconv* iconv = 0);
  int lid(const char* l) const;
  int rid(const char* r) const;

  size_t left_size() const { return left_.size(); }
  size_t right_size() const { return right_.size(); }

  const std::map<std::string, int>& left_ids() const { return left_; }
  const std::map<std::string, int>& right_ids() const { return right_; }

  bool is_valid(size_t lid, size_t rid) { return (lid >= 0 && lid < left_size() && rid >= 0 && rid < right_size()); }
};
}  // namespace MeCab

#endif  // _MECAB_CONTEXT_ID_H_
