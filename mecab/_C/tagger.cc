#include <iostream>
#include "PythonCommon.h"

typedef struct {
  PyObject_HEAD

      PyObject taggerElements;
  MeCab::Tagger* tagger;
} Tagger;

static PyObject* tagger_create(PyObject* self, PyObject* args);
static void mecab_destruct_tagger(PyObject* self);
static PyObject* tagger_parse(PyObject* self, PyObject* args);

static PyMethodDef taggerMethods[] = {{"parse", (PyCFunction)tagger_parse, METH_VARARGS, ""}, {NULL}};

static PyTypeObject taggerType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) "mecab._C.Tagger",
    sizeof(Tagger),
    0,
    (destructor)mecab_destruct_tagger,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    taggerMethods,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (newfunc)tagger_create,
};

static PyObject* tagger_create(PyObject* self, PyObject* args) {
  Tagger* tagger = PyObject_NEW(Tagger, &taggerType);
  if (tagger == NULL)
    return NULL;

  PyObject* string = NULL;
  if (!PyArg_UnpackTuple(args, "args", 0, 1, &string))
    return NULL;

  if (string != NULL && !PyUnicode_Check(string)) {
    PyErr_SetString(PyExc_TypeError, "arg must be str type");
    return NULL;
  }

  if (string != NULL) {
    string = PyUnicode_AsUTF8String(string);
    const char* dicdir = PyBytes_AsString(string);

    std::string args = "-C -d ";
    args += dicdir;
    tagger->tagger = MeCab::createTagger(args.c_str());
  } else {
    tagger->tagger = MeCab::createTagger("-C");
  }

  if (!tagger->tagger) {
    PyErr_SetString(PyExc_Exception, "cannot create tagger");
    return NULL;
  }
  return (PyObject*)tagger;
}

static void mecab_destruct_tagger(PyObject* self) {
  MeCab::deleteTagger(((Tagger*)self)->tagger);
  PyObject_Del(self);
}

static PyObject* tagger_parse(PyObject* self, PyObject* args) {
  PyObject* string = NULL;
  if (!PyArg_UnpackTuple(args, "args", 1, 1, &string))
    return NULL;

  if (!PyUnicode_Check(string)) {
    PyErr_SetString(PyExc_TypeError, "arg must be str type");
    return NULL;
  }

  char* text;
  Py_ssize_t size;

  string = PyUnicode_AsUTF8String(string);
  PyBytes_AsStringAndSize(string, &text, &size);

  const char* result = ((Tagger*)self)->tagger->parse(text, size);
  PyObject* resultObject = PyUnicode_FromString(result);
  Py_IncRef(resultObject);
  return resultObject;
}
