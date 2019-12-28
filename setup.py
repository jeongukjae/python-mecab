from setuptools import setup, Extension

mecab = Extension(
    "mecab._C",
    sources=[
        "mecab/mecab.cc",
        "mecab/_C/char_property.cpp",
        "mecab/_C/connector.cpp",
        "mecab/_C/context_id.cpp",
        "mecab/_C/dictionary.cpp",
        "mecab/_C/dictionary_compiler.cpp",
        "mecab/_C/dictionary_generator.cpp",
        "mecab/_C/dictionary_rewriter.cpp",
        "mecab/_C/eval.cpp",
        "mecab/_C/feature_index.cpp",
        "mecab/_C/iconv_utils.cpp",
        "mecab/_C/lbfgs.cpp",
        "mecab/_C/learner.cpp",
        "mecab/_C/learner_tagger.cpp",
        "mecab/_C/libmecab.cpp",
        "mecab/_C/nbest_generator.cpp",
        "mecab/_C/param.cpp",
        "mecab/_C/string_buffer.cpp",
        "mecab/_C/tagger.cpp",
        "mecab/_C/tokenizer.cpp",
        "mecab/_C/utils.cpp",
        "mecab/_C/viterbi.cpp",
        "mecab/_C/writer.cpp",
    ],
    define_macros=[
        ("DIC_VERSION", "102"),
        ("PACKAGE", '"mecab"'),
        ("VERSION", '"0.996"'),
        ("MECAB_DEFAULT_RC", '"/usr/local/etc/mecabrc"'),
    ],
    libraries=["mecab"],
    library_dirs=["./lib"],
)

setup(
    name="mecab-python",
    version="1.0.0a0",
    python_requires=">=3.5",
    packages=["mecab"],
    ext_modules=[mecab],
    url="https://github.com/jeongukjae/mecab-python",
    author="Jeong Ukjae",
    author_email="jeongukjae@gmail.com",
)
