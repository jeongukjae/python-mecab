#ifndef _MECAB_TOKENIZER_H_
#define _MECAB_TOKENIZER_H_

#include "mecab.h"
#include "mecab/char_property.h"
#include "mecab/dictionary.h"
#include "mecab/freelist.h"
#include "mecab/nbest_generator.h"
#include "mecab/param.h"
#include "mecab/scoped_ptr.h"

namespace MeCab {
namespace {
void inline read_node_info(const Dictionary& dic, const Token& token, LearnerNode** node) {
  (*node)->lcAttr = token.lcAttr;
  (*node)->rcAttr = token.rcAttr;
  (*node)->posid = token.posid;
  (*node)->wcost2 = token.wcost;
  (*node)->feature = dic.feature(token);
}

void inline read_node_info(const Dictionary& dic, const Token& token, Node** node) {
  (*node)->lcAttr = token.lcAttr;
  (*node)->rcAttr = token.rcAttr;
  (*node)->posid = token.posid;
  (*node)->wcost = token.wcost;
  (*node)->feature = dic.feature(token);
}

inline bool partial_match(const char* f1, const char* f2) {
  if (std::strcmp(f1, "*") == 0) {
    return true;
  }

  scoped_fixed_array<char, BUF_SIZE> buf1;
  scoped_fixed_array<char, BUF_SIZE> buf2;
  scoped_fixed_array<char*, 64> c1;
  scoped_fixed_array<char*, 64> c2;

  std::strncpy(buf1.get(), f1, buf1.size());
  std::strncpy(buf2.get(), f2, buf2.size());

  const size_t n1 = tokenizeCSV(buf1.get(), c1.get(), c1.size());
  const size_t n2 = tokenizeCSV(buf2.get(), c2.get(), c2.size());
  const size_t n = std::min(n1, n2);

  for (size_t i = 0; i < n; ++i) {
    if (std::strcmp(c1[i], "*") != 0 && std::strcmp(c1[i], c2[i]) != 0) {
      return false;
    }
  }

  return true;
}

template <typename N>
bool is_valid_node(const Lattice* lattice, N* node) {
  const size_t end_pos = node->surface - lattice->sentence() + node->length;
  if (lattice->boundary_constraint(end_pos) == MECAB_INSIDE_TOKEN) {
    return false;
  }
  const size_t begin_pos = node->surface - lattice->sentence() + node->length - node->rlength;
  const char* feature = lattice->feature_constraint(begin_pos);
  if (!feature) {
    return true;
  }
  if (lattice->boundary_constraint(begin_pos) == MECAB_TOKEN_BOUNDARY &&
      lattice->boundary_constraint(end_pos) == MECAB_TOKEN_BOUNDARY && partial_match(feature, node->feature)) {
    return true;
  }
  return false;
}
}  // namespace

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

  Dictionary::result_type* mutable_results() { return results_.get(); }

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
        results_(new Dictionary::result_type[kResultsSize]) {}
  virtual ~Allocator() {}

 private:
  static const size_t kResultsSize = 512;
  size_t id_;
  scoped_ptr<FreeList<N>> node_freelist_;
  scoped_ptr<FreeList<P>> path_freelist_;
  scoped_ptr<ChunkFreeList<char>> char_freelist_;
  scoped_ptr<NBestGenerator> nbest_generator_;
  std::vector<char> partial_buffer_;
  scoped_array<Dictionary::result_type> results_;
};

template <typename N, typename P>
class Tokenizer {
 public:
  N* getBOSNode(Allocator<N, P>* allocator) const {
    N* bos_node = allocator->newNode();
    bos_node->surface = const_cast<const char*>(BOS_KEY);  // dummy
    bos_node->feature = bos_feature_.get();
    bos_node->isbest = 1;
    bos_node->stat = MECAB_BOS_NODE;
    return bos_node;
  }
  N* getEOSNode(Allocator<N, P>* allocator) const {
    N* eos_node = getBOSNode(allocator);  // same
    eos_node->stat = MECAB_EOS_NODE;
    return eos_node;
  }

