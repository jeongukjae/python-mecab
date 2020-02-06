/*
  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer

  Copyright(C) 2001-2011 Taku Kudo <taku@chasen.org>
  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
*/
#ifndef _MECAB_MECAB_H_
#define _MECAB_MECAB_H_

#include <iostream>

#include "mecab/data_structure.h"

namespace MeCab {

template <typename N, typename P>
class Allocator;
class Tagger;
class Param;

/**
 * Lattice class
 */
class Lattice {
 public:
  /**
   * Clear all internal lattice data.
   */
  virtual void clear() = 0;

  /**
   * Return true if result object is available.
   * @return boolean
   */
  virtual bool is_available() const = 0;

  /**
   * Return bos (begin of sentence) node.
   * You can obtain all nodes via "for (const Node *node = lattice->bos_node(); node; node = node->next) {}"
   * @return bos node object
   */
  virtual Node* bos_node() const = 0;

  /**
   * Return eos (end of sentence) node.
   * @return eos node object
   */
  virtual Node* eos_node() const = 0;

  /**
   * This method is used internally.
   */
  virtual Node** begin_nodes() const = 0;

  /**
   * This method is used internally.
   */
  virtual Node** end_nodes() const = 0;

  /**
   * Return node linked list ending at |pos|.
   * You can obtain all nodes via "for (const Node *node = lattice->end_nodes(pos); node; node = node->enext) {}"
   * @param pos position of nodes. 0 <= pos < size()
   * @return node linked list
   */
  virtual Node* end_nodes(size_t pos) const = 0;

  /**
   * Return node linked list starting at |pos|.
   * You can obtain all nodes via "for (const Node *node = lattice->begin_nodes(pos); node; node = node->bnext) {}"
   * @param pos position of nodes. 0 <= pos < size()
   * @return node linked list
   */
  virtual Node* begin_nodes(size_t pos) const = 0;

  /**
   * Return sentence.
   * If MECAB_NBEST or MECAB_PARTIAL mode is off, the returned poiner is the same as the one set by set_sentence().
   * @return sentence
   */
  virtual const char* sentence() const = 0;

  /**
   * Set sentence. This method does not take the ownership of the object.
   * @param sentence sentence
   */
  virtual void set_sentence(const char* sentence) = 0;

  /**
   * Set sentence. This method does not take the ownership of the object.
   * @param sentence sentence
   * @param len length of the sentence
   */
  virtual void set_sentence(const char* sentence, size_t len) = 0;

  /**
   * Return sentence size.
   * @return sentence size
   */
  virtual size_t size() const = 0;

  /**
   * Set normalization factor of CRF.
   * @param Z new normalization factor.
   */
  virtual void set_Z(double Z) = 0;

  /**
   * return normalization factor of CRF.
   * @return normalization factor.
   */
  virtual double Z() const = 0;

  /**
   * Set temparature parameter theta.
   * @param theta temparature parameter.
   */
  virtual void set_theta(float theta) = 0;

  /**
   * Return temparature parameter theta.
   * @return temparature parameter.
   */
  virtual float theta() const = 0;

  /**
   * Obtain next-best result. The internal linked list structure is updated.
   * You should set MECAB_NBEST reques_type in advance.
   * Return false if no more results are available or request_type is invalid.
   * @return boolean
   */
  virtual bool next() = 0;

  /**
   * Return the current request type.
   * @return request type
   */
  virtual int request_type() const = 0;

  /**
   * Return true if the object has a specified request type.
   * @return boolean
   */
  virtual bool has_request_type(int request_type) const = 0;

  /**
   * Set request type.
   * @param request_type new request type assigned
   */
  virtual void set_request_type(int request_type) = 0;

  /**
   * Add request type.
   * @param request_type new request type added
   */
  virtual void add_request_type(int request_type) = 0;

  /**
   * Remove request type.
   * @param request_type new request type removed
   */
  virtual void remove_request_type(int request_type) = 0;

  /**
   * This method is used internally.
   */
  virtual Allocator<Node, Path>* allocator() const = 0;

  /**
   * Return new node. Lattice objects has the ownership of the node.
   * @return new node object
   */
  virtual Node* newNode() = 0;

  /**
   * Return string representation of the lattice.
   * Returned object is managed by this instance. When clear/set_sentence() method
   * is called, the returned buffer is initialized.
   * @return string representation of the lattice
   */
  virtual const char* toString() = 0;

