#ifndef __MECAB_TAGGER_H__
#define __MECAB_TAGGER_H__

#include "mecab/lattice.h"
#include "mecab/model.h"

namespace MeCab {

/**
 * Tagger class
 */
class Tagger {
 public:
  Tagger() : current_model_(0), request_type_(MECAB_ONE_BEST), theta_(DEFAULT_THETA) {}
  ~Tagger() {}

  /**
   * Factory method to create a new Tagger with a specified main's argc/argv-style parameters.
   * Return NULL if new model cannot be initialized.
   * @return new Tagger object
   * @param argc number of parameters
   * @param argv parameter list
   */
  static Tagger* create(int argc, char** argv) {
    Tagger* tagger = new Tagger();
    if (!tagger->open(argc, argv)) {
      std::cerr << tagger->what();
      delete tagger;
      return 0;
    }
    return tagger;
  }

  /**
   * Factory method to create a new Tagger with a string parameter representation, i.e.,
   * "-d /user/local/mecab/dic/ipadic -Ochasen".
   * Return NULL if new model cannot be initialized.
   * @return new Model object
   * @param arg single string representation of the argment.
   */
  static Tagger* create(const char* argv) {
    Tagger* tagger = new Tagger();
    if (!tagger->open(argv)) {
      std::cerr << tagger->what();
      delete tagger;
      return 0;
    }
    return tagger;
  }

  static Tagger* create(const Model* model) {
    if (!model->is_available()) {
      std::cerr << "Model is not available";
      return 0;
    }
    Tagger* tagger = new Tagger;
    if (!tagger->open(*model)) {
      std::cerr << tagger->what();
      delete tagger;
      return 0;
    }
    tagger->set_theta(model->theta());
    tagger->set_request_type(model->request_type());
    return tagger;
  }

  /**
   * Return a version string
   * @return version string
   */
  static const char* version() { return VERSION; }

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

  /**
   * Handy static method.
   * Return true if lattice is parsed successfully.
   * This function is equivalent to
   * {
   *   Tagger *tagger = model.createModel();
   *   cosnt bool result = tagger->parse(lattice);
   *   delete tagger;
   *   return result;
   * }
   * @return boolean
   */
  static bool parse(const Model& model, Lattice* lattice) {
    scoped_ptr<Tagger> tagger(create(&model));
    return tagger->parse(lattice);
  }

  /**
   * Parse lattice object.
   * Return true if lattice is parsed successfully.
   * A sentence must be set to the lattice with Lattice:set_sentence object before calling this method.
   * Parsed node object can be obtained with Lattice:bos_node.
   * This method is thread safe.
   * @return lattice lattice object
   * @return boolean
   */
  bool parse(Lattice* lattice) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model()->viterbi()->analyze(lattice);
  }

  /**
   * Parse given sentence and return parsed result as string.
   * You should not delete the returned string. The returned buffer
   * is overwritten when parse method is called again.
   * This method is NOT thread safe.
   * @param str sentence
   * @return parsed result
   */
  const char* parse(const char* str) { return parse(str, std::strlen(str)); }

  /**
   * Parse given sentence and return Node object.
   * You should not delete the returned node object. The returned buffer
   * is overwritten when parse method is called again.
   * You can traverse all nodes via Node::next member.
   * This method is NOT thread safe.
   * @param str sentence
   * @return bos node object
   */
  const Node* parseToNode(const char* str) { return parseToNode(str, std::strlen(str)); }

  /**
   * Parse given sentence and obtain N-best results as a string format.
   * Currently, N must be 1 <= N <= 512 due to the limitation of the buffer size.
   * You should not delete the returned string. The returned buffer
   * is overwritten when parse method is called again.
   * This method is DEPRECATED. Use Lattice class.
   * @param N how many results you want to obtain
   * @param str sentence
   * @return parsed result
   */
  const char* parseNBest(size_t N, const char* str) { return parseNBest(N, str, std::strlen(str)); }

  /**
   * Initialize N-best enumeration with a sentence.
   * Return true if initialization finishes successfully.
   * N-best result is obtained by calling next() or nextNode() in sequence.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @param str sentence
   * @return boolean
   */
  bool parseNBestInit(const char* str) { return parseNBestInit(str, std::strlen(str)); }

  /**
   * Return next-best parsed result. You must call parseNBestInit() in advance.
   * Return NULL if no more reuslt is available.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @return node object
   */
  const Node* nextNode() {
    Lattice* lattice = mutable_lattice();
    if (!lattice->next()) {
      lattice->set_what("no more results");
      return 0;
    }
    return lattice->bos_node();
  }

  /**
   * Return next-best parsed result. You must call parseNBestInit() in advance.
   * Return NULL if no more reuslt is available.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @return parsed result
   */
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

  /**
   * Return formatted node object. The format is specified with
   * --unk-format, --bos-format, --eos-format, and --eon-format respectively.
   * You should not delete the returned string. The returned buffer
   * is overwritten when parse method is called again.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @param node node object.
   * @return parsed result
   */
  const char* formatNode(const Node* node) {
    // const char* result = mutable_lattice()->toString(node);
    const char* result = model()->writer()->stringifyLattice(mutable_lattice(), node);
    if (!result) {
      set_what(mutable_lattice()->what());
      return 0;
    }
    return result;
  }

