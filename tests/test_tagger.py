import os

from mecab import Tagger
from mecab.cli import run_mecab_dict_index


def test_tagger_with_test_data_katakana(tmpdir):
    DIC_DIR = "tests/test-data/katakana"
    PROCESSED_DIC_DIR = tmpdir.mkdir("katakana")

    DICRC = os.path.join(DIC_DIR, "dicrc")
    TEST_CASE = os.path.join(DIC_DIR, "test")
    TRUE_PATH = os.path.join(DIC_DIR, "test.gld")

    with open(TEST_CASE) as f:
        test_data = [line.strip() for line in f]

    with open(TRUE_PATH) as f:
        true_data = [line.strip() for line in f]

    _copy_file(DICRC, PROCESSED_DIC_DIR.join("dicrc"))
    run_mecab_dict_index(["index", "-d", DIC_DIR, "-o", str(PROCESSED_DIC_DIR)])
    tagger = Tagger(str(PROCESSED_DIC_DIR))

    # feature is on first index
    result = ["".join([parsed[1] for parsed in tagger.parse(line)]) for line in test_data]

    assert result == true_data


def _copy_file(from_, to):
    with open(from_) as f:
        to.write(f.read())

