#ifndef __MECAB_LATTICE_H__
#define __MECAB_LATTICE_H__

#include <string>
#include <vector>

#include "mecab/allocator.h"
#include "mecab/common.h"
#include "mecab/data_structure.h"
#include "mecab/utils/string_buffer.h"

namespace MeCab {

/**
 * Lattice class
 */
class Lattice {
 public:
  Lattice(const Writer* writer = 0)
      : sentence_(0),
        size_(0),
        theta_(DEFAULT_THETA),
        Z_(0.0),
        request_type_(MECAB_ONE_BEST),
        writer_(writer),
        ostrs_(0),
        allocator_(new Allocator<Node, Path>) {
    begin_nodes_.reserve(MIN_INPUT_BUFFER_SIZE);
    end_nodes_.reserve(MIN_INPUT_BUFFER_SIZE);
  }
  ~Lattice() {}

  /**
   * Clear all internal lattice data.
   */
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
    theta_ = DEFAULT_THETA;
    Z_ = 0.0;
    sentence_ = 0;
  };

  /**
   * Return true if result object is available.
   * @return boolean
   */
  bool is_available() const { return (sentence_ && !begin_nodes_.empty() && !end_nodes_.empty()); };

  /**
   * Return bos (begin of sentence) node.
   * You can obtain all nodes via "for (const Node *node = lattice->bos_node(); node; node = node->next) {}"
   * @return bos node object
   */
  Node* bos_node() const { return end_nodes_[0]; };

  /**
   * Return eos (end of sentence) node.
   * @return eos node object
   */
  Node* eos_node() const { return begin_nodes_[size()]; }

  /**
   * This method is used internally.
   */
  Node** begin_nodes() const { return const_cast<Node**>(&begin_nodes_[0]); }

  /**
   * This method is used internally.
   */
  Node** end_nodes() const { return const_cast<Node**>(&end_nodes_[0]); }

  /**
   * Return node linked list ending at |pos|.
   * You can obtain all nodes via "for (const Node *node = lattice->end_nodes(pos); node; node = node->enext) {}"
   * @param pos position of nodes. 0 <= pos < size()
   * @return node linked list
   */
  Node* end_nodes(size_t pos) const { return end_nodes_[pos]; }

  /**
   * Return node linked list starting at |pos|.
   * You can obtain all nodes via "for (const Node *node = lattice->begin_nodes(pos); node; node = node->bnext) {}"
   * @param pos position of nodes. 0 <= pos < size()
   * @return node linked list
   */
  Node* begin_nodes(size_t pos) const { return begin_nodes_[pos]; }

  /**
   * Return sentence.
   * If MECAB_NBEST or MECAB_PARTIAL mode is off, the returned poiner is the same as the one set by set_sentence().
   * @return sentence
   */
  const char* sentence() const { return sentence_; }

  /**
   * Set sentence. This method does not take the ownership of the object.
   * @param sentence sentence
   */
  void set_sentence(const char* sentence) { return set_sentence(sentence, strlen(sentence)); }

  /**
   * Set sentence. This method does not take the ownership of the object.
   * @param sentence sentence
   * @param len length of the sentence
   */
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

  /**
   * Return sentence size.
   * @return sentence size
   */
  size_t size() const { return size_; }

  /**
   * Set normalization factor of CRF.
   * @param Z new normalization factor.
   */
  void set_Z(double Z) { Z_ = Z; }

  /**
   * return normalization factor of CRF.
   * @return normalization factor.
   */
  double Z() const { return Z_; }

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
   * Obtain next-best result. The internal linked list structure is updated.
   * You should set MECAB_NBEST reques_type in advance.
   * Return false if no more results are available or request_type is invalid.
   * @return boolean
   */
  bool next() {
    if (!has_request_type(MECAB_NBEST)) {
      set_what("MECAB_NBEST request type is not set");
      return false;
    }

    if (!allocator()->nbest_generator()->next()) {
      return false;
    }

    buildAllLattice(this);
    return true;
  }

  /**
   * Return the current request type.
   * @return request type
   */
  int request_type() const { return request_type_; }

  /**
   * Return true if the object has a specified request type.
   * @return boolean
   */
  bool has_request_type(int request_type) const { return request_type & request_type_; }

  /**
   * Set request type.
   * @param request_type new request type assigned
   */
  void set_request_type(int request_type) { request_type_ = request_type; }

  /**
   * Add request type.
   * @param request_type new request type added
   */
  void add_request_type(int request_type) { request_type_ |= request_type; }

  /**
   * Remove request type.
   * @param request_type new request type removed
   */
  void remove_request_type(int request_type) { request_type_ &= ~request_type; }

