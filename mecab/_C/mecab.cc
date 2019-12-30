#include <iostream>
#include "PythonCommon.h"

#include "cli/cli.h"
#include "tagger.h"

ADD_MECAB_CLI(mecab_main_python, mecab_main);
ADD_MECAB_CLI(mecab_dict_index_python, mecab_dict_index);
ADD_MECAB_CLI(mecab_dict_gen_python, mecab_dict_gen);
ADD_MECAB_CLI(mecab_cost_train_python, mecab_cost_train);
ADD_MECAB_CLI(mecab_system_eval_python, mecab_system_eval);
ADD_MECAB_CLI(mecab_test_gen_python, mecab_test_gen);

static PyMethodDef mecabMethods[] = {{"mecab_main", mecab_main_python, METH_VARARGS, ""},
                                     {"mecab_dict_index", mecab_dict_index_python, METH_VARARGS, ""},
                                     {"mecab_dict_gen", mecab_dict_gen_python, METH_VARARGS, ""},
                                     {"mecab_cost_train", mecab_cost_train_python, METH_VARARGS, ""},
                                     {"mecab_system_eval", mecab_system_eval_python, METH_VARARGS, ""},
                                     {"mecab_test_gen", mecab_test_gen_python, METH_VARARGS, ""},
                                     {NULL}};
static PyModuleDef mecabModule = {PyModuleDef_HEAD_INIT, "mecab._C", "", -1, mecabMethods};

PyMODINIT_FUNC PyInit__C(void) {
  PyObject* mecabExtension = PyModule_Create(&mecabModule);

  if (mecabExtension == NULL) {
    return NULL;
  }

  if (!initializeTaggerClass(mecabExtension)) {
    Py_DECREF(mecabExtension);
    return NULL;
  }

  return mecabExtension;
}
