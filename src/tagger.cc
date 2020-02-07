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

const std::vector<MeCab::Option> mecab_options{
    {"rcfile", 'r', "", "FILE", "use FILE as resource file"},
    {"dicdir", 'd', "", "DIR", "set DIR  as a system dicdir"},
    {"userdic", 'u', "", "FILE", "use FILE as a user dictionary"},
    {"lattice-level", 'l', "0", "INT", "lattice information level (DEPRECATED)"},
    {"dictionary-info", 'D', "", "", "show dictionary information and exit"},
    {"output-format-type", 'O', "", "TYPE", "set output format type (wakati,none,...)"},
    {"all-morphs", 'a', "", "", "output all morphs(default false)"},
    {"nbest", 'N', "1", "INT", "output N best results (default 1)"},
    {"partial", 'p', "", "", "partial parsing mode (default false)"},
    {"marginal", 'm', "", "", "output marginal probability (default false)"},
    {"max-grouping-size", 'M', "24", "INT", "maximum grouping size for unknown words (default 24)"},
    {"node-format", 'F', "%m\\t%H\\n", "STR", "use STR as the user-defined node format"},
    {"unk-format", 'U', "%m\\t%H\\n", "STR", "use STR as the user-defined unknown node format"},
    {"bos-format", 'B', "", "STR", "use STR as the user-defined beginning-of-sentence format"},
    {"eos-format", 'E', "EOS\\n", "STR", "use STR as the user-defined end-of-sentence format"},
    {"eon-format", 'S', "", "STR", "use STR as the user-defined end-of-NBest format"},
    {"unk-feature", 'x', "", "STR", "use STR as the feature for unknown word"},
    {"input-buffer-size", 'b', "", "INT", "set input buffer size (default 8192)"},
    {"dump-config", 'P', "", "", "dump MeCab parameters"},
    {"allocate-sentence", 'C', "", "", "allocate new memory for input sentence"},
    {"theta", 't', "0.75", "FLOAT", "set temparature parameter theta (default 0.75)"},
    {"cost-factor", 'c', "700", "INT", "set cost factor (default 700)"},
    {"output", 'o', "", "FILE", "set the output file name"}};

class ModelImpl : public Model {
 public:
  ModelImpl() : viterbi_(new Viterbi), writer_(new Writer), request_type_(MECAB_ONE_BEST), theta_(0.0) {}
  virtual ~ModelImpl() {
    delete viterbi_;
    viterbi_ = 0;
  }

  bool open(int argc, char** argv) {
    Param param;
    if (!param.parse(argc, argv, mecab_options) || !load_dictionary_resource(&param)) {
      return false;
    }
    return open(param);
  }

  bool open(const char* arg) {
    Param param;
    if (!param.parse(arg, mecab_options) || !load_dictionary_resource(&param)) {
      return false;
    }
    return open(param);
  }

  bool open(const Param& param) {
    CHECK_FALSE(writer_->open(param) && viterbi_->open(param));

    request_type_ = load_request_type(param);
    theta_ = param.get<double>("theta");

    return is_available();
  }

  bool swap(Model* model) {
    scoped_ptr<Model> model_data(model);

    if (!is_available()) {
      setGlobalError("current model is not available");
      return false;
    }
    ModelImpl* m = static_cast<ModelImpl*>(model_data.get());
    if (!m) {
      setGlobalError("Invalid model is passed");
      return false;
    }

    if (!m->is_available()) {
      setGlobalError("Passed model is not available");
      return false;
    }

    Viterbi* current_viterbi = viterbi_;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      viterbi_ = m->take_viterbi();
      request_type_ = m->request_type();
      theta_ = m->theta();
    }

    delete current_viterbi;

