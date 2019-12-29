#ifndef MECAB_VITERBI_H_
#define MECAB_VITERBI_H_

#include <vector>

#include "mecab.h"
#include "mecab/common.h"
#include "mecab/scoped_ptr.h"
#include "mecab/thread.h"

namespace MeCab {

class Lattice;
class Param;
class Connector;
template <typename N, typename P>
class Tokenizer;

class Viterbi {
 public:
  bool open(const Param& param);

  bool analyze(Lattice* lattice) const;

  const Tokenizer<Node, Path>* tokenizer() const;

  const Connector* connector() const;

  const char* what() { return what_.str(); }

  static bool buildResultForNBest(Lattice* lattice);

  Viterbi();
  virtual ~Viterbi();

 private:
  template <bool IsAllPath, bool IsPartial>
  bool viterbi(Lattice* lattice) const;

  static bool forwardbackward(Lattice* lattice);
  static bool initPartial(Lattice* lattice);
  static bool initNBest(Lattice* lattice);
  static bool buildBestLattice(Lattice* lattice);
  static bool buildAllLattice(Lattice* lattice);
  static bool buildAlternative(Lattice* lattice);

  scoped_ptr<Tokenizer<Node, Path>> tokenizer_;
  scoped_ptr<Connector> connector_;
  int cost_factor_;
  whatlog what_;
};
}  // namespace MeCab
#endif  // MECAB_VITERBI_H_
