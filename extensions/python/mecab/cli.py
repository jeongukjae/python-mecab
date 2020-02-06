import sys

from ._C import mecab_cost_train, mecab_dict_gen, mecab_dict_index, mecab_main, mecab_system_eval, mecab_test_gen


def run_mecab_main(argv=sys.argv):
    mecab_main(argv)


def run_mecab_dict_index(argv=sys.argv):
    mecab_dict_index(argv)


def run_mecab_dict_gen(argv=sys.argv):
    mecab_dict_gen(argv)


def run_mecab_cost_train(argv=sys.argv):
    mecab_cost_train(argv)


def run_mecab_system_eval(argv=sys.argv):
    mecab_system_eval(argv)


def run_mecab_test_gen(argv=sys.argv):
    mecab_test_gen(argv)
