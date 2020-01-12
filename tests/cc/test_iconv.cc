#include <fstream>
#include "gtest/gtest.h"
#include "mecab/iconv.h"

TEST(mecab_iconv, test_convert_utf16_to_utf8) {
  std::ifstream utf16File("../tests/test-data/cc/file-used-in-test-iconv-utf16.txt");
  ASSERT_FALSE(utf16File.fail());

  std::string utf16FileContent((std::istreambuf_iterator<char>(utf16File)), std::istreambuf_iterator<char>());

  MeCab::Iconv iconv;
  iconv.open("UTF-16", "UTF-8");
  iconv.convert(&utf16FileContent);
  ASSERT_EQ(utf16FileContent, "hello world\n");
}
