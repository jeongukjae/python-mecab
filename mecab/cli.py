import sys

from ._C import mecab_dict_index


def run_mecab_dict_index():
    mecab_dict_index(sys.argv)
