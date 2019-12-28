#include <iostream>
#include "PythonCommon.h"

#include "dictionary_compiler.cc"
#include "tagger.cc"

static PyMethodDef mecabMethods[] = {{"mecab_dict_index", (PyCFunction)mecab_dict_index, METH_VARARGS, ""}, {NULL}};
static PyModuleDef mecabModule = {PyModuleDef_HEAD_INIT, "mecab._C", "", -1, mecabMethods};

PyMODINIT_FUNC PyInit__C(void) {
  PyObject* mecabExtension = PyModule_Create(&mecabModule);

  if (mecabExtension == NULL)
    return NULL;

  if (PyType_Ready(&taggerType) < 0) {
    Py_DECREF(mecabExtension);
    return NULL;
  } else
    PyModule_AddObject(mecabExtension, "Tagger", (PyObject*)&taggerType);

  return mecabExtension;
}
