#include <algorithm>
#include <fstream>

#include "fixture/tmpdir.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mecab/cli/entrypoints.h"

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

/**
 * equivalent test of run-eval.sh in
 * https://github.com/taku910/mecab/blob/master/mecab/tests/run-eval.sh
 */
TEST(mecab_eval_test, check_evaluate_command) {
  fixture::TmpDir tmpdir;

  const std::string evalDataPath = "../test-data/eval";
  const std::string systemDataPath = evalDataPath + "/system";
  const std::string answerDataPath = evalDataPath + "/answer";
  const std::string trueDataPath = evalDataPath + "/test.gld";

  ASSERT_TRUE(is_exists(evalDataPath));
  ASSERT_TRUE(is_exists(systemDataPath));
  ASSERT_TRUE(is_exists(answerDataPath));
  ASSERT_TRUE(is_exists(trueDataPath));

  const std::string resultPath = tmpdir.getPath() + "/test.out";

  MAKE_ARGS(mecab_system_eval_args, "eval", "-l", "0 1 2 3 4", "-o", resultPath, systemDataPath, answerDataPath);
  mecab_system_eval(mecab_system_eval_args.size(), mecab_system_eval_args.data());

  ASSERT_TRUE(compare_files(trueDataPath, resultPath));
}
