#include "gtest/gtest.h"
#include "utils.h"

TEST(mecab_utils, test_decode_charset) {
  ASSERT_EQ(MeCab::decode_charset("euc"), MeCab::EUC_JP);
  ASSERT_EQ(MeCab::decode_charset("utf8"), MeCab::UTF8);
  ASSERT_EQ(MeCab::decode_charset("invalid"), MeCab::UTF8);
}

TEST(mecab_utils, test_create_filename) {
  ASSERT_STREQ(MeCab::create_filename("path", "file").c_str(), "path/file");
}

TEST(mecab_utils, test_remove_filename) {
  ASSERT_STREQ(MeCab::remove_filename("/some-path/to/file").c_str(), "/some-path/to");
  ASSERT_STREQ(MeCab::remove_filename("some-relative/path/to/file").c_str(), "some-relative/path/to");
  ASSERT_STREQ(MeCab::remove_filename("not-a-path").c_str(), ".");
}

TEST(mecab_utils, test_remove_pathname) {
  ASSERT_STREQ(MeCab::remove_pathname("/some-path/to/file").c_str(), "file");
  ASSERT_STREQ(MeCab::remove_pathname("some/relative/path").c_str(), "path");
  ASSERT_STREQ(MeCab::remove_pathname("not-a-path").c_str(), ".");
}

TEST(mecab_utils, test_replace_string) {
  ASSERT_STREQ(MeCab::replace_string("example-string", "string", "test").c_str(), "example-test");
  ASSERT_STREQ(MeCab::replace_string("test this string", "this", "that").c_str(), "test that string");
}

TEST(mecab_utils, test_to_lower) {
  ASSERT_STREQ(MeCab::to_lower("TesT ThIS StRINg").c_str(), "test this string");
}
