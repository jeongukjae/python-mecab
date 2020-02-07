#ifndef __MECAB_ALLOCATOR_H__
#define __MECAB_ALLOCATOR_H__

#include "mecab/common.h"
#include "mecab/darts.h"
#include "mecab/nbest_generator.h"
#include "mecab/utils/scoped_ptr.h"

namespace MeCab {

template <typename N, typename P>
class Allocator {
 public:
  N* newNode() {
    N* node = node_freelist_->alloc();
    std::memset(node, 0, sizeof(N));
    node->id = id_++;
    return node;
  }

  P* newPath() {
    if (!path_freelist_.get()) {
      path_freelist_.reset(new FreeList<P>(PATH_FREELIST_SIZE));
    }
    return path_freelist_->alloc();
  }

  Darts::DoubleArray::result_pair_type* mutable_results() { return results_.get(); }

  char* alloc(size_t size) {
    if (!char_freelist_.get()) {
      char_freelist_.reset(new ChunkFreeList<char>(BUF_SIZE));
    }
    return char_freelist_->alloc(size + 1);
  }

  char* strdup(const char* str, size_t size) {
    char* n = alloc(size + 1);
    std::strncpy(n, str, size + 1);
    return n;
  }

  NBestGenerator* nbest_generator() {
    if (!nbest_generator_.get()) {
      nbest_generator_.reset(new NBestGenerator);
    }
    return nbest_generator_.get();
  }

  char* partial_buffer(size_t size) {
    partial_buffer_.resize(size);
    return &partial_buffer_[0];
  }

  size_t results_size() const { return kResultsSize; }

  void free() {
    id_ = 0;
    node_freelist_->free();
    if (path_freelist_.get()) {
      path_freelist_->free();
    }
    if (char_freelist_.get()) {
      char_freelist_->free();
    }
  }

  Allocator()
      : id_(0),
        node_freelist_(new FreeList<N>(NODE_FREELIST_SIZE)),
        path_freelist_(0),
        char_freelist_(0),
        nbest_generator_(0),
        results_(new Darts::DoubleArray::result_pair_type[kResultsSize]) {}
  virtual ~Allocator() {}

 private:
  static const size_t kResultsSize = 512;
  size_t id_;
  scoped_ptr<FreeList<N>> node_freelist_;
  scoped_ptr<FreeList<P>> path_freelist_;
  scoped_ptr<ChunkFreeList<char>> char_freelist_;
  scoped_ptr<NBestGenerator> nbest_generator_;
  std::vector<char> partial_buffer_;
  scoped_array<Darts::DoubleArray::result_pair_type> results_;
};

}  // namespace MeCab

#endif  // __MECAB_ALLOCATOR_H__
