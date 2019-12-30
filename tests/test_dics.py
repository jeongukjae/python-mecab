"""
equivalent test of run-dics.sh in
https://github.com/taku910/mecab/blob/master/mecab/tests/run-dics.sh
"""
import os

import pytest

from mecab.cli import run_mecab_dict_index, run_mecab_main


@pytest.mark.parametrize(
    "test_data_path",
    [
        pytest.param("autolink"),
        pytest.param("chartype"),
        pytest.param("katakana"),
        pytest.param("latin"),
        pytest.param("ngram"),
        pytest.param("shiin"),
        pytest.param("t9"),
    ],
)
def test_dics(test_data_path, tmpdir):
    DIC_DIR = os.path.join("tests/test-data", test_data_path)
    PROCESSED_DIC_DIR = tmpdir.mkdir(test_data_path)

    DICRC = os.path.join(DIC_DIR, "dicrc")
    TEST_CASE = os.path.join(DIC_DIR, "test")
    TRUE_PATH = os.path.join(DIC_DIR, "test.gld")
    PREDICT_PATH = PROCESSED_DIC_DIR.join("output.txt")

    assert os.path.exists(DIC_DIR)
    assert os.path.exists(DICRC)
    assert os.path.exists(TEST_CASE)
    assert os.path.exists(TRUE_PATH)

    _copy_file(DICRC, PROCESSED_DIC_DIR.join("dicrc"))

    run_mecab_dict_index(["index", "-f", "euc-jp", "-c", "euc-jp", "-d", DIC_DIR, "-o", str(PROCESSED_DIC_DIR)])
    run_mecab_main(["mecab", "-r", "/dev/null", "-d", str(PROCESSED_DIC_DIR), "-o", str(PREDICT_PATH), TEST_CASE])

    with open(TRUE_PATH, "rb") as f:
        assert f.read() == PREDICT_PATH.read(mode="rb")


def _copy_file(from_, to):
    with open(from_) as f:
        to.write(f.read())