  template <bool IsPartial>
  N* lookup(const char* begin, const char* end, Allocator<N, P>* allocator, Lattice* lattice) const {
    CharInfo cinfo;
    N* result_node = 0;
    size_t mblen = 0;
    size_t clen = 0;

    end = static_cast<size_t>(end - begin) >= 65535 ? begin + 65535 : end;

    if (IsPartial) {
      const size_t begin_pos = begin - lattice->sentence();
      for (size_t n = begin_pos + 1; n < lattice->size(); ++n) {
        if (lattice->boundary_constraint(n) == MECAB_TOKEN_BOUNDARY) {
          end = lattice->sentence() + n;
          break;
        }
      }
    }

    const char* begin2 = property_.seekToOtherType(begin, end, space_, &cinfo, &mblen, &clen);

    Dictionary::result_type* daresults = allocator->mutable_results();
    const size_t results_size = allocator->results_size();

    for (std::vector<Dictionary*>::const_iterator it = dic_.begin(); it != dic_.end(); ++it) {
      const size_t n = (*it)->commonPrefixSearch(begin2, static_cast<size_t>(end - begin2), daresults, results_size);
      for (size_t i = 0; i < n; ++i) {
        size_t size = (*it)->token_size(daresults[i]);
        const Token* token = (*it)->token(daresults[i]);
        for (size_t j = 0; j < size; ++j) {
          N* new_node = allocator->newNode();
          read_node_info(**it, *(token + j), &new_node);
          new_node->length = daresults[i].length;
          new_node->rlength = begin2 - begin + new_node->length;
          new_node->surface = begin2;
          new_node->stat = MECAB_NOR_NODE;
          new_node->char_type = cinfo.default_type;
          if (IsPartial && !is_valid_node(lattice, new_node)) {
            continue;
          }
          new_node->bnext = result_node;
          result_node = new_node;
        }
      }
    }

    if (result_node && !cinfo.invoke) {
      return result_node;
    }

    const char* begin3 = begin2 + mblen;
    const char* group_begin3 = 0;

    if (begin3 > end) {
      addUnknown<IsPartial>(begin, begin2, begin3, allocator, lattice, cinfo, &result_node);
      if (result_node) {
        return result_node;
      }
    }

    if (cinfo.group) {
      const char* tmp = begin3;
      CharInfo fail;
      begin3 = property_.seekToOtherType(begin3, end, cinfo, &fail, &mblen, &clen);
      if (clen <= max_grouping_size_) {
        addUnknown<IsPartial>(begin, begin2, begin3, allocator, lattice, cinfo, &result_node);
      }
      group_begin3 = begin3;
      begin3 = tmp;
    }

    for (size_t i = 1; i <= cinfo.length; ++i) {
      if (begin3 > end) {
        break;
      }
      if (begin3 == group_begin3) {
        continue;
      }
      clen = i;
      addUnknown<IsPartial>(begin, begin2, begin3, allocator, lattice, cinfo, &result_node);
      if (!cinfo.isKindOf(property_.getCharInfo(begin3, end, &mblen))) {
        break;
      }
      begin3 += mblen;
    }

    if (!result_node) {
      addUnknown<IsPartial>(begin, begin2, begin3, allocator, lattice, cinfo, &result_node);
    }

    if (IsPartial && !result_node) {
      begin3 = begin2;
      while (true) {
        cinfo = property_.getCharInfo(begin3, end, &mblen);
        begin3 += mblen;
        if (begin3 > end || lattice->boundary_constraint(begin3 - lattice->sentence()) != MECAB_INSIDE_TOKEN) {
          break;
        }
      }
      addUnknown<IsPartial>(begin, begin2, begin3, allocator, lattice, cinfo, &result_node);

      if (!result_node) {
        N* new_node = allocator->newNode();
        new_node->char_type = cinfo.default_type;
        new_node->surface = begin2;
        new_node->length = begin3 - begin2;
        new_node->rlength = begin3 - begin;
        new_node->stat = MECAB_UNK_NODE;
        new_node->bnext = result_node;
        new_node->feature = lattice->feature_constraint(begin - lattice->sentence());
        CHECK_DIE(new_node->feature);
        result_node = new_node;
      }
    }

    return result_node;
  }
  bool open(const Param& param) {
    close();

    const std::string prefix = param.template get<std::string>("dicdir");

    CHECK_FALSE(unkdic_.open(create_filename(prefix, UNK_DIC_FILE).c_str()));
    CHECK_FALSE(property_.open(param.get<std::string>("dicdir")));

    Dictionary* sysdic = new Dictionary;

    CHECK_FALSE(sysdic->open(create_filename(prefix, SYS_DIC_FILE).c_str()));

    CHECK_FALSE(sysdic->type() == 0) << "not a system dictionary: " << prefix;

    property_.set_charset(sysdic->charset());
    dic_.push_back(sysdic);

    const std::string userdic = param.template get<std::string>("userdic");
    if (!userdic.empty()) {
      scoped_fixed_array<char, BUF_SIZE> buf;
      scoped_fixed_array<char*, BUF_SIZE> dicfile;
      std::strncpy(buf.get(), userdic.c_str(), buf.size());
      const size_t n = tokenizeCSV(buf.get(), dicfile.get(), dicfile.size());
      for (size_t i = 0; i < n; ++i) {
        Dictionary* d = new Dictionary;
        CHECK_FALSE(d->open(dicfile[i]));
        CHECK_FALSE(d->type() == 1) << "not a user dictionary: " << dicfile[i];
        CHECK_FALSE(sysdic->isCompatible(*d)) << "incompatible dictionary: " << dicfile[i];
        dic_.push_back(d);
      }
    }

    dictionary_info_ = 0;
    dictionary_info_freelist_.free();
    for (int i = static_cast<int>(dic_.size() - 1); i >= 0; --i) {
      DictionaryInfo* d = dictionary_info_freelist_.alloc();
      d->next = dictionary_info_;
      d->filename = dic_[i]->filename();
      d->charset = dic_[i]->charset();
      d->size = dic_[i]->size();
      d->lsize = dic_[i]->lsize();
      d->rsize = dic_[i]->rsize();
      d->type = dic_[i]->type();
      d->version = dic_[i]->version();
      dictionary_info_ = d;
    }

    unk_tokens_.clear();
    for (size_t i = 0; i < property_.size(); ++i) {
      const char* key = property_.name(i);
      const Dictionary::result_type n = unkdic_.exactMatchSearch(key);
      CHECK_FALSE(n.value != -1) << "cannot find UNK category: " << key;
      const Token* token = unkdic_.token(n);
      size_t size = unkdic_.token_size(n);
      unk_tokens_.push_back(std::make_pair(token, size));
    }

    space_ = property_.getCharInfo(0x20);  // ad-hoc

    bos_feature_.reset_string(param.template get<std::string>("bos-feature"));

    const std::string tmp = param.template get<std::string>("unk-feature");
    unk_feature_.reset(0);
    if (!tmp.empty()) {
      unk_feature_.reset_string(tmp);
    }

    CHECK_FALSE(*bos_feature_ != '\0') << "bos-feature is undefined in dicrc";

    max_grouping_size_ = param.template get<size_t>("max-grouping-size");
    if (max_grouping_size_ == 0) {
      max_grouping_size_ = DEFAULT_MAX_GROUPING_SIZE;
    }

    return true;
  }
  void close() {
    for (std::vector<Dictionary*>::iterator it = dic_.begin(); it != dic_.end(); ++it) {
      delete *it;
    }
    dic_.clear();
    unk_tokens_.clear();
    property_.close();
  }
  const DictionaryInfo* dictionary_info() const { return const_cast<const DictionaryInfo*>(dictionary_info_); }

