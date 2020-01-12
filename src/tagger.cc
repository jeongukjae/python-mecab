#include <cstring>
#include <iostream>
#include <iterator>
#include <mutex>
#include <thread>

#include "mecab.h"
#include "mecab/common.h"
#include "mecab/connector.h"
#include "mecab/nbest_generator.h"
#include "mecab/param.h"
#include "mecab/scoped_ptr.h"
#include "mecab/stream_wrapper.h"
#include "mecab/string_buffer.h"
#include "mecab/thread.h"
#include "mecab/tokenizer.h"
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

const float kDefaultTheta = 0.75;

const std::vector<MeCab::Option> long_options{
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
    if (!param.parse(argc, argv, long_options) || !load_dictionary_resource(&param)) {
      return false;
    }
    return open(param);
  }

  bool open(const char* arg) {
    Param param;
    if (!param.parse(arg, long_options) || !load_dictionary_resource(&param)) {
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

  TaggerImpl() : current_model_(0), request_type_(MECAB_ONE_BEST), theta_(kDefaultTheta) {}
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

class LatticeImpl : public Lattice {
 public:
  explicit LatticeImpl(const Writer* writer = 0)
      : sentence_(0),
        size_(0),
        theta_(kDefaultTheta),
        Z_(0.0),
        request_type_(MECAB_ONE_BEST),
        writer_(writer),
        ostrs_(0),
        allocator_(new Allocator<Node, Path>) {
    begin_nodes_.reserve(MIN_INPUT_BUFFER_SIZE);
    end_nodes_.reserve(MIN_INPUT_BUFFER_SIZE);
  }
  ~LatticeImpl() {}

  // clear internal lattice
  void clear() {
    allocator_->free();
    if (ostrs_.get()) {
      ostrs_->clear();
    }
    begin_nodes_.clear();
    end_nodes_.clear();
    feature_constraint_.clear();
    boundary_constraint_.clear();
    size_ = 0;
    theta_ = kDefaultTheta;
    Z_ = 0.0;
    sentence_ = 0;
  }

  bool is_available() const { return (sentence_ && !begin_nodes_.empty() && !end_nodes_.empty()); }

  // nbest;
  bool next() {
    if (!has_request_type(MECAB_NBEST)) {
      set_what("MECAB_NBEST request type is not set");
      return false;
    }

    if (!allocator()->nbest_generator()->next()) {
      return false;
    }

    Viterbi::buildResultForNBest(this);
    return true;
  }

  // return bos/eos node
  Node* bos_node() const { return end_nodes_[0]; }
  Node* eos_node() const { return begin_nodes_[size()]; }
  Node** begin_nodes() const { return const_cast<Node**>(&begin_nodes_[0]); }
  Node** end_nodes() const { return const_cast<Node**>(&end_nodes_[0]); }
  Node* begin_nodes(size_t pos) const { return begin_nodes_[pos]; }
  Node* end_nodes(size_t pos) const { return end_nodes_[pos]; }

  const char* sentence() const { return sentence_; }
  void set_sentence(const char* sentence) { return set_sentence(sentence, strlen(sentence)); }
  void set_sentence(const char* sentence, size_t len) {
    clear();
    end_nodes_.resize(len + 4);
    begin_nodes_.resize(len + 4);

    if (has_request_type(MECAB_ALLOCATE_SENTENCE) || has_request_type(MECAB_PARTIAL)) {
      char* new_sentence = allocator()->strdup(sentence, len);
      sentence_ = new_sentence;
    } else {
      sentence_ = sentence;
    }

    size_ = len;
    std::memset(&end_nodes_[0], 0, sizeof(end_nodes_[0]) * (len + 4));
    std::memset(&begin_nodes_[0], 0, sizeof(begin_nodes_[0]) * (len + 4));
  }
  size_t size() const { return size_; }

  void set_Z(double Z) { Z_ = Z; }
  double Z() const { return Z_; }

  float theta() const { return theta_; }
  void set_theta(float theta) { theta_ = theta; }

  int request_type() const { return request_type_; }

  void set_request_type(int request_type) { request_type_ = request_type; }
  bool has_request_type(int request_type) const { return request_type & request_type_; }
  void add_request_type(int request_type) { request_type_ |= request_type; }
  void remove_request_type(int request_type) { request_type_ &= ~request_type; }

  Allocator<Node, Path>* allocator() const { return allocator_.get(); }

  Node* newNode() { return allocator_->newNode(); }

  bool has_constraint() const { return !boundary_constraint_.empty(); }
  int boundary_constraint(size_t pos) const {
    if (!boundary_constraint_.empty()) {
      return boundary_constraint_[pos];
    }
    return MECAB_ANY_BOUNDARY;
  }
  const char* feature_constraint(size_t begin_pos) const {
    if (!feature_constraint_.empty()) {
      return feature_constraint_[begin_pos];
    }
    return 0;
  }

  void set_boundary_constraint(size_t pos, int boundary_constraint_type) {
    if (boundary_constraint_.empty()) {
      boundary_constraint_.resize(size() + 4, MECAB_ANY_BOUNDARY);
    }
    boundary_constraint_[pos] = boundary_constraint_type;
  }

  void set_feature_constraint(size_t begin_pos, size_t end_pos, const char* feature) {
    if (begin_pos >= end_pos || !feature) {
      return;
    }

    if (feature_constraint_.empty()) {
      feature_constraint_.resize(size() + 4, 0);
    }

    end_pos = std::min(end_pos, size());

    set_boundary_constraint(begin_pos, MECAB_TOKEN_BOUNDARY);
    set_boundary_constraint(end_pos, MECAB_TOKEN_BOUNDARY);
    for (size_t i = begin_pos + 1; i < end_pos; ++i) {
      set_boundary_constraint(i, MECAB_INSIDE_TOKEN);
    }

    feature_constraint_[begin_pos] = feature;
  }

  void set_result(const char* result) {
    char* str = allocator()->strdup(result, std::strlen(result));
    std::vector<char*> lines;
    const size_t lsize = tokenize(str, "\n", std::back_inserter(lines), std::strlen(result));
    CHECK_DIE(lsize == lines.size());

    std::string sentence;
    std::vector<std::string> surfaces, features;
    for (size_t i = 0; i < lines.size(); ++i) {
      if (::strcmp("EOS", lines[i]) == 0) {
        break;
      }
      char* cols[2];
      if (tokenize(lines[i], "\t", cols, 2) != 2) {
        break;
      }
      sentence += cols[0];
      surfaces.push_back(cols[0]);
      features.push_back(cols[1]);
    }

    CHECK_DIE(features.size() == surfaces.size());

    set_sentence(allocator()->strdup(sentence.c_str(), sentence.size()));

    Node* bos_node = allocator()->newNode();
    bos_node->surface = const_cast<const char*>(BOS_KEY);  // dummy
    bos_node->feature = "BOS/EOS";
    bos_node->isbest = 1;
    bos_node->stat = MECAB_BOS_NODE;

    Node* eos_node = allocator()->newNode();
    eos_node->surface = const_cast<const char*>(BOS_KEY);  // dummy
    eos_node->feature = "BOS/EOS";
    eos_node->isbest = 1;
    eos_node->stat = MECAB_EOS_NODE;

    bos_node->surface = sentence_;
    end_nodes_[0] = bos_node;

    size_t offset = 0;
    Node* prev = bos_node;
    for (size_t i = 0; i < surfaces.size(); ++i) {
      Node* node = allocator()->newNode();
      node->prev = prev;
      prev->next = node;
      node->surface = sentence_ + offset;
      node->length = surfaces[i].size();
      node->rlength = surfaces[i].size();
      node->isbest = 1;
      node->stat = MECAB_NOR_NODE;
      node->wcost = 0;
      node->cost = 0;
      node->feature = allocator()->strdup(features[i].c_str(), features[i].size());
      begin_nodes_[offset] = node;
      end_nodes_[offset + node->length] = node;
      offset += node->length;
      prev = node;
    }

    prev->next = eos_node;
    eos_node->prev = prev;
  }

  const char* what() const { return what_.c_str(); }

  void set_what(const char* str) { what_.assign(str); }

  const char* toString() { return toStringInternal(stream()); }
  const char* toString(char* buf, size_t size) {
    StringBuffer os(buf, size);
    return toStringInternal(&os);
  }
  const char* toString(const Node* node) { return toStringInternal(node, stream()); }
  const char* toString(const Node* node, char* buf, size_t size) {
    StringBuffer os(buf, size);
    return toStringInternal(node, &os);
  }
  const char* enumNBestAsString(size_t N) { return enumNBestAsStringInternal(N, stream()); }
  const char* enumNBestAsString(size_t N, char* buf, size_t size) {
    StringBuffer os(buf, size);
    return enumNBestAsStringInternal(N, &os);
  }

 private:
  const char* sentence_;
  size_t size_;
  double theta_;
  double Z_;
  int request_type_;
  std::string what_;
  std::vector<Node*> end_nodes_;
  std::vector<Node*> begin_nodes_;
  std::vector<const char*> feature_constraint_;
  std::vector<unsigned char> boundary_constraint_;
  const Writer* writer_;
  scoped_ptr<StringBuffer> ostrs_;
  scoped_ptr<Allocator<Node, Path>> allocator_;

  StringBuffer* stream() {
    if (!ostrs_.get()) {
      ostrs_.reset(new StringBuffer);
    }
    return ostrs_.get();
  }

  const char* toStringInternal(StringBuffer* os) {
    os->clear();
    if (writer_) {
      if (!writer_->write(this, os)) {
        return 0;
      }
    } else {
      writeLattice(os);
    }
    *os << '\0';
    if (!os->str()) {
      set_what("output buffer overflow");
      return 0;
    }
    return os->str();
  }
  const char* toStringInternal(const Node* node, StringBuffer* os) {
    os->clear();
    if (!node) {
      set_what("node is NULL");
      return 0;
    }
    if (writer_) {
      if (!writer_->writeNode(this, node, os)) {
        return 0;
      }
    } else {
      os->write(node->surface, node->length);
      *os << '\t' << node->feature;
    }
    *os << '\0';
    if (!os->str()) {
      set_what("output buffer overflow");
      return 0;
    }
    return os->str();
  }
  const char* enumNBestAsStringInternal(size_t N, StringBuffer* os) {
    os->clear();

    if (N == 0 || N > NBEST_MAX) {
      set_what("nbest size must be 1 <= nbest <= 512");
      return 0;
    }

    for (size_t i = 0; i < N; ++i) {
      if (!next()) {
        break;
      }
      if (writer_) {
        if (!writer_->write(this, os)) {
          return 0;
        }
      } else {
        writeLattice(os);
      }
    }

    // make a dummy node for EON
    if (writer_) {
      Node eon_node;
      memset(&eon_node, 0, sizeof(eon_node));
      eon_node.stat = MECAB_EON_NODE;
      eon_node.next = 0;
      eon_node.surface = this->sentence() + this->size();
      if (!writer_->writeNode(this, &eon_node, os)) {
        return 0;
      }
    }
    *os << '\0';

    if (!os->str()) {
      set_what("output buffer overflow");
      return 0;
    }

    return os->str();
  }
  void writeLattice(StringBuffer* os) {
    for (const Node* node = this->bos_node()->next; node->next; node = node->next) {
      os->write(node->surface, node->length);
      *os << '\t' << node->feature;
      *os << '\n';
    }
    *os << "EOS\n";
  }
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
  return new LatticeImpl(writer_.get());
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

void deleteTagger(Tagger* tagger) {
  delete tagger;
}

const char* getTaggerError() {
  return getLastError();
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

void deleteModel(Model* model) {
  delete model;
}

Model* Model::create(int argc, char** argv) {
  return createModel(argc, argv);
}

Model* Model::create(const char* arg) {
  return createModel(arg);
}

const char* Model::version() {
  return VERSION;
}

bool Tagger::parse(const Model& model, Lattice* lattice) {
  scoped_ptr<Tagger> tagger(model.createTagger());
  return tagger->parse(lattice);
}

Lattice* Lattice::create() {
  return createLattice();
}

Lattice* createLattice() {
  return new LatticeImpl;
}

void deleteLattice(Lattice* lattice) {
  delete lattice;
}
}  // namespace MeCab

int mecab_do(int argc, char** argv) {
#define WHAT_ERROR(msg)            \
  do {                             \
    std::cout << msg << std::endl; \
    return EXIT_FAILURE;           \
  } while (0);

  MeCab::Param param;
  if (!param.parse(argc, argv, MeCab::long_options)) {
    return EXIT_FAILURE;
  }

  if (param.get<bool>("help")) {
    std::cout << param.getHelpMessage() << std::endl;
    return EXIT_SUCCESS;
  }

  if (param.get<bool>("version")) {
    std::cout << param.getVersionMessage() << std::endl;
    return EXIT_SUCCESS;
  }

  if (!load_dictionary_resource(&param)) {
    return EXIT_SUCCESS;
  }

  if (param.get<int>("lattice-level") >= 1) {
    std::cerr << "lattice-level is DEPERCATED. "
              << "use --marginal or --nbest." << std::endl;
  }

  MeCab::scoped_ptr<MeCab::ModelImpl> model(new MeCab::ModelImpl);
  if (!model->open(param)) {
    std::cout << MeCab::getLastError() << std::endl;
    return EXIT_FAILURE;
  }

  std::string ofilename = param.get<std::string>("output");
  if (ofilename.empty()) {
    ofilename = "-";
  }

  const int nbest = param.get<int>("nbest");
  if (nbest <= 0 || nbest > NBEST_MAX) {
    WHAT_ERROR("invalid N value");
  }

  MeCab::ostream_wrapper ofs(ofilename.c_str());
  if (!*ofs) {
    WHAT_ERROR("no such file or directory: " << ofilename);
  }

  if (param.get<bool>("dump-config")) {
    param.dumpConfig(*ofs);
    return EXIT_FAILURE;
  }

  if (param.get<bool>("dictionary-info")) {
    for (const MeCab::DictionaryInfo* d = model->dictionary_info(); d; d = d->next) {
      *ofs << "filename:\t" << d->filename << std::endl;
      *ofs << "version:\t" << d->version << std::endl;
      *ofs << "charset:\t" << d->charset << std::endl;
      *ofs << "type:\t" << d->type << std::endl;
      *ofs << "size:\t" << d->size << std::endl;
      *ofs << "left size:\t" << d->lsize << std::endl;
      *ofs << "right size:\t" << d->rsize << std::endl;
      *ofs << std::endl;
    }
    return EXIT_FAILURE;
  }

  const std::vector<std::string>& rest_ = param.getRestParameters();
  std::vector<std::string> rest = rest_;

  if (rest.empty()) {
    rest.push_back("-");
  }

  size_t ibufsize =
      std::min(MAX_INPUT_BUFFER_SIZE, std::max(param.get<int>("input-buffer-size"), MIN_INPUT_BUFFER_SIZE));

  const bool partial = param.get<bool>("partial");
  if (partial) {
    ibufsize *= 8;
  }

  MeCab::scoped_array<char> ibuf_data(new char[ibufsize]);
  char* ibuf = ibuf_data.get();

  MeCab::scoped_ptr<MeCab::Tagger> tagger(model->createTagger());

  if (!tagger.get()) {
    WHAT_ERROR("cannot create tagger");
  }

  for (size_t i = 0; i < rest.size(); ++i) {
    MeCab::istream_wrapper ifs(rest[i].c_str());
    if (!*ifs) {
      WHAT_ERROR("no such file or directory: " << rest[i]);
    }

    while (true) {
      if (!partial) {
        ifs->getline(ibuf, ibufsize);
      } else {
        std::string sentence;
        MeCab::scoped_fixed_array<char, BUF_SIZE> line;
        for (;;) {
          if (!ifs->getline(line.get(), line.size())) {
            ifs->clear(std::ios::eofbit | std::ios::badbit);
            break;
          }
          sentence += line.get();
          sentence += '\n';
          if (std::strcmp(line.get(), "EOS") == 0 || line[0] == '\0') {
            break;
          }
        }
        std::strncpy(ibuf, sentence.c_str(), ibufsize);
      }
      if (ifs->eof() && !ibuf[0]) {
        return false;
      }
      if (ifs->fail()) {
        std::cerr << "input-buffer overflow. "
                  << "The line is split. use -b #SIZE option." << std::endl;
        ifs->clear();
      }
      const char* r = (nbest >= 2) ? tagger->parseNBest(nbest, ibuf) : tagger->parse(ibuf);
      if (!r) {
        WHAT_ERROR(tagger->what());
      }
      *ofs << r << std::flush;
    }
  }

  return EXIT_SUCCESS;

#undef WHAT_ERROR
}
