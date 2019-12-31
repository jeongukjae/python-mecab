#include "tagger.h"
#include <iostream>

typedef struct {
  PyObject_HEAD;
  MeCab::Tagger* tagger;
} Tagger;

static PyObject* tagger_new(PyTypeObject* subtype, PyObject* args);
static void tagger_dealloc(Tagger* self);
static int tagger_traverse(Tagger* self, visitproc visit, void* arg);
static PyObject* tagger_parse(Tagger* self, PyObject* args);

static PyMethodDef taggerMethods[] = {{"parse", (PyCFunction)tagger_parse, METH_VARARGS, ""}, {NULL}};
static PyTypeObject taggerType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) "mecab._C.Tagger",
    sizeof(Tagger),
    0,
    (destructor)tagger_dealloc,
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
    (traverseproc)tagger_traverse,
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
    (newfunc)tagger_new,
};

static PyObject* tagger_new(PyTypeObject* subtype, PyObject* args) {
  Tagger* tagger = PyObject_GC_New(Tagger, &taggerType);
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

    std::string args = "-C -r /dev/null -d ";
    args += dicdir;
    tagger->tagger = MeCab::createTagger(args.c_str());
  } else {
    tagger->tagger = MeCab::createTagger("-C");
  }

  if (tagger->tagger == NULL) {
    PyErr_SetString(PyExc_Exception, "cannot create tagger");
    return NULL;
  }
  return (PyObject*)tagger;
}

static void tagger_dealloc(Tagger* self) {
  MeCab::deleteTagger(self->tagger);
  PyObject_GC_Del(self);
}

static int tagger_traverse(Tagger* self, visitproc visit, void* arg) {
  return 0;
}

static PyObject* tagger_parse(Tagger* self, PyObject* args) {
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

  const MeCab::Node* parsedNode = self->tagger->parseToNode(text, size);
  size_t nodeCount = 0;
  for (const MeCab::Node* node = parsedNode->next; node->next; node = node->next, ++nodeCount)
    ;

  PyObject* resultObject = PyTuple_New(nodeCount);
  size_t index = 0;
  for (const MeCab::Node* node = parsedNode->next; node->next; node = node->next) {
    PyObject* surface = PyUnicode_FromStringAndSize(node->surface, node->length);
    PyObject* feature = PyUnicode_FromString(node->feature);
    PyObject* innerTuple = PyTuple_Pack(2, surface, feature);
    PyTuple_SetItem(resultObject, index++, innerTuple);
  }
  Py_IncRef(resultObject);
  return resultObject;
}

bool initializeTaggerClass(PyObject* to) {
  if (PyType_Ready(&taggerType) < 0) {
    return false;
  }

  PyModule_AddObject(to, "Tagger", (PyObject*)&taggerType);
  return true;
}
