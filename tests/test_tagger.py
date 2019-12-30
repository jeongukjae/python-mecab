from mecab import Tagger


def test_run_tagger_without_raising_error():
    tagger = Tagger()
    tagger.parse("안녕")
