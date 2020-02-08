#include <cstring>
#include <iostream>
#include <iterator>
#include <mutex>
#include <thread>

#include "mecab.h"
#include "mecab/common.h"
#include "mecab/connector.h"
#include "mecab/nbest_generator.h"
#include "mecab/tokenizer.h"
#include "mecab/utils/param.h"
#include "mecab/utils/scoped_ptr.h"
#include "mecab/utils/stream_wrapper.h"
#include "mecab/utils/string_buffer.h"
#include "mecab/utils/thread.h"
#include "mecab/viterbi.h"
#include "mecab/writer.h"

namespace {
const size_t kErrorBufferSize = 256;
}

namespace {
char kErrorBuffer[kErrorBufferSize];
}  // namespace

const char* getGlobalError() {
  return kErrorBuffer;
}

void setGlobalError(const char* str) {
  strncpy(kErrorBuffer, str, kErrorBufferSize - 1);
  kErrorBuffer[kErrorBufferSize - 1] = '\0';
}

namespace MeCab {
namespace {

class TaggerImpl : public Tagger {
 public:
  bool open(int argc, char** argv) {
    model_.reset(new Model);
    if (!model_->open(argc, argv)) {
      model_.reset(0);
      return false;
    }
    current_model_ = model_.get();
    request_type_ = model()->request_type();
    theta_ = model()->theta();
    return true;
  }
  bool open(const char* arg) {
    model_.reset(new Model);
    if (!model_->open(arg)) {
      model_.reset(0);
      return false;
    }
    current_model_ = model_.get();
    request_type_ = model()->request_type();
    theta_ = model()->theta();
    return true;
  }
  bool open(const Model& model) {
    if (!model.is_available()) {
      return false;
    }
    model_.reset(0);
    current_model_ = &model;
    request_type_ = current_model_->request_type();
    theta_ = current_model_->theta();
    return true;
  }

  bool parse(Lattice* lattice) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model()->viterbi()->analyze(lattice);
  }

  void set_request_type(int request_type) { request_type_ = request_type; }
  int request_type() const { return request_type_; }

  const char* parse(const char* str) { return parse(str, std::strlen(str)); }
  const char* parse(const char* str, size_t len) {
    Lattice* lattice = mutable_lattice();
    initRequestType();
    lattice->set_sentence(str, len);
    if (!parse(lattice)) {
      set_what(lattice->what());
      return 0;
    }
    // const char* result = lattice->toString();
    const char* result = model()->writer()->stringifyLattice(lattice);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }
  const char* parse(const char* str, size_t len, char* out, size_t len2) {
    Lattice* lattice = mutable_lattice();
    initRequestType();
    lattice->set_sentence(str, len);
    if (!parse(lattice)) {
      set_what(lattice->what());
      return 0;
    }
    // const char* result = lattice->toString(out, len2);
    const char* result = model()->writer()->stringifyLattice(lattice, out, len2);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }
  const Node* parseToNode(const char* str) { return parseToNode(str, std::strlen(str)); }
  const Node* parseToNode(const char* str, size_t len = 0) {
    Lattice* lattice = mutable_lattice();
    initRequestType();
    lattice->set_sentence(str, len);
    if (!parse(lattice)) {
      set_what(lattice->what());
      return 0;
    }
    return lattice->bos_node();
  }

