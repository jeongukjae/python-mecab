# python-mecab

![Run Test Status](https://github.com/jeongukjae/python-mecab/workflows/Run%20Test/badge.svg)
[![codecov](https://codecov.io/gh/jeongukjae/python-mecab/branch/master/graph/badge.svg)](https://codecov.io/gh/jeongukjae/python-mecab)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/0264497c53a1491cb24ecbee05dfb90f)](https://www.codacy.com/manual/jeongukjae/python-mecab?utm_source=github.com&utm_medium=referral&utm_content=jeongukjae/python-mecab&utm_campaign=Badge_Grade)

![Py Versions](https://img.shields.io/pypi/pyversions/python-mecab)
![PyPi Versions](https://img.shields.io/pypi/v/python-mecab)
![License](https://img.shields.io/pypi/l/python-mecab)

A repository to bind mecab for Python 3.5+. Not using swig nor pybind.

Support only Linux, macOS

Original source codes: [taku910/mecab](https://github.com/taku910/mecab)

## Installation

```sh
pip install python-mecab
```

## Usage

### Tagger

with [eunjeon/mecab-ko-dic](https://bitbucket.org/eunjeon/mecab-ko-dic/src/master/).

```python
>>> from mecab import Tagger
>>> tagger = Tagger() # or Tagger('path/to/dic')
>>> tagger.parse("안녕하세요. 이 프로젝트는 python-mecab입니다.")
(('안녕', 'NNG,행위,T,안녕,*,*,*,*'), ('하', 'XSV,*,F,하,*,*,*,*'), ('세요', 'EP+EF,*,F,세요,Inflect,EP,EF,시/EP/*+어요/EF/*'), ('.', 'SF,*,*,*,*,*,*,*'), ('이', 'MM,~명사,F,이,*,*,*,*'), ('프로젝트', 'NNG,*,F,프로젝트,*,*,*,*'), ('는', 'JX,*,T,는,*,*,*,*'), ('python', 'SL,*,*,*,*,*,*,*'), ('-', 'SY,*,*,*,*,*,*,*'), ('mecab', 'SL,*,*,*,*,*,*,*'), ('입니다', 'VCP+EF,*,F,입니다,Inflect,VCP,EF,이/VCP/*+ᄇ니다/EF/*'), ('.', 'SF,*,*,*,*,*,*,*'))
>>> parsed = tagger.parse("안녕하세요. 이 프로젝트는 python-mecab입니다.")
>>> print(*parsed, sep='\n')
('안녕', 'NNG,행위,T,안녕,*,*,*,*')
('하', 'XSV,*,F,하,*,*,*,*')
('세요', 'EP+EF,*,F,세요,Inflect,EP,EF,시/EP/*+어요/EF/*')
('.', 'SF,*,*,*,*,*,*,*')
('이', 'MM,~명사,F,이,*,*,*,*')
('프로젝트', 'NNG,*,F,프로젝트,*,*,*,*')
('는', 'JX,*,T,는,*,*,*,*')
('python', 'SL,*,*,*,*,*,*,*')
('-', 'SY,*,*,*,*,*,*,*')
('mecab', 'SL,*,*,*,*,*,*,*')
('입니다', 'VCP+EF,*,F,입니다,Inflect,VCP,EF,이/VCP/*+ᄇ니다/EF/*')
('.', 'SF,*,*,*,*,*,*,*')
```

## binded cli commands

- `mecab`
- `mecab-dict-index`
- `mecab-dict-gen`
- `mecab-test-gen`
- `mecab-cost-train`
- `mecab-system-eval`
