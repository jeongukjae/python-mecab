from setuptools import setup, Extension

mecab = Extension(
    "mecab._C",
    sources=["mecab/mecab.cc"],
    libraries=["mecab"],
    library_dirs=["./lib"],
    include_dirs=["Includes"],
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