  /**
   * The same as parse() method, but input length and output buffer are passed.
   * Return parsed result as string. The result pointer is the same as |ostr|.
   * Return NULL, if parsed result string cannot be stored within |olen| bytes.
   * @param str sentence
   * @param len sentence length
   * @param ostr output buffer
   * @param olen output buffer length
   * @return parsed result
   */
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
  /**
   * The same as parse() method, but input length can be passed.
   * @param str sentence
   * @param len sentence length
   * @return parsed result
   */
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

  /**
   * The same as parseToNode(), but input lenth can be passed.
   * @param str sentence
   * @param len sentence length
   * @return node object
   */
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

  /**
   * The same as parseNBest(), but input length can be passed.
   * @param N how many results you want to obtain
   * @param str sentence
   * @param len sentence length
   * @return parsed result
   */
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

  /**
   * The same as parseNBestInit(), but input length can be passed.
   * @param str sentence
   * @param len sentence length
   * @return boolean
   * @return parsed result
   */
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

  /**
   * The same as next(), but output buffer can be passed.
   * Return NULL if more than |olen| buffer is required to store output string.
   * @param ostr output buffer
   * @param olen output buffer length
   * @return parsed result
   */
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

  /**
   * The same as parseNBest(), but input length and output buffer can be passed.
   * Return NULL if more than |olen| buffer is required to store output string.
   * @param N how many results you want to obtain
   * @param str input sentence
   * @param len input sentence length
   * @param ostr output buffer
   * @param olen output buffer length
   * @return parsed result
   */
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

  /**
   * The same as formatNode(), but output buffer can be passed.
   * Return NULL if more than |olen| buffer is required to store output string.
   * @param node node object
   * @param ostr output buffer
   * @param olen output buffer length
   * @return parsed result
   */
  const char* formatNode(const Node* node, char* out, size_t len) {
    // const char* result = mutable_lattice()->toString(node, out, len);
    const char* result = model()->writer()->stringifyLattice(mutable_lattice(), node, out, len);
    if (!result) {
      set_what(mutable_lattice()->what());
      return 0;
    }
    return result;
  }

  /**
   * Set request type.
   * This method is DEPRECATED. Use Lattice::set_request_type(MECAB_PARTIAL).
   * @param request_type new request type assigned
   */
  void set_request_type(int request_type) { request_type_ = request_type; }

  /**
   * Return the current request type.
   * This method is DEPRECATED. Use Lattice class.
   * @return request type
   */
  int request_type() const { return request_type_; }

  /**
   * Return true if partial parsing mode is on.
   * This method is DEPRECATED. Use Lattice::has_request_type(MECAB_PARTIAL).
   * @return boolean
   */
  bool partial() const { return request_type_ & MECAB_PARTIAL; }

  /**
   * set partial parsing mode.
   * This method is DEPRECATED. Use Lattice::add_request_type(MECAB_PARTIAL) or
   * Lattice::remove_request_type(MECAB_PARTIAL)
   * @param partial partial mode
   */
  void set_partial(bool partial) {
    if (partial) {
      request_type_ |= MECAB_PARTIAL;
    } else {
      request_type_ &= ~MECAB_PARTIAL;
    }
  }

  /**
   * Return lattice level.
   * This method is DEPRECATED. Use Lattice::*_request_type()
   * @return int lattice level
   */
  int lattice_level() const {
    if (request_type_ & MECAB_MARGINAL_PROB) {
      return 2;
    } else if (request_type_ & MECAB_NBEST) {
      return 1;
    } else {
      return 0;
    }
  }

  /**
   * Set lattice level.
   * This method is DEPRECATED. Use Lattice::*_request_type()
   * @param level lattice level
   */
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

  /**
   * Return true if all morphs output mode is on.
   * This method is DEPRECATED. Use Lattice::has_request_type(MECAB_ALL_MORPHS).
   * @return boolean
   */
  bool all_morphs() const { return request_type_ & MECAB_ALL_MORPHS; }

  /**
   * set all-morphs output mode.
   * This method is DEPRECATED. Use Lattice::add_request_type(MECAB_ALL_MORPHS) or
   * Lattice::remove_request_type(MECAB_ALL_MORPHS)
   * @param all_morphs
   */
  void set_all_morphs(bool all_morphs) {
    if (all_morphs) {
      request_type_ |= MECAB_ALL_MORPHS;
    } else {
      request_type_ &= ~MECAB_ALL_MORPHS;
    }
  }

  /**
   * Set temparature parameter theta.
   * @param theta temparature parameter.
   */
  void set_theta(float theta) { theta_ = theta; }

  /**
   * Return temparature parameter theta.
   * @return temparature parameter.
   */
  float theta() const { return theta_; }

  /**
   * Return DictionaryInfo linked list.
   * @return DictionaryInfo linked list
   */
  const DictionaryInfo* dictionary_info() const { return model()->dictionary_info(); }

  /**
   * Return error string.
   * @return error string
   */
  const char* what() const { return what_.c_str(); }

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
}  // namespace MeCab

#endif  // __MECAB_TAGGER_H__
