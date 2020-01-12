#include "gtest/gtest.h"
#include "mecab/iconv.h"

TEST(mecab_iconv, test_convert_utf8_to_utf16_and_vice_versa) {
  MeCab::Iconv iconv;
  MeCab::Iconv iconv2;
  iconv.open("UTF-8", "UTF-16");
  iconv2.open("UTF-16", "UTF-8");
  std::string str = "hello world";
  iconv.convert(&str);
  iconv2.convert(&str);

  ASSERT_EQ(str, "hello world");
}