  /**
   * This method is used internally.
   */
  Allocator<Node, Path>* allocator() const { return allocator_.get(); }

  /**
   * Return new node. Lattice objects has the ownership of the node.
   * @return new node object
   */
  Node* newNode() { return allocator_->newNode(); }

  /**
   * Return string representation of the lattice.
   * Returned object is managed by this instance. When clear/set_sentence() method
   * is called, the returned buffer is initialized.
   * @return string representation of the lattice
   */
  const char* toString() { return toStringInternal(stream()); }

  /**
   * Return string representation of the node.
   * Returned object is managed by this instance. When clear/set_sentence() method
   * is called, the returned buffer is initialized.
   * @return string representation of the node
   * @param node node object
   */
  const char* toString(const Node* node) { return toStringInternal(node, stream()); }

  /**
   * Return string representation of the N-best results.
   * Returned object is managed by this instance. When clear/set_sentence() method
   * is called, the returned buffer is initialized.
   * @return string representation of the node
   * @param N how many results you want to obtain
   */
  const char* enumNBestAsString(size_t N) { return enumNBestAsStringInternal(N, stream()); }

  /**
   * Return string representation of the lattice.
   * Result is saved in the specified buffer.
   * @param buf output buffer
   * @param size output buffer size
   * @return string representation of the lattice
   */
  const char* toString(char* buf, size_t size) {
    StringBuffer os(buf, size);
    return toStringInternal(&os);
  }

  /**
   * Return string representation of the node.
   * Result is saved in the specified buffer.
   * @param node node object
   * @param buf output buffer
   * @param size output buffer size
   * @return string representation of the lattice
   */
  const char* toString(const Node* node, char* buf, size_t size) {
    StringBuffer os(buf, size);
    return toStringInternal(node, &os);
  }

  /**
   * Return string representation of the N-best result.
   * Result is saved in the specified.
   * @param N how many results you want to obtain
   * @param buf output buffer
   * @param size output buffer size
   * @return string representation of the lattice
   */
  const char* enumNBestAsString(size_t N, char* buf, size_t size) {
    StringBuffer os(buf, size);
    return enumNBestAsStringInternal(N, &os);
  }

  /**
   * Returns true if any parsing constraint is set
   */
  bool has_constraint() const { return !boundary_constraint_.empty(); }

  /**
   * Returns the boundary constraint at the position.
   * @param pos the position of constraint
   * @return boundary constraint type
   */
  int boundary_constraint(size_t pos) const {
    if (!boundary_constraint_.empty()) {
      return boundary_constraint_[pos];
    }
    return MECAB_ANY_BOUNDARY;
  }

  /**
   * Returns the token constraint at the position.
   * @param pos the beginning position of constraint.
   * @return constrained node starting at the position.
   */
  const char* feature_constraint(size_t begin_pos) const {
    if (!feature_constraint_.empty()) {
      return feature_constraint_[begin_pos];
    }
    return 0;
  }

  /**
   * Set parsing constraint for partial parsing mode.
   * @param pos the position of the boundary
   * @param boundary_constraint_type the type of boundary
   */
  void set_boundary_constraint(size_t pos, int boundary_constraint_type) {
    if (boundary_constraint_.empty()) {
      boundary_constraint_.resize(size() + 4, MECAB_ANY_BOUNDARY);
    }
    boundary_constraint_[pos] = boundary_constraint_type;
  }

  /**
   * Set parsing constraint for partial parsing mode.
   * @param begin_pos the starting position of the constrained token.
   * @param end_pos the the ending position of the constrained token.
   * @param feature the feature of the constrained token.
   */
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

  /**
   * Set golden parsing results for unittesting.
   * @param result the parsing result written in the standard mecab output.
   */
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

  /**
   * Return error string.
   * @return error string
   */
  const char* what() const { return what_.c_str(); }

  /**
   * Set error string. given string is copied to the internal buffer.
   * @param str new error string
   */
  void set_what(const char* str) { what_.assign(str); }

  /**
   * Create new Lattice object
   * @return new Lattice object
   */
  static Lattice* create() { return new Lattice; }

  static bool buildAllLattice(Lattice* lattice) {
    if (!lattice->has_request_type(MECAB_ALL_MORPHS)) {
      return true;
    }

    Node* prev = lattice->bos_node();
    const size_t len = lattice->size();
    Node** begin_node_list = lattice->begin_nodes();

    for (long pos = 0; pos <= static_cast<long>(len); ++pos) {
      for (Node* node = begin_node_list[pos]; node; node = node->bnext) {
        prev->next = node;
        node->prev = prev;
        prev = node;
      }
    }

    return true;
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

}  // namespace MeCab

#endif  // __MECAB_LATTICE_H__