  /**
   * Return string representation of the node.
   * Returned object is managed by this instance. When clear/set_sentence() method
   * is called, the returned buffer is initialized.
   * @return string representation of the node
   * @param node node object
   */
  virtual const char* toString(const Node* node) = 0;

  /**
   * Return string representation of the N-best results.
   * Returned object is managed by this instance. When clear/set_sentence() method
   * is called, the returned buffer is initialized.
   * @return string representation of the node
   * @param N how many results you want to obtain
   */
  virtual const char* enumNBestAsString(size_t N) = 0;

  /**
   * Return string representation of the lattice.
   * Result is saved in the specified buffer.
   * @param buf output buffer
   * @param size output buffer size
   * @return string representation of the lattice
   */
  virtual const char* toString(char* buf, size_t size) = 0;

  /**
   * Return string representation of the node.
   * Result is saved in the specified buffer.
   * @param node node object
   * @param buf output buffer
   * @param size output buffer size
   * @return string representation of the lattice
   */
  virtual const char* toString(const Node* node, char* buf, size_t size) = 0;

  /**
   * Return string representation of the N-best result.
   * Result is saved in the specified.
   * @param N how many results you want to obtain
   * @param buf output buffer
   * @param size output buffer size
   * @return string representation of the lattice
   */
  virtual const char* enumNBestAsString(size_t N, char* buf, size_t size) = 0;

  /**
   * Returns true if any parsing constraint is set
   */
  virtual bool has_constraint() const = 0;

  /**
   * Returns the boundary constraint at the position.
   * @param pos the position of constraint
   * @return boundary constraint type
   */
  virtual int boundary_constraint(size_t pos) const = 0;

  /**
   * Returns the token constraint at the position.
   * @param pos the beginning position of constraint.
   * @return constrained node starting at the position.
   */
  virtual const char* feature_constraint(size_t pos) const = 0;

  /**
   * Set parsing constraint for partial parsing mode.
   * @param pos the position of the boundary
   * @param boundary_constraint_type the type of boundary
   */
  virtual void set_boundary_constraint(size_t pos, int boundary_constraint_type) = 0;

  /**
   * Set parsing constraint for partial parsing mode.
   * @param begin_pos the starting position of the constrained token.
   * @param end_pos the the ending position of the constrained token.
   * @param feature the feature of the constrained token.
   */
  virtual void set_feature_constraint(size_t begin_pos, size_t end_pos, const char* feature) = 0;

  /**
   * Set golden parsing results for unittesting.
   * @param result the parsing result written in the standard mecab output.
   */
  virtual void set_result(const char* result) = 0;

  /**
   * Return error string.
   * @return error string
   */
  virtual const char* what() const = 0;

  /**
   * Set error string. given string is copied to the internal buffer.
   * @param str new error string
   */
  virtual void set_what(const char* str) = 0;

  /**
   * Create new Lattice object
   * @return new Lattice object
   */
  static Lattice* create();

  virtual ~Lattice() {}
};

/**
 * Model class
 */
class Model {
 public:
  /**
   * Return DictionaryInfo linked list.
   * @return DictionaryInfo linked list
   */
  virtual const DictionaryInfo* dictionary_info() const = 0;

  /**
   * Return transtion cost from rcAttr to lcAttr.
   * @return transtion cost
   */
  virtual int transition_cost(unsigned short rcAttr, unsigned short lcAttr) const = 0;

  /**
   * perform common prefix search from the range [begin, end).
   * |lattice| takes the ownership of return value.
   * @return node linked list.
   */
  virtual Node* lookup(const char* begin, const char* end, Lattice* lattice) const = 0;

  /**
   * Create a new Tagger object.
   * All returned tagger object shares this model object as a parsing model.
   * Never delete this model object before deleting tagger object.
   * @return new Tagger object
   */
  virtual Tagger* createTagger() const = 0;

  /**
   * Create a new Lattice object.
   * @return new Lattice object
   */
  virtual Lattice* createLattice() const = 0;

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
  virtual bool swap(Model* model) = 0;

  /**
   * Return a version string
   * @return version string
   */
  static const char* version();

  virtual ~Model() {}

  /**
   * Factory method to create a new Model with a specified main's argc/argv-style parameters.
   * Return NULL if new model cannot be initialized. Use MeCab::getLastError() to obtain the
   * cause of the errors.
   * @return new Model object
   * @param argc number of parameters
   * @param argv parameter list
   */
  static Model* create(int argc, char** argv);

