/*
  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer

  Copyright(C) 2001-2011 Taku Kudo <taku@chasen.org>
  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
*/
#ifndef _MECAB_MECAB_H_
#define _MECAB_MECAB_H_

#include <iostream>

#include "mecab/data_structure.h"
#include "mecab/lattice.h"
#include "mecab/model.h"

namespace MeCab {

template <typename N, typename P>
class Allocator;
class Param;

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

extern Tagger* createTagger(const Model* model);

/**
 * Return last error string.
 * @return error string
 */
extern const char* getLastError();
}  // namespace MeCab

#endif /* _MECAB_MECAB_H_ */
