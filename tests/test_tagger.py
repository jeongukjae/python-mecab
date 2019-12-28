from mecab import Tagger


def test_tagger():
    tagger = Tagger()
    tagger.parse("안녕")
