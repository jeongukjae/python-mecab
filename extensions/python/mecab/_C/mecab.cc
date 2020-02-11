#include <iostream>

#include "PythonCommon.h"
#include "mecab/cli.h"
#include "tagger.h"

#define ADD_MECAB_CLI(function_name, function_to_run)                            \
  static PyObject* function_name(PyObject* self, PyObject* args) {               \
    PyObject* list = NULL;                                                       \
    if (!PyArg_UnpackTuple(args, "args", 1, 1, &list)) {                         \
      PyErr_SetString(PyExc_ValueError, "#function_name takes only 1 argument"); \
      return NULL;                                                               \
    }                                                                            \
    if (!PyList_Check(list)) {                                                   \
      PyErr_SetString(PyExc_TypeError, "argument must be list of str");          \
      return NULL;                                                               \
    }                                                                            \
    size_t argc = PyList_Size(list);                                             \
    char** argv = new char*[argc];                                               \
    for (size_t i = 0; i < argc; ++i) {                                          \
      PyObject* item = PyList_GetItem(list, i);                                  \
      if (!PyUnicode_Check(item)) {                                              \
        PyErr_SetString(PyExc_ValueError, "argument must be list of str");       \
        return NULL;                                                             \
      }                                                                          \
      item = PyUnicode_AsUTF8String(item);                                       \
      argv[i] = PyBytes_AsString(item);                                          \
    }                                                                            \
    function_to_run(argc, argv);                                                 \
    delete[] argv;                                                               \
    Py_INCREF(Py_None);                                                          \
    return Py_None;                                                              \
  }

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
