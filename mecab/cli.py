import sys

from ._C import mecab_dict_index, mecab_dict_gen


def run_mecab_dict_index():
    mecab_dict_index(sys.argv)


def run_mecab_dict_gen():
    mecab_dict_gen(sys.argv)
