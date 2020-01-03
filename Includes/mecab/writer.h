#ifndef _MECAB_WRITER_H_
#define _MECAB_WRITER_H_

#include <string>

#include "mecab.h"
#include "mecab/common.h"
#include "mecab/scoped_ptr.h"
#include "mecab/string_buffer.h"
#include "mecab/utils.h"

namespace MeCab {

class Param;

class Writer {
 public:
  Writer();
  virtual ~Writer();
  bool open(const Param& param);
  void close();

  bool writeNode(Lattice* lattice, const char* format, const Node* node, StringBuffer* s) const;
  bool writeNode(Lattice* lattice, const Node* node, StringBuffer* s) const;

  bool write(Lattice* lattice, StringBuffer* node) const;

 private:
  scoped_string node_format_;
  scoped_string bos_format_;
  scoped_string eos_format_;
  scoped_string unk_format_;
  scoped_string eon_format_;

  bool writeLattice(Lattice* lattice, StringBuffer* s) const;
  bool writeWakati(Lattice* lattice, StringBuffer* s) const;
  bool writeNone(Lattice* lattice, StringBuffer* s) const;
  bool writeUser(Lattice* lattice, StringBuffer* s) const;
  bool writeDump(Lattice* lattice, StringBuffer* s) const;
  bool writeEM(Lattice* lattice, StringBuffer* s) const;

  bool (Writer::*write_)(Lattice* lattice, StringBuffer* s) const;
};
}  // namespace MeCab

#endif  // _MECAB_WRITER_H_
