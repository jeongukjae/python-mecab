#ifndef _MECAB_UTILS_H_
#define _MECAB_UTILS_H_

#include <stdint.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "mecab/common.h"
#include "mecab/data_structure.h"
#include "mecab/utils/fingerprint.h"
#include "mecab/utils/io.h"
#include "mecab/utils/param.h"
#include "mecab/utils/string_utils.h"

namespace MeCab {

inline const char* read_ptr(const char** ptr, size_t size) {
  const char* r = *ptr;
  *ptr += size;
  return r;
}

template <class T>
inline void read_static(const char** ptr, T& value) {
  const char* r = read_ptr(ptr, sizeof(T));
  memcpy(&value, r, sizeof(T));
}

inline int load_request_type(const Param& param) {
  int request_type = MECAB_ONE_BEST;

  if (param.get<bool>("allocate-sentence")) {
    request_type |= MECAB_ALLOCATE_SENTENCE;
  }

  if (param.get<bool>("partial")) {
    request_type |= MECAB_PARTIAL;
  }

  if (param.get<bool>("all-morphs")) {
    request_type |= MECAB_ALL_MORPHS;
  }

  if (param.get<bool>("marginal")) {
    request_type |= MECAB_MARGINAL_PROB;
  }

  const int nbest = param.get<int>("nbest");
  if (nbest >= 2) {
    request_type |= MECAB_NBEST;
  }

  // DEPRECATED:
  const int lattice_level = param.get<int>("lattice-level");
  if (lattice_level >= 1) {
    request_type |= MECAB_NBEST;
  }

  if (lattice_level >= 2) {
    request_type |= MECAB_MARGINAL_PROB;
  }

  return request_type;
}

inline double logsumexp(double x, double y, bool flg) {
#define MINUS_LOG_EPSILON 50

  if (flg)
    return y;  // init mode
  double vmin = std::min<double>(x, y);
  double vmax = std::max<double>(x, y);
  if (vmax > vmin + MINUS_LOG_EPSILON) {
    return vmax;
  } else {
    return vmax + std::log(std::exp(vmin - vmax) + 1.0);
  }
}

inline short int tocost(double d, int n) {
  static const short max = +32767;
  static const short min = -32767;
  return static_cast<short>(
      std::max<double>(std::min<double>(-n * d, static_cast<double>(max)), static_cast<double>(min)));
}
}  // namespace MeCab

#endif  // _MECAB_UTILS_H_
