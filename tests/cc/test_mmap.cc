#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mecab/mmap.h"

TEST(mecab_mmap, test_unopend_mmap_instance) {
  MeCab::Mmap<char> mmap;

  ASSERT_EQ(mmap.size(), 0);
  ASSERT_EQ(mmap.file_size(), 0);
  ASSERT_TRUE(mmap.empty());
  ASSERT_EQ(mmap.file_name(), "");
  ASSERT_EQ(mmap.begin(), nullptr);
}

TEST(mecab_mmap, test_open_mmap_instance) {
  MeCab::Mmap<char> mmap;

  // this test case will be ran in ./build directory
  ASSERT_TRUE(mmap.open("../tests/test-data/cc/file-used-in-test-mmap.txt"));
  // file content: "test content" + new line
  ASSERT_STREQ(mmap.begin(), "test content\n");
  ASSERT_EQ(mmap[3], 't');
  ASSERT_EQ(mmap.file_name(), "../tests/test-data/cc/file-used-in-test-mmap.txt");
  ASSERT_EQ(mmap.size(), 13);
  ASSERT_EQ(mmap.file_size(), 13);
  ASSERT_FALSE(mmap.empty());
}

TEST(mecab_mmap, test_open_mmap_instance_with_invalid_mode) {
  MeCab::Mmap<char> mmap;

  // this test case will be ran in ./build directory
  testing::internal::CaptureStderr();
  ASSERT_FALSE(mmap.open("../tests/test-data/cc/file-used-in-test-mmap.txt", "invalid"));
  std::string captured = testing::internal::GetCapturedStderr();
  EXPECT_THAT(captured, ::testing::HasSubstr(
                            "unknown open mode: ../tests/test-data/cc/file-used-in-test-mmap.txt mode: invalid"));
}
