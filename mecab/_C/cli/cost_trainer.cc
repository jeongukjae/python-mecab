#include "mecab/cost_trainer.h"

int mecab_cost_train(int argc, char** argv) {
  return MeCab::Learner::run(argc, argv);
}