  explicit Tokenizer() : dictionary_info_freelist_(4), dictionary_info_(0), max_grouping_size_(0) {}
  virtual ~Tokenizer() { this->close(); }

 private:
  std::vector<Dictionary*> dic_;
  Dictionary unkdic_;
  scoped_string bos_feature_;
  scoped_string unk_feature_;
  FreeList<DictionaryInfo> dictionary_info_freelist_;
  std::vector<std::pair<const Token*, size_t>> unk_tokens_;
  DictionaryInfo* dictionary_info_;
  CharInfo space_;
  CharProperty property_;
  size_t max_grouping_size_;

  template <bool IsPartial>
  void addUnknown(const char* begin,
                  const char* begin2,
                  const char* begin3,
                  Allocator<N, P>* allocator,
                  const Lattice* lattice,
                  const CharInfo cinfo,
                  N** result_node) const {
    do {
      const Token* token = unk_tokens_[cinfo.default_type].first;
      size_t size = unk_tokens_[cinfo.default_type].second;
      for (size_t k = 0; k < size; ++k) {
        N* new_node = allocator->newNode();
        read_node_info(unkdic_, *(token + k), &new_node);
        new_node->char_type = cinfo.default_type;
        new_node->surface = begin2;
        new_node->length = begin3 - begin2;
        new_node->rlength = begin3 - begin;
        new_node->stat = MECAB_UNK_NODE;
        new_node->bnext = *result_node;
        if (unk_feature_.get())
          new_node->feature = unk_feature_.get();
        if (IsPartial && !is_valid_node(lattice, new_node)) {
          continue;
        }
        *result_node = new_node;
      }
    } while (0);
  }
};
}  // namespace MeCab
#endif  // _MECAB_TOKENIZER_H_
