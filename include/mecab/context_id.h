#ifndef _MECAB_CONTEXT_ID_H_
#define _MECAB_CONTEXT_ID_H_

#include <map>
#include <string>
#include <vector>

#include "mecab/iconv.h"
#include "mecab/param.h"

namespace MeCab {

namespace {
bool openMap(const char* filename, std::map<std::string, int>* cmap, Iconv* iconv) {
  std::ifstream ifs(filename);
  CHECK_DIE(ifs) << "no such file or directory: " << filename;
  cmap->clear();
  char* col[2];
  std::string line;
  while (std::getline(ifs, line)) {
    CHECK_DIE(2 == tokenize2(const_cast<char*>(line.c_str()), " \t", col, 2)) << "format error: " << line;
    std::string pos = col[1];
    if (iconv) {
      iconv->convert(&pos);
    }
    cmap->insert(std::pair<std::string, int>(pos, std::atoi(col[0])));
  }
  return true;
}

bool buildBOS(std::map<std::string, int>* cmap, const std::string& bos) {
  int id = 1;  // for BOS/EOS
  for (std::map<std::string, int>::iterator it = cmap->begin(); it != cmap->end(); ++it)
    it->second = id++;
  cmap->insert(std::make_pair(bos, 0));
  return true;
}

bool saveFile(const char* filename, std::map<std::string, int>* cmap) {
  std::ofstream ofs(filename);
  CHECK_DIE(ofs) << "permission denied: " << filename;
  for (std::map<std::string, int>::const_iterator it = cmap->begin(); it != cmap->end(); ++it) {
    ofs << it->second << " " << it->first << std::endl;
  }
  return true;
}
}  // namespace

class ContextID {
 private:
  std::map<std::string, int> left_;
  std::map<std::string, int> right_;
  std::string left_bos_;
  std::string right_bos_;

 public:
  void clear() {
    left_.clear();
    right_.clear();
    left_bos_.clear();
    right_bos_.clear();
  }

  void add(const char* l, const char* r) {
    left_.insert(std::make_pair(std::string(l), 1));
    right_.insert(std::make_pair(std::string(r), 1));
  }

  void addBOS(const char* l, const char* r) {
    left_bos_ = l;
    right_bos_ = r;
  }
  bool save(const char* lfile, const char* rfile) { return saveFile(lfile, &left_) && saveFile(rfile, &right_); }
  bool build() { return buildBOS(&left_, left_bos_) && buildBOS(&right_, right_bos_); }
  bool open(const char* lfile, const char* rfile, Iconv* iconv = 0) {
    return (openMap(lfile, &left_, iconv) && openMap(rfile, &right_, iconv));
  }

  int lid(const char* l) const {
    std::map<std::string, int>::const_iterator it = left_.find(l);
    CHECK_DIE(it != left_.end()) << "cannot find LEFT-ID  for " << l;
    return it->second;
  }
  int rid(const char* r) const {
    std::map<std::string, int>::const_iterator it = right_.find(r);
    CHECK_DIE(it != right_.end()) << "cannot find RIGHT-ID  for " << r;
    return it->second;
  }

  size_t left_size() const { return left_.size(); }
  size_t right_size() const { return right_.size(); }

  const std::map<std::string, int>& left_ids() const { return left_; }
  const std::map<std::string, int>& right_ids() const { return right_; }

  bool is_valid(size_t lid, size_t rid) { return (lid >= 0 && lid < left_size() && rid >= 0 && rid < right_size()); }
};
}  // namespace MeCab

#endif  // _MECAB_CONTEXT_ID_H_
