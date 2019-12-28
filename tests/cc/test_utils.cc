#include "gtest/gtest.h"
#include "utils.h"

TEST(mecab_utils, test_create_filename) {
  ASSERT_STREQ(MeCab::create_filename("path", "file").c_str(), "path/file");
}