  /**
   * Factory method to create a new Model with a string parameter representation, i.e.,
   * "-d /user/local/mecab/dic/ipadic -Ochasen".
   * Return NULL if new model cannot be initialized. Use MeCab::getLastError() to obtain the
   * cause of the errors.
   * @return new Model object
   * @param arg single string representation of the argment.
   */
  static Model* create(const char* arg);

  static Model* create(const Param& param);
};

/**
 * Tagger class
 */
class Tagger {
 public:
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
  static bool parse(const Model& model, Lattice* lattice);

  /**
   * Parse lattice object.
   * Return true if lattice is parsed successfully.
   * A sentence must be set to the lattice with Lattice:set_sentence object before calling this method.
   * Parsed node object can be obtained with Lattice:bos_node.
   * This method is thread safe.
   * @return lattice lattice object
   * @return boolean
   */
  virtual bool parse(Lattice* lattice) const = 0;

  /**
   * Parse given sentence and return parsed result as string.
   * You should not delete the returned string. The returned buffer
   * is overwritten when parse method is called again.
   * This method is NOT thread safe.
   * @param str sentence
   * @return parsed result
   */
  virtual const char* parse(const char* str) = 0;

  /**
   * Parse given sentence and return Node object.
   * You should not delete the returned node object. The returned buffer
   * is overwritten when parse method is called again.
   * You can traverse all nodes via Node::next member.
   * This method is NOT thread safe.
   * @param str sentence
   * @return bos node object
   */
  virtual const Node* parseToNode(const char* str) = 0;

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
  virtual const char* parseNBest(size_t N, const char* str) = 0;

  /**
   * Initialize N-best enumeration with a sentence.
   * Return true if initialization finishes successfully.
   * N-best result is obtained by calling next() or nextNode() in sequence.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @param str sentence
   * @return boolean
   */
  virtual bool parseNBestInit(const char* str) = 0;

  /**
   * Return next-best parsed result. You must call parseNBestInit() in advance.
   * Return NULL if no more reuslt is available.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @return node object
   */
  virtual const Node* nextNode() = 0;

  /**
   * Return next-best parsed result. You must call parseNBestInit() in advance.
   * Return NULL if no more reuslt is available.
   * This method is NOT thread safe.
   * This method is DEPRECATED. Use Lattice class.
   * @return parsed result
   */
  virtual const char* next() = 0;

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
  virtual const char* formatNode(const Node* node) = 0;

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
  virtual const char* parse(const char* str, size_t len, char* ostr, size_t olen) = 0;

  /**
   * The same as parse() method, but input length can be passed.
   * @param str sentence
   * @param len sentence length
   * @return parsed result
   */
  virtual const char* parse(const char* str, size_t len) = 0;

  /**
   * The same as parseToNode(), but input lenth can be passed.
   * @param str sentence
   * @param len sentence length
   * @return node object
   */
  virtual const Node* parseToNode(const char* str, size_t len) = 0;

  /**
   * The same as parseNBest(), but input length can be passed.
   * @param N how many results you want to obtain
   * @param str sentence
   * @param len sentence length
   * @return parsed result
   */
  virtual const char* parseNBest(size_t N, const char* str, size_t len) = 0;

  /**
   * The same as parseNBestInit(), but input length can be passed.
   * @param str sentence
   * @param len sentence length
   * @return boolean
   * @return parsed result
   */
  virtual bool parseNBestInit(const char* str, size_t len) = 0;

  /**
   * The same as next(), but output buffer can be passed.
   * Return NULL if more than |olen| buffer is required to store output string.
   * @param ostr output buffer
   * @param olen output buffer length
   * @return parsed result
   */
  virtual const char* next(char* ostr, size_t olen) = 0;

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
  virtual const char* parseNBest(size_t N, const char* str, size_t len, char* ostr, size_t olen) = 0;

  /**
   * The same as formatNode(), but output buffer can be passed.
   * Return NULL if more than |olen| buffer is required to store output string.
   * @param node node object
   * @param ostr output buffer
   * @param olen output buffer length
   * @return parsed result
   */
  virtual const char* formatNode(const Node* node, char* ostr, size_t olen) = 0;

  /**
   * Set request type.
   * This method is DEPRECATED. Use Lattice::set_request_type(MECAB_PARTIAL).
   * @param request_type new request type assigned
   */
  virtual void set_request_type(int request_type) = 0;

