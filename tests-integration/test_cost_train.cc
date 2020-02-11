#include <fstream>

#include "fixture/tmpdir.h"
#include "gmock/gmock.h"
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
}  // namespace

/**
 * equivalent test of run-cost-train.sh in
 * https://github.com/taku910/mecab/blob/master/mecab/tests/run-cost-train.sh
 */
TEST(mecab_integration_test, cost_train) {
  fixture::TmpDir tmpdir;

  const std::string trainingResult =
      "              precision          recall         F\n"
      "LEVEL 0:    12.4183(57/459) 11.8998(57/479) 12.1535\n"
      "LEVEL 1:    11.7647(54/459) 11.2735(54/479) 11.5139\n"
      "LEVEL 2:    11.3290(52/459) 10.8559(52/479) 11.0874\n"
      "LEVEL 4:    11.3290(52/459) 10.8559(52/479) 11.0874";
  const std::string corpusPath = "../test-data/cost-train/training-data.txt";
  const std::string costTrainSeedPath = "../test-data/cost-train/seed";
  const std::string costTrainTestPath = "../test-data/cost-train/test-data.txt";

  ASSERT_TRUE(is_exists(corpusPath));
  ASSERT_TRUE(is_exists(costTrainSeedPath));
  ASSERT_TRUE(is_exists(costTrainTestPath));

  const std::string modelPath = tmpdir.getPath() + "/model.bin";
  const std::string dicPath = tmpdir.createPath("dic");
  const std::string testProcessedPath = tmpdir.getPath() + "/test.txt";
  const std::string evalResultPath = tmpdir.getPath() + "/result.txt";

  for (auto file : {"char.def", "dicrc", "dictionary.csv", "feature.def", "rewrite.def", "unk.def"}) {
    copy_file(costTrainSeedPath + "/" + file, tmpdir.getPath() + "/" + file);
  }

  testing::internal::CaptureStdout();
  {
    MAKE_ARGS(mecab_dict_index_args, "mecab-dict-index", "-d", costTrainSeedPath, "-o", tmpdir.getPath());
    mecab_dict_index(mecab_dict_index_args.size(), mecab_dict_index_args.data());
  }
  {
    MAKE_ARGS(mecab_cost_train_args, "mecab-cost-train", "-c", "1.0", "-p", "1", "-d", tmpdir.getPath(), "-f", "1",
              corpusPath, modelPath);
    mecab_cost_train(mecab_cost_train_args.size(), mecab_cost_train_args.data());
  }
  {
    MAKE_ARGS(mecab_dict_gen_args, "mecab-dict-gen", "-d", tmpdir.getPath(), "-m", modelPath, "-o", dicPath);
    mecab_dict_gen(mecab_dict_gen_args.size(), mecab_dict_gen_args.data());
  }
  {
    MAKE_ARGS(mecab_dict_index_args, "mecab-dict-index", "-d", dicPath, "-o", dicPath);
    mecab_dict_index(mecab_dict_index_args.size(), mecab_dict_index_args.data());
  }
  {
    MAKE_ARGS(mecab_test_gen_args, "mecab-test-gen", "-o", testProcessedPath, costTrainTestPath);
    mecab_test_gen(mecab_test_gen_args.size(), mecab_test_gen_args.data());
  }
  {
    MAKE_ARGS(run_mecab_main_args, "mecab", "-r", "/dev/null", "-d", dicPath, "-o", evalResultPath, testProcessedPath);
    mecab_main(run_mecab_main_args.size(), run_mecab_main_args.data());
  }
  {
    MAKE_ARGS(mecab_system_eval_args, "mecab-system-eval", "-l", "0 1 2 4", evalResultPath, costTrainTestPath);
    mecab_system_eval(mecab_system_eval_args.size(), mecab_system_eval_args.data());
  }

  std::string captured = testing::internal::GetCapturedStdout();
  ASSERT_THAT(captured, ::testing::HasSubstr(trainingResult));
}
