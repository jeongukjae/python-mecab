from setuptools import setup, Extension

mecab = Extension(
    "mecab._C",
    sources=["mecab/_C/mecab.cc", "mecab/_C/tagger.cc"],
    libraries=["mecab"],
    library_dirs=["./lib"],
    include_dirs=["Includes"],
)

setup(
    name="python-mecab",
    version="1.0.0a1",
    python_requires=">=3.5",
    packages=["mecab"],
    ext_modules=[mecab],
    url="https://github.com/jeongukjae/python-mecab",
    author="Jeong Ukjae",
    author_email="jeongukjae@gmail.com",
)
