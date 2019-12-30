from setuptools import setup, Extension

mecab = Extension(
    "mecab._C",
    sources=[
        "mecab/_C/mecab.cc",
        "mecab/_C/tagger.cc",
        "mecab/_C/cli/dictionary_compiler.cc",
        "mecab/_C/cli/dictionary_generator.cc",
        "mecab/_C/cli/cost_trainer.cc",
    ],
    libraries=["mecab"],
    library_dirs=["./mecab/lib/"],
    include_dirs=["./Includes"],
    extra_compile_args=["-std=c++11"],
    extra_link_args=["-Wl,-rpath,@loader_path/lib"],
    language="c++",
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
    entry_points={
        "console_scripts": [
            "mecab-dict-index=mecab.cli:run_mecab_dict_index",
            "mecab-dict-gen=mecab.cli:run_mecab_dict_gen",
            "mecab-cost-train=mecab.cli:run_mecab_cost_train",
        ],
    },
    package_data={"mecab": ["lib/*.dylib"]},
)
