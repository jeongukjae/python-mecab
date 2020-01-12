#ifndef _MECAB_NBEST_GENERATOR_H_
#define _MECAB_NBEST_GENERATOR_H_

#include <queue>

#include "mecab.h"
#include "mecab/freelist.h"

namespace MeCab {

namespace {
struct QueueElement {
  Node* node;
  QueueElement* next;
  long fx;  // f(x) = h(x) + g(x): cost function for A* search
  long gx;  // g(x)
};

class QueueElementComp {
 public:
  const bool operator()(QueueElement* q1, QueueElement* q2) { return (q1->fx > q2->fx); }
};
}  // namespace

class NBestGenerator {
 private:
  std::priority_queue<QueueElement*, std::vector<QueueElement*>, QueueElementComp> agenda_;
  FreeList<QueueElement> freelist_;

 public:
  explicit NBestGenerator() : freelist_(512) {}
  virtual ~NBestGenerator() {}
  bool set(Lattice* lattice) {
    freelist_.free();
    while (!agenda_.empty()) {
      agenda_.pop();  // make empty
    }
    QueueElement* eos = freelist_.alloc();
    eos->node = lattice->eos_node();
    eos->next = 0;
    eos->fx = eos->gx = 0;
    agenda_.push(eos);
    return true;
  }
  bool next() {
    while (!agenda_.empty()) {
      QueueElement* top = agenda_.top();
      agenda_.pop();
      Node* rnode = top->node;

      if (rnode->stat == MECAB_BOS_NODE) {  // BOS
        for (QueueElement* n = top; n->next; n = n->next) {
          n->node->next = n->next->node;  // change next & prev
          n->next->node->prev = n->node;
          // TODO: rewrite costs;
        }
        return true;
      }

      for (Path* path = rnode->lpath; path; path = path->lnext) {
        QueueElement* n = freelist_.alloc();
        n->node = path->lnode;
        n->gx = path->cost + top->gx;
        n->fx = path->lnode->cost + path->cost + top->gx;
        n->next = top;
        agenda_.push(n);
      }
    }

    return false;
  }
};
}  // namespace MeCab

#endif  // _MECAB_NBEST_GENERATOR_H_
