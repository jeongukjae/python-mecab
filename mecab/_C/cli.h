#ifndef MECAB_PYTHON_CLI_H
#define MECAB_PYTHON_CLI_H

#include "PythonCommon.h"

int mecab_main(int argc, char** argv);
int mecab_dict_index(int argc, char** argv);
int mecab_dict_gen(int argc, char** argv);
int mecab_cost_train(int argc, char** argv);
int mecab_system_eval(int argc, char** argv);
int mecab_test_gen(int argc, char** argv);

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

#endif  // MECAB_PYTHON_CLI_H
