#ifndef _MECAB_CONNECTOR_H_
#define _MECAB_CONNECTOR_H_

#include "mecab.h"
#include "mecab/common.h"
#include "mecab/mmap.h"
#include "mecab/param.h"
#include "mecab/scoped_ptr.h"

namespace MeCab {

class Connector {
 private:
  scoped_ptr<Mmap<short>> cmmap_;
  short* matrix_;
  unsigned short lsize_;
  unsigned short rsize_;

 public:
  bool open(const Param& param) {
    const std::string filename = create_filename(param.get<std::string>("dicdir"), MATRIX_FILE);
    return open(filename.c_str());
  }

  bool open(const char* filename, const char* mode = "r") {
    CHECK_FALSE(cmmap_->open(filename, mode)) << "cannot open: " << filename;

    matrix_ = cmmap_->begin();

    CHECK_FALSE(matrix_) << "matrix is NULL";
    CHECK_FALSE(cmmap_->size() >= 2) << "file size is invalid: " << filename;

    lsize_ = static_cast<unsigned short>((*cmmap_)[0]);
    rsize_ = static_cast<unsigned short>((*cmmap_)[1]);

    CHECK_FALSE(static_cast<size_t>(lsize_ * rsize_ + 2) == cmmap_->size()) << "file size is invalid: " << filename;

    matrix_ = cmmap_->begin() + 2;
    return true;
  }

  bool openText(const char* filename) {
    std::ifstream ifs(filename);
    CHECK_FALSE(ifs) << "no such file or directory: " << filename;
    char* column[2];
    scoped_fixed_array<char, BUF_SIZE> buf;
    ifs.getline(buf.get(), buf.size());
    CHECK_DIE(tokenize2(buf.get(), "\t ", column, 2) == 2) << "format error: " << buf.get();
    lsize_ = std::atoi(column[0]);
    rsize_ = std::atoi(column[1]);
    return true;
  }

  void close() { cmmap_->close(); }
  void clear() {}

  size_t left_size() const { return static_cast<size_t>(lsize_); }
  size_t right_size() const { return static_cast<size_t>(rsize_); }

  void set_left_size(size_t lsize) { lsize_ = lsize; }
  void set_right_size(size_t rsize) { rsize_ = rsize; }

  inline int transition_cost(unsigned short rcAttr, unsigned short lcAttr) const {
    return matrix_[rcAttr + lsize_ * lcAttr];
  }

  inline int cost(const Node* lNode, const Node* rNode) const {
    return matrix_[lNode->rcAttr + lsize_ * rNode->lcAttr] + rNode->wcost;
  }

  // access to raw matrix
  short* mutable_matrix() { return &matrix_[0]; }
  const short* matrix() const { return &matrix_[0]; }

  bool is_valid(size_t lid, size_t rid) const { return (lid >= 0 && lid < rsize_ && rid >= 0 && rid < lsize_); }

  static bool compile(const char* ifile, const char* ofile) {
    std::ifstream ifs(ifile);
    std::istringstream iss(MATRIX_DEF_DEFAULT);
    std::istream* is = &ifs;

    if (!ifs) {
      std::cerr << ifile << " is not found. minimum setting is used." << std::endl;
      is = &iss;
    }

    char* column[4];
    scoped_fixed_array<char, BUF_SIZE> buf;

    is->getline(buf.get(), buf.size());

    CHECK_DIE(tokenize2(buf.get(), "\t ", column, 2) == 2) << "format error: " << buf.get();

    const unsigned short lsize = std::atoi(column[0]);
    const unsigned short rsize = std::atoi(column[1]);
    std::vector<short> matrix(lsize * rsize);
    std::fill(matrix.begin(), matrix.end(), 0);

    std::cout << "reading " << ifile << " ... " << lsize << "x" << rsize << std::endl;

    while (is->getline(buf.get(), buf.size())) {
      CHECK_DIE(tokenize2(buf.get(), "\t ", column, 3) == 3) << "format error: " << buf.get();
      const size_t l = std::atoi(column[0]);
      const size_t r = std::atoi(column[1]);
      const int c = std::atoi(column[2]);
      CHECK_DIE(l < lsize && r < rsize) << "index values are out of range";
      progress_bar("emitting matrix      ", l + 1, lsize);
      matrix[(l + lsize * r)] = static_cast<short>(c);
    }

    std::ofstream ofs(ofile, std::ios::binary | std::ios::out);
    CHECK_DIE(ofs) << "permission denied: " << ofile;
    ofs.write(reinterpret_cast<const char*>(&lsize), sizeof(unsigned short));
    ofs.write(reinterpret_cast<const char*>(&rsize), sizeof(unsigned short));
    ofs.write(reinterpret_cast<const char*>(&matrix[0]), lsize * rsize * sizeof(short));
    ofs.close();

    return true;
  }

  explicit Connector() : cmmap_(new Mmap<short>), matrix_(0), lsize_(0), rsize_(0) {}

  virtual ~Connector() { this->close(); }
};
}  // namespace MeCab

#endif  // _MECAB_CONNECTOR_H_