    return true;
  }

  bool is_available() const { return (viterbi_ && writer_.get()); }

  int request_type() const { return request_type_; }

  double theta() const { return theta_; }

  const DictionaryInfo* dictionary_info() const {
    return viterbi_->tokenizer() ? viterbi_->tokenizer()->dictionary_info() : 0;
  }

  int transition_cost(unsigned short rcAttr, unsigned short lcAttr) const {
    return viterbi_->connector()->transition_cost(rcAttr, lcAttr);
  }

  Node* lookup(const char* begin, const char* end, Lattice* lattice) const {
    return viterbi_->tokenizer()->lookup<false>(begin, end, lattice->allocator(), lattice);
  }

  Tagger* createTagger() const;

  Lattice* createLattice() const;

  const Viterbi* viterbi() const { return viterbi_; }

  // moves the owership.
  Viterbi* take_viterbi() {
    Viterbi* result = viterbi_;
    viterbi_ = 0;
    return result;
  }

  const Writer* writer() const { return writer_.get(); }

 private:
  Viterbi* viterbi_;
  std::mutex mutex_;
  scoped_ptr<Writer> writer_;
  int request_type_;
  double theta_;
};

class TaggerImpl : public Tagger {
 public:
  bool open(int argc, char** argv) {
    model_.reset(new ModelImpl);
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
    model_.reset(new ModelImpl);
    if (!model_->open(arg)) {
      model_.reset(0);
      return false;
    }
    current_model_ = model_.get();
    request_type_ = model()->request_type();
    theta_ = model()->theta();
    return true;
  }
  bool open(const ModelImpl& model) {
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
    const char* result = lattice->toString();
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
    const char* result = lattice->toString(out, len2);
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

    const char* result = lattice->enumNBestAsString(N);
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

    const char* result = lattice->enumNBestAsString(N, out, len2);
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
    const char* result = lattice->toString();
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
    const char* result = lattice->toString(out, len2);
    if (!result) {
      set_what(lattice->what());
      return 0;
    }
    return result;
  }

  const char* formatNode(const Node* node) {
    const char* result = mutable_lattice()->toString(node);
    if (!result) {
      set_what(mutable_lattice()->what());
      return 0;
    }
    return result;
  }
  const char* formatNode(const Node* node, char* out, size_t len) {
    const char* result = mutable_lattice()->toString(node, out, len);
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
  const ModelImpl* model() const { return current_model_; }

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

  const ModelImpl* current_model_;
  mutable std::mutex mutex_;

  scoped_ptr<ModelImpl> model_;
  scoped_ptr<Lattice> lattice_;
  int request_type_;
  double theta_;
  std::string what_;
};

Tagger* ModelImpl::createTagger() const {
  if (!is_available()) {
    setGlobalError("Model is not available");
    return 0;
  }
  TaggerImpl* tagger = new TaggerImpl;
  if (!tagger->open(*this)) {
    setGlobalError(tagger->what());
    delete tagger;
    return 0;
  }
  tagger->set_theta(theta_);
  tagger->set_request_type(request_type_);
  return tagger;
}

Lattice* ModelImpl::createLattice() const {
  if (!is_available()) {
    setGlobalError("Model is not available");
    return 0;
  }
  return new Lattice(writer_.get());
}
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

const char* getLastError() {
  return getGlobalError();
}

Model* createModel(int argc, char** argv) {
  ModelImpl* model = new ModelImpl;
  if (!model->open(argc, argv)) {
    delete model;
    return 0;
  }
  return model;
}

Model* createModel(const char* arg) {
  ModelImpl* model = new ModelImpl;
  if (!model->open(arg)) {
    delete model;
    return 0;
  }
  return model;
}

Model* createModel(const Param& param) {
  ModelImpl* model = new ModelImpl;
  if (!model->open(param)) {
    delete model;
    return 0;
  }
  return model;
}

Model* Model::create(int argc, char** argv) {
  return createModel(argc, argv);
}

Model* Model::create(const char* arg) {
  return createModel(arg);
}

Model* Model::create(const Param& param) {
  return createModel(param);
}

const char* Model::version() {
  return VERSION;
}

bool Tagger::parse(const Model& model, Lattice* lattice) {
  scoped_ptr<Tagger> tagger(model.createTagger());
  return tagger->parse(lattice);
}
}  // namespace MeCab
