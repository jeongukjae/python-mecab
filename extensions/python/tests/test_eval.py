"""
equivalent test of run-eval.sh in
https://github.com/taku910/mecab/blob/master/mecab/tests/run-eval.sh
"""
import os

from mecab.cli import run_mecab_system_eval


def test_eval(tmpdir):
    """check evaluate command"""
    EVAL_DATA_PATH = "../../test-data/eval"
    SYSTEM_DATA_PATH = os.path.join(EVAL_DATA_PATH, "system")
    ANSWER_DATA_PATH = os.path.join(EVAL_DATA_PATH, "answer")
    TRUE_DATA_PATH = os.path.join(EVAL_DATA_PATH, "test.gld")

    assert os.path.exists(EVAL_DATA_PATH)
    assert os.path.exists(SYSTEM_DATA_PATH)
    assert os.path.exists(ANSWER_DATA_PATH)
    assert os.path.exists(TRUE_DATA_PATH)

    RESULT_PATH = tmpdir.join("test.out")

    run_mecab_system_eval(["eval", "-l", "0 1 2 3 4", "-o", str(RESULT_PATH), SYSTEM_DATA_PATH, ANSWER_DATA_PATH])

    with open(TRUE_DATA_PATH, "rb") as f:
        assert f.read() == RESULT_PATH.read(mode="rb")
