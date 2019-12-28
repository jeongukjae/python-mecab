from setuptools import setup, Extension

mecab = Extension(
    "mecab._C",
    sources=["mecab/mecab.cc",],
    define_macros=[
        ("DIC_VERSION", "102"),
        ("PACKAGE", '"mecab"'),
        ("VERSION", '"0.996"'),
        ("MECAB_DEFAULT_RC", '"/usr/local/etc/mecabrc"'),
    ],
    libraries=["mecab"],
    library_dirs=["./lib"],
    include_directories=["Sources"],
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