  /**
   * Return the current request type.
   * This method is DEPRECATED. Use Lattice class.
   * @return request type
   */
  virtual int request_type() const = 0;

  /**
   * Return true if partial parsing mode is on.
   * This method is DEPRECATED. Use Lattice::has_request_type(MECAB_PARTIAL).
   * @return boolean
   */
  virtual bool partial() const = 0;

  /**
   * set partial parsing mode.
   * This method is DEPRECATED. Use Lattice::add_request_type(MECAB_PARTIAL) or
   * Lattice::remove_request_type(MECAB_PARTIAL)
   * @param partial partial mode
   */
  virtual void set_partial(bool partial) = 0;

  /**
   * Return lattice level.
   * This method is DEPRECATED. Use Lattice::*_request_type()
   * @return int lattice level
   */
  virtual int lattice_level() const = 0;

  /**
   * Set lattice level.
   * This method is DEPRECATED. Use Lattice::*_request_type()
   * @param level lattice level
   */
  virtual void set_lattice_level(int level) = 0;

  /**
   * Return true if all morphs output mode is on.
   * This method is DEPRECATED. Use Lattice::has_request_type(MECAB_ALL_MORPHS).
   * @return boolean
   */
  virtual bool all_morphs() const = 0;

  /**
   * set all-morphs output mode.
   * This method is DEPRECATED. Use Lattice::add_request_type(MECAB_ALL_MORPHS) or
   * Lattice::remove_request_type(MECAB_ALL_MORPHS)
   * @param all_morphs
   */
  virtual void set_all_morphs(bool all_morphs) = 0;

  /**
   * Set temparature parameter theta.
   * @param theta temparature parameter.
   */
  virtual void set_theta(float theta) = 0;

  /**
   * Return temparature parameter theta.
   * @return temparature parameter.
   */
  virtual float theta() const = 0;

  /**
   * Return DictionaryInfo linked list.
   * @return DictionaryInfo linked list
   */
  virtual const DictionaryInfo* dictionary_info() const = 0;

  /**
   * Return error string.
   * @return error string
   */
  virtual const char* what() const = 0;

  virtual ~Tagger() {}

  /**
   * Factory method to create a new Tagger with a specified main's argc/argv-style parameters.
   * Return NULL if new model cannot be initialized. Use MeCab::getLastError() to obtain the
   * cause of the errors.
   * @return new Tagger object
   * @param argc number of parameters
   * @param argv parameter list
   */
  static Tagger* create(int argc, char** argv);

  /**
   * Factory method to create a new Tagger with a string parameter representation, i.e.,
   * "-d /user/local/mecab/dic/ipadic -Ochasen".
   * Return NULL if new model cannot be initialized. Use MeCab::getLastError() to obtain the
   * cause of the errors.
   * @return new Model object
   * @param arg single string representation of the argment.
   */
  static Tagger* create(const char* arg);

  /**
   * Return a version string
   * @return version string
   */
  static const char* version();
};

/**
 * Alias of Lattice::create()
 */
extern Lattice* createLattice();

/**
 * Alias of Mode::create(argc, argv)
 */
extern Model* createModel(int argc, char** argv);

/**
 * Alias of Mode::create(arg)
 */
extern Model* createModel(const char* arg);

/**
 * Alias of Tagger::create(argc, argv)
 */
extern Tagger* createTagger(int argc, char** argv);

/**
 * Alias of Tagger::create(arg)
 */
extern Tagger* createTagger(const char* arg);

/**
 * delete Lattice object.
 * This method calles "delete lattice".
 * In some environment, e.g., MS-Windows, an object allocated inside a DLL must be deleted in the same DLL too.
 * @param lattice lattice object
 */
inline void deleteLattice(Lattice* lattice) {
  delete lattice;
}

/**
 * delete Model object.
 * This method calles "delete model".
 * In some environment, e.g., MS-Windows, an object allocated inside a DLL must be deleted in the same DLL too.
 * @param model model object
 */
inline void deleteModel(Model* model) {
  delete model;
}

/**
 * delete Tagger object.
 * This method calles "delete tagger".
 * In some environment, e.g., MS-Windows, an object allocated inside a DLL must be deleted in the same DLL too.
 * @param tagger tagger object
 */
inline void deleteTagger(Tagger* tagger) {
  delete tagger;
}

/**
 * Return last error string.
 * @return error string
 */
extern const char* getLastError();
}  // namespace MeCab

#endif /* _MECAB_MECAB_H_ */
