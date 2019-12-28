#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>
#include "_C/mecab.h"

static PyObject *mecab_parse(PyObject *self, PyObject *args)
{
  MeCab::Tagger *tagger = MeCab::createTagger("-C");
  if (!tagger) {
    const char* error = MeCab::getLastError();
    std::cerr << "mecab tagger throws error: " << error << std::endl;
    throw error;
  }

  PyObject *string = NULL;
  if (!PyArg_UnpackTuple(args, "args", 1, 1, &string))
    return NULL;

  if (!PyUnicode_Check(string))
  {
    PyErr_SetString(PyExc_TypeError, "arg must be str type");
    return NULL;
  }

  char *text;
  Py_ssize_t size;

  string = PyUnicode_AsUTF8String(string);
  PyBytes_AsStringAndSize(string, &text, &size);

  const char *result = tagger->parse(text, size);
  PyObject *resultObject = PyUnicode_FromString(result);
  Py_IncRef(resultObject);
  return resultObject;
}

static PyMethodDef mecabMethods[] = {
    {"parse", (PyCFunction)mecab_parse, METH_VARARGS, ""},
    {NULL}};

static PyModuleDef mecabModule = {PyModuleDef_HEAD_INIT, "mecab._C", "", -1, mecabMethods};

PyMODINIT_FUNC PyInit__C(void)
{
  PyObject *mecabExtension = PyModule_Create(&mecabModule);

  if (mecabExtension == NULL)
    return NULL;

  return mecabExtension;
}
