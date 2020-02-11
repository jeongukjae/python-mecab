#include <algorithm>
#include <fstream>

#include "fixture/tmpdir.h"
#include "gtest/gtest.h"
#include "mecab/cli.h"

#define MAKE_ARGS(arg_name, ...)                                   \
  std::vector<std::string> __arguments{__VA_ARGS__};               \
  std::vector<char*> arg_name;                                     \
  for (size_t i = 0; i < __arguments.size(); ++i) {                \
    arg_name.push_back(const_cast<char*>(__arguments[i].c_str())); \
  }

namespace {
bool is_exists(std::string path) {
  struct stat info;
  return stat(path.c_str(), &info) == 0;
}

void copy_file(std::string from, std::string to) {
  std::ifstream src(from, std::ios::binary);
  std::ofstream dst(to, std::ios::binary);

  dst << src.rdbuf();
}

bool compare_files(const std::string filename1, const std::string filename2) {
  std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary);  // open file at the end
  std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary);  // open file at the end
  const std::ifstream::pos_type fileSize = file1.tellg();

  if (fileSize != file2.tellg()) {
    return false;  // different file size
  }

  file1.seekg(0);  // rewind
  file2.seekg(0);  // rewind

  std::istreambuf_iterator<char> begin1(file1);
  std::istreambuf_iterator<char> begin2(file2);

  return std::equal(begin1, std::istreambuf_iterator<char>(), begin2);  // Second argument is end-of-range iterator
}
}  // namespace

class mecab_dics_test : public testing::TestWithParam<const char*> {};

/**
 * equivalent test of run-dics.sh in
 * https://github.com/taku910/mecab/blob/master/mecab/tests/run-dics.sh
 */
TEST_P(mecab_dics_test, test_index_command) {
  fixture::TmpDir tmpdir;

  const std::string dictionaryDir = "../test-data/" + std::string(GetParam());
  const std::string processedDictionaryDir = tmpdir.createPath(GetParam());

  const std::string dicrc = dictionaryDir + "/dicrc";
  const std::string testCase = dictionaryDir + "/test";
  const std::string truePath = dictionaryDir + "/test.gld";
  const std::string predictPath = processedDictionaryDir + "/output.txt";

  ASSERT_TRUE(is_exists(dictionaryDir));
  ASSERT_TRUE(is_exists(dicrc));
  ASSERT_TRUE(is_exists(testCase));
  ASSERT_TRUE(is_exists(truePath));

  copy_file(dicrc, processedDictionaryDir + "/dicrc");

  // remove stdout
  ::testing::internal::CaptureStdout();
  ::testing::internal::CaptureStderr();
  {
    MAKE_ARGS(mecab_dict_index_args, "dict-index", "-d", dictionaryDir, "-o", processedDictionaryDir);
    mecab_dict_index(mecab_dict_index_args.size(), mecab_dict_index_args.data());
  }
  {
    MAKE_ARGS(mecab_main_args, "mecab", "-r", "/dev/null", "-d", processedDictionaryDir, "-o", predictPath, testCase);
    mecab_main(mecab_main_args.size(), mecab_main_args.data());
  }
  ::testing::internal::GetCapturedStdout();
  ::testing::internal::GetCapturedStderr();

  ASSERT_TRUE(compare_files(predictPath, truePath));
}

INSTANTIATE_TEST_SUITE_P(DictionaryName,
                         mecab_dics_test,
                         testing::Values("autolink", "chartype", "katakana", "latin", "ngram", "shiin", "t9"));
