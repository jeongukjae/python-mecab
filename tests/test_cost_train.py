"""
equivalent test of run-cost-train.sh in
https://github.com/taku910/mecab/blob/master/mecab/tests/run-cost-train.sh
"""

import os

import pytest

from mecab.cli import (
    run_mecab_cost_train,
    run_mecab_dict_gen,
    run_mecab_dict_index,
    run_mecab_main,
    run_mecab_system_eval,
    run_mecab_test_gen,
)

training_result = """
              precision          recall         F
LEVEL 0:    12.4183(57/459) 11.8998(57/479) 12.1535
LEVEL 1:    11.7647(54/459) 11.2735(54/479) 11.5139
LEVEL 2:    11.3290(52/459) 10.8559(52/479) 11.0874
LEVEL 4:    11.3290(52/459) 10.8559(52/479) 11.0874"""

# test multithreaded training
@pytest.mark.parametrize("threadnum", [pytest.param("1"), pytest.param("4")])
def test_cost_train(threadnum, tmpdir, capfd):
    """check parameter estimation is possible"""
    COST_TRAIN_SEED_PATH = "tests/test-data/cost-train/seed"
    CORPUS_PATH = "tests/test-data/cost-train/training-data.txt"
    COST_TRAIN_TEST_PATH = "tests/test-data/cost-train/test-data.txt"
    MODEL_PATH = str(tmpdir.join("model.bin"))
    DICTIONARY_PATH = str(tmpdir.mkdir("dic"))
    TEST_PROCESSED_PATH = str(tmpdir.join("test.txt"))
    EVAL_RESULT_PATH = str(tmpdir.join("result.txt"))

    assert os.path.exists(CORPUS_PATH)
    assert os.path.exists(COST_TRAIN_SEED_PATH)
    assert os.path.exists(COST_TRAIN_TEST_PATH)

    # move dicrc
    for filename in os.listdir(COST_TRAIN_SEED_PATH):
        _copy_file(os.path.join(COST_TRAIN_SEED_PATH, filename), tmpdir.join(filename))

    run_mecab_dict_index(["index", "-d", COST_TRAIN_SEED_PATH, "-o", str(tmpdir)])
    run_mecab_cost_train(["train", "-c", "1.0", "-p", threadnum, "-d", str(tmpdir), "-f", "1", CORPUS_PATH, MODEL_PATH])
    run_mecab_dict_gen(["dgen", "-d", str(tmpdir), "-m", MODEL_PATH, "-o", DICTIONARY_PATH])
    run_mecab_dict_index(["index", "-d", DICTIONARY_PATH, "-o", DICTIONARY_PATH])
    run_mecab_test_gen(["tgen", "-o", TEST_PROCESSED_PATH, COST_TRAIN_TEST_PATH])
    run_mecab_main(["mecab", "-r", "/dev/null", "-d", DICTIONARY_PATH, "-o", EVAL_RESULT_PATH, TEST_PROCESSED_PATH])
    run_mecab_system_eval(["eval", "-l", "0 1 2 4", EVAL_RESULT_PATH, COST_TRAIN_TEST_PATH])

    captured = capfd.readouterr().out
    assert training_result in captured


def _copy_file(from_, to):
    with open(from_) as f:
        to.write(f.read())