  const char* parseNBest(size_t N, const char* str) { return parseNBest(N, str, std::strlen(str)); }
  const char* parseNBest(size_t N, const char* str, size_t len) {
    Lattice* lattice = mutable_lattice();
    initRequestType();
    lattice->add_request_type(MECAB_NBEST);
    lattice->set_sentence(str, len);

    if (!parse(lattice)) {
      set_what(lattice->what());
      return 0;
    }

    const char* result = model()->writer()->stringifyLatticeNBest(lattice, N);
    // const char* result = lattice->enumNBestAsString(N);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }
  const char* parseNBest(size_t N, const char* str, size_t len, char* out, size_t len2) {
    Lattice* lattice = mutable_lattice();
    initRequestType();
    lattice->add_request_type(MECAB_NBEST);
    lattice->set_sentence(str, len);

    if (!parse(lattice)) {
      set_what(lattice->what());
      return 0;
    }

    const char* result = model()->writer()->stringifyLatticeNBest(lattice, N, out, len2);
    // const char* result = lattice->enumNBestAsString(N, out, len2);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }

  bool parseNBestInit(const char* str) { return parseNBestInit(str, std::strlen(str)); }
  bool parseNBestInit(const char* str, size_t len) {
    Lattice* lattice = mutable_lattice();
    initRequestType();
    lattice->add_request_type(MECAB_NBEST);
    lattice->set_sentence(str, len);
    if (!parse(lattice)) {
      set_what(lattice->what());
      return false;
    }
    return true;
  }
  const Node* nextNode() {
    Lattice* lattice = mutable_lattice();
    if (!lattice->next()) {
      lattice->set_what("no more results");
      return 0;
    }
    return lattice->bos_node();
  }
  const char* next() {
    Lattice* lattice = mutable_lattice();
    if (!lattice->next()) {
      lattice->set_what("no more results");
      return 0;
    }
    // const char* result = lattice->toString();
    const char* result = model()->writer()->stringifyLattice(lattice);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }
  const char* next(char* out, size_t len2) {
    Lattice* lattice = mutable_lattice();
    if (!lattice->next()) {
      lattice->set_what("no more results");
      return 0;
    }
    // const char* result = lattice->toString(out, len2);
    const char* result = model()->writer()->stringifyLattice(lattice, out, len2);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }

  const char* formatNode(const Node* node) {
    // const char* result = mutable_lattice()->toString(node);
    const char* result = model()->writer()->stringifyLattice(mutable_lattice(), node);
    if (!result) {
      set_what(mutable_lattice()->what());
      return 0;
    }
    return result;
  }
  const char* formatNode(const Node* node, char* out, size_t len) {
    // const char* result = mutable_lattice()->toString(node, out, len);
    const char* result = model()->writer()->stringifyLattice(mutable_lattice(), node, out, len);
    if (!result) {
      set_what(mutable_lattice()->what());
      return 0;
    }
    return result;
  }

  const DictionaryInfo* dictionary_info() const { return model()->dictionary_info(); }

  void set_partial(bool partial) {
    if (partial) {
      request_type_ |= MECAB_PARTIAL;
    } else {
      request_type_ &= ~MECAB_PARTIAL;
    }
  }
  bool partial() const { return request_type_ & MECAB_PARTIAL; }
  void set_theta(float theta) { theta_ = theta; }
  float theta() const { return theta_; }
  void set_lattice_level(int level) {
    switch (level) {
      case 0:
        request_type_ |= MECAB_ONE_BEST;
        break;
      case 1:
        request_type_ |= MECAB_NBEST;
        break;
      case 2:
        request_type_ |= MECAB_MARGINAL_PROB;
        break;
      default:
        break;
    }
  }
  int lattice_level() const {
    if (request_type_ & MECAB_MARGINAL_PROB) {
      return 2;
    } else if (request_type_ & MECAB_NBEST) {
      return 1;
    } else {
      return 0;
    }
  }
  void set_all_morphs(bool all_morphs) {
    if (all_morphs) {
      request_type_ |= MECAB_ALL_MORPHS;
    } else {
      request_type_ &= ~MECAB_ALL_MORPHS;
    }
  }
  bool all_morphs() const { return request_type_ & MECAB_ALL_MORPHS; }

  const char* what() const { return what_.c_str(); }

  TaggerImpl() : current_model_(0), request_type_(MECAB_ONE_BEST), theta_(DEFAULT_THETA) {}
  virtual ~TaggerImpl() {}

 private:
  const Model* model() const { return current_model_; }

  void set_what(const char* str) { what_.assign(str); }

  void initRequestType() {
    mutable_lattice()->set_request_type(request_type_);
    mutable_lattice()->set_theta(theta_);
  }

  Lattice* mutable_lattice() {
    if (!lattice_.get()) {
      lattice_.reset(model()->createLattice());
    }
    return lattice_.get();
  }

  const Model* current_model_;
  mutable std::mutex mutex_;

  scoped_ptr<Model> model_;
  scoped_ptr<Lattice> lattice_;
  int request_type_;
  double theta_;
  std::string what_;
};

}  // namespace

Tagger* Tagger::create(int argc, char** argv) {
  return createTagger(argc, argv);
}

Tagger* Tagger::create(const char* arg) {
  return createTagger(arg);
}

const char* Tagger::version() {
  return VERSION;
}

Tagger* createTagger(int argc, char** argv) {
  TaggerImpl* tagger = new TaggerImpl();
  if (!tagger->open(argc, argv)) {
    setGlobalError(tagger->what());
    delete tagger;
    return 0;
  }
  return tagger;
}

Tagger* createTagger(const char* argv) {
  TaggerImpl* tagger = new TaggerImpl();
  if (!tagger->open(argv)) {
    setGlobalError(tagger->what());
    delete tagger;
    return 0;
  }
  return tagger;
}

Tagger* createTagger(const Model* model) {
  if (!model->is_available()) {
    setGlobalError("Model is not available");
    return 0;
  }
  TaggerImpl* tagger = new TaggerImpl;
  if (!tagger->open(*model)) {
    setGlobalError(tagger->what());
    delete tagger;
    return 0;
  }
  tagger->set_theta(model->theta());
  tagger->set_request_type(model->request_type());
  return tagger;
}

const char* getLastError() {
  return getGlobalError();
}

bool Tagger::parse(const Model& model, Lattice* lattice) {
  scoped_ptr<Tagger> tagger(createTagger(&model));
  return tagger->parse(lattice);
}
}  // namespace MeCab
