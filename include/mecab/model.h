#ifndef __MECAB_MODEL_H__
#define __MECAB_MODEL_H__

#include <mutex>
#include <vector>

#include "mecab/data_structure.h"
#include "mecab/lattice.h"
#include "mecab/utils/param.h"
#include "mecab/viterbi.h"
#include "mecab/writer.h"

namespace MeCab {

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

/**
 * Model class
 */
class Model {
 public:
  Model() : viterbi_(new Viterbi), writer_(new Writer), request_type_(MECAB_ONE_BEST), theta_(0.0) {}
  ~Model() {
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

  /**
   * Return a version string
   * @return version string
   */
  static const char* version() { return VERSION; }

  /**
   * Factory method to create a new Model with a specified main's argc/argv-style parameters.
   * Return NULL if new model cannot be initialized.
   * @return new Model object
   * @param argc number of parameters
   * @param argv parameter list
   */
  static Model* create(int argc, char** argv) {
    Model* model = new Model;
    if (!model->open(argc, argv)) {
      delete model;
      return 0;
    }
    return model;
  }

  /**
   * Factory method to create a new Model with a string parameter representation, i.e.,
   * "-d /user/local/mecab/dic/ipadic -Ochasen".
   * Return NULL if new model cannot be initialized.
   * @return new Model object
   * @param arg single string representation of the argment.
   */
  static Model* create(const char* arg) {
    Model* model = new Model;
    if (!model->open(arg)) {
      delete model;
      return 0;
    }
    return model;
  }

  static Model* create(const Param& param) {
    Model* model = new Model;
    if (!model->open(param)) {
      delete model;
      return 0;
    }
    return model;
  }

  /**
   * Return DictionaryInfo linked list.
   * @return DictionaryInfo linked list
   */
  const DictionaryInfo* dictionary_info() const {
    return viterbi_->tokenizer() ? viterbi_->tokenizer()->dictionary_info() : 0;
  }

  /**
   * Return transtion cost from rcAttr to lcAttr.
   * @return transtion cost
   */
  int transition_cost(unsigned short rcAttr, unsigned short lcAttr) const {
    return viterbi_->connector()->transition_cost(rcAttr, lcAttr);
  }

  /**
   * perform common prefix search from the range [begin, end).
   * |lattice| takes the ownership of return value.
   * @return node linked list.
   */
  Node* lookup(const char* begin, const char* end, Lattice* lattice) const {
    return viterbi_->tokenizer()->lookup<false>(begin, end, lattice->allocator(), lattice);
  }

  /**
   * Create a new Lattice object.
   * @return new Lattice object
   */
  Lattice* createLattice() const {
    if (!is_available()) {
      std::cerr << "Model is not available";
      return 0;
    }
    return new Lattice;
  }

  /**
   * Swap the instance with |model|.
   * The ownership of |model| always moves to this instance,
   * meaning that passed |model| will no longer be accessible after calling this method.
   * return true if new model is swapped successfully.
   * This method is thread safe. All taggers created by
   * Model::createTagger() method will also be updated asynchronously.
   * No need to stop the parsing thread excplicitly before swapping model object.
   * @return boolean
   * @param model new model which is going to be swapped with the current model.
   */
  bool swap(Model* model) {
    scoped_ptr<Model> model_data(model);

    if (!is_available()) {
      std::cerr << "current model is not available";
      return false;
    }
    Model* m = static_cast<Model*>(model_data.get());
    if (!m) {
      std::cerr << "Invalid model is passed";
      return false;
    }

    if (!m->is_available()) {
      std::cerr << "Passed model is not available";
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

  // moves the owership.
  Viterbi* take_viterbi() {
    Viterbi* result = viterbi_;
    viterbi_ = 0;
    return result;
  }

  bool is_available() const { return (viterbi_ && writer_.get()); }

  int request_type() const { return request_type_; }

  double theta() const { return theta_; }

  const Viterbi* viterbi() const { return viterbi_; }

  const Writer* writer() const { return writer_.get(); }

 private:
  Viterbi* viterbi_;
  std::mutex mutex_;
  scoped_ptr<Writer> writer_;
  int request_type_;
  double theta_;
};

}  // namespace MeCab

#endif  // __MECAB_MODEL_H__
