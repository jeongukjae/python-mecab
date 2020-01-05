#include "gtest/gtest.h"
#include "mecab/new_param.h"

std::vector<MeCab::Option> options{{"test-option", 't', "", "some description of test-option", ""},
                                   {"arg-option", 'a', "ARG", "some description of arg-option", ""}};

#define MAKE_ARGS(arg_name, ...)                                   \
  std::vector<std::string> __arguments{__VA_ARGS__};               \
  std::vector<char*> arg_name;                                     \
  for (size_t i = 0; i < __arguments.size(); ++i) {                \
    arg_name.push_back(const_cast<char*>(__arguments[i].c_str())); \
  }

TEST(mecab_param, test_help_version_messages) {
  MeCab::Param param;
  MAKE_ARGS(arguments, "command", "-v");

  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_STREQ(param.getHelpMessage().c_str(),
               "MeCab: Yet Another Part-of-Speech and Morphological Analyzer\n\n"
               "Copyright(C) 2001-2012 Taku Kudo\n"
               "Copyright(C) 2004-2008 Nippon Telegraph and Telephone Corporation\n\n"
               "Usage: command [options] files\n"
               " -t, --test-option    some description of test-option\n"
               " -a, --arg-option=ARG some description of arg-option\n"
               " -h, --help           print help message and exit\n"
               " -v, --version        print version and exit\n");
  ASSERT_STREQ(param.getVersionMessage().c_str(), "mecab of 0.996\n");
  ASSERT_STREQ(param.getCommandName().c_str(), "command");
}

TEST(mecab_param, test_parse_help_argument) {
  MeCab::Param param;
  std::vector<MeCab::Option> emptyOption;

  MAKE_ARGS(arguments, "command", "-h");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), emptyOption));
  ASSERT_TRUE(param.get<bool>("help"));
  ASSERT_FALSE(param.get<bool>("some-unknown"));
}

TEST(mecab_param, test_parse_long_help_argument) {
  MeCab::Param param;
  std::vector<MeCab::Option> emptyOption;

  MAKE_ARGS(arguments, "command", "--help");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), emptyOption));
  ASSERT_TRUE(param.get<bool>("help"));
  ASSERT_FALSE(param.get<bool>("some-unknown"));
}

TEST(mecab_param, test_parse_version_and_help_argument) {
  MeCab::Param param;
  std::vector<MeCab::Option> emptyOption;

  MAKE_ARGS(arguments, "command", "-v", "-h");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), emptyOption));
  ASSERT_TRUE(param.get<bool>("version"));
  ASSERT_TRUE(param.get<bool>("help"));
  ASSERT_FALSE(param.get<bool>("some-unknown"));
}
