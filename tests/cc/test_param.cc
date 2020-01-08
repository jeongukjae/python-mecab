#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mecab/param.h"

std::vector<MeCab::Option> options{{"test-option", 't', "", "", "some description of test-option"},
                                   {"arg-option", 'a', "", "ARG", "some description of arg-option"}};

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
  ASSERT_EQ(param.getHelpMessage(),
            "MeCab: Yet Another Part-of-Speech and Morphological Analyzer\n\n"
            "Copyright(C) 2001-2012 Taku Kudo\n"
            "Copyright(C) 2004-2008 Nippon Telegraph and Telephone Corporation\n\n"
            "Usage: command [options] files\n"
            " -t, --test-option    some description of test-option\n"
            " -a, --arg-option=ARG some description of arg-option\n"
            " -h, --help           print help message and exit\n"
            " -v, --version        print version and exit\n");
  ASSERT_EQ(param.getVersionMessage(), "mecab of 0.996\n");
  ASSERT_EQ(param.getCommandName(), "command");
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

TEST(mecab_param, test_parse_argument_1) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "-a=hello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}

TEST(mecab_param, test_parse_argument_2) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "-a=");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "");
}

TEST(mecab_param, test_parse_argument_3) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "-ahello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}

TEST(mecab_param, test_parse_argument_4) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "-a", "hello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}
TEST(mecab_param, test_parse_argument_5) {
  MeCab::Param param;

  testing::internal::CaptureStderr();
  MAKE_ARGS(arguments, "command", "-a");
  ASSERT_FALSE(param.parse(arguments.size(), arguments.data(), options));
  EXPECT_THAT(testing::internal::GetCapturedStderr(), ::testing::HasSubstr("`arg-option` requires an argument"));
}

TEST(mecab_param, test_parse_long_argument_1) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "--arg-option", "hello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}

TEST(mecab_param, test_parse_long_argument_2) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "--arg-option=hello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}

TEST(mecab_param, test_parse_long_argument_3) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "--arg-option=");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "");
}

TEST(mecab_param, test_parse_long_argument_4) {
  MeCab::Param param;

  testing::internal::CaptureStderr();
  MAKE_ARGS(arguments, "command", "--arg-option");
  ASSERT_FALSE(param.parse(arguments.size(), arguments.data(), options));
  EXPECT_THAT(testing::internal::GetCapturedStderr(), ::testing::HasSubstr("`arg-option` requires an argument"));
}

TEST(mecab_param, test_parse_long_argument_5) {
  MeCab::Param param;

  testing::internal::CaptureStderr();
  MAKE_ARGS(arguments, "command", "--arg-optiontest");
  ASSERT_FALSE(param.parse(arguments.size(), arguments.data(), options));
  EXPECT_THAT(testing::internal::GetCapturedStderr(), ::testing::HasSubstr("unrecognized option `--arg-optiontest`"));
}

TEST(mecab_param, test_parse_with_string) {
  MeCab::Param param;

  testing::internal::CaptureStderr();
  ASSERT_FALSE(param.parse("--arg-optiontest", options));
  EXPECT_THAT(testing::internal::GetCapturedStderr(), ::testing::HasSubstr("unrecognized option `--arg-optiontest`"));
}

TEST(mecab_param, test_parse_with_string_2) {
  MeCab::Param param;

  ASSERT_TRUE(param.parse("-a hello", options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}

TEST(mecab_param, test_parse_with_string_3) {
  MeCab::Param param;

  ASSERT_TRUE(param.parse("--arg-option hello", options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
}

TEST(mecab_param, test_multiple_arg_1) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "--arg-option=blabla", "-t");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_TRUE(param.get<bool>("test-option"));
  ASSERT_EQ(param.get<std::string>("arg-option"), "blabla");
}

TEST(mecab_param, test_clear) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "--arg-option=hello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");

  param.clear();
  ASSERT_EQ(param.get<std::string>("arg-option"), "");
}

TEST(mecab_param, test_get_unknown) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "--arg-option=hello");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "hello");
  ASSERT_EQ(param.get<bool>("unknown"), false);
  ASSERT_EQ(param.get<short>("unknown"), 0);
  ASSERT_EQ(param.get<int>("unknown"), 0);
  ASSERT_EQ(param.get<float>("unknown"), 0.0);
  ASSERT_EQ(param.get<void*>("unknown"), nullptr);
}

TEST(mecab_param, test_dump_config) {
  MeCab::Param param;

  ASSERT_TRUE(param.parse("--arg-option hello -t", options));
  testing::internal::CaptureStdout();
  param.dumpConfig();
  auto captured = testing::internal::GetCapturedStdout();

  ASSERT_THAT(captured, testing::HasSubstr("arg-option: hello\n"));
  ASSERT_THAT(captured, testing::HasSubstr("test-option: 1\n"));
}

/* file content:
; some-comment = 123
# some-comment = 123
test-option = blabla
test-option2 = blablablabla
*/
TEST(mecab_param, test_parse_file) {
  MeCab::Param param;

  ASSERT_TRUE(param.parseFile("../tests/test-data/cc/file-used-in-test-param.txt"));
  ASSERT_EQ(param.get<std::string>("test-option"), "blabla");
  ASSERT_EQ(param.get<std::string>("test-option2"), "blablablabla");
  ASSERT_EQ(param.get<std::string>("unknown-option"), "");
  ASSERT_EQ(param.get<std::string>("some-comment"), "");
}

TEST(mecab_param, test_parse_file_after_parsing_parameter) {
  MeCab::Param param;
  MAKE_ARGS(arguments, "command", "-t");

  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_TRUE(param.parseFile("../tests/test-data/cc/file-used-in-test-param.txt"));
  ASSERT_EQ(param.get<std::string>("test-option"), "1");
  ASSERT_EQ(param.get<std::string>("test-option2"), "blablablabla");
  ASSERT_EQ(param.get<std::string>("unknown-option"), "");
}

TEST(mecab_param, test_get_rest_parameters) {
  MeCab::Param param;
  MAKE_ARGS(arguments, "command", "-t", "1", "2", "3", "4");

  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_THAT(param.getRestParameters(), testing::ElementsAre("1", "2", "3", "4"));
}

// =========
// regressions

TEST(mecab_param, test_value_containing_whitespcae) {
  MeCab::Param param;

  MAKE_ARGS(arguments, "command", "-a", "0 1 2 4");
  ASSERT_TRUE(param.parse(arguments.size(), arguments.data(), options));
  ASSERT_EQ(param.get<std::string>("arg-option"), "0 1 2 4");
}

TEST(mecab_param, test_parse_autolink_dicrc) {
  MeCab::Param param;

  param.set("output-format-type", "");
  ASSERT_TRUE(param.parseFile("../tests/test-data/autolink/dicrc"));
  ASSERT_EQ(param.get<std::string>("output-format-type"), "autolink");
  ASSERT_EQ(param.get<std::string>("node-format-autolink"), "<a href=\"%H\">%M</a>");
}
