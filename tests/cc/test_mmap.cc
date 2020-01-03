#include "gtest/gtest.h"
#include "mecab/mmap.h"

TEST(mecab_mmap, test_unopend_mmap_instance) {
  MeCab::Mmap<char> mmap;

  ASSERT_EQ(mmap.size(), 0);
  ASSERT_EQ(mmap.file_size(), 0);
  ASSERT_TRUE(mmap.empty());
  ASSERT_STREQ(mmap.file_name().c_str(), "");
  ASSERT_EQ(mmap.begin(), nullptr);
}
