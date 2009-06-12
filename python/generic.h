// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: generic.h,v 1.4 2002/03/10 05:45:34 mdz Exp $
/* ######################################################################

   generic - Some handy functions to make integration a tad simpler

   Python needs this little _HEAD tacked onto the front of the object..
   This complicates the integration with C++. We use some templates to
   make that quite transparent to us. It would have been nice if Python
   internally used a page from the C++ ref counting book to hide its little
   header from the world, but it doesn't.

   The CppPyObject has the target object and the Python header, this is
   needed to ensure proper alignment.
   GetCpp returns the C++ object from a PyObject.
   CppPyObject_NEW creates the Python object and then uses placement new
     to init the C++ class.. This is good for simple situations and as an
     example on how to do it in other more specific cases.
   CppPyObject_Dealloc should be used in the Type as the destructor
     function.
   HandleErrors converts errors from the internal _error stack into Python
     exceptions and makes sure the _error stack is empty.

   ##################################################################### */
									/*}}}*/
#ifndef GENERIC_H
#define GENERIC_H

#include <Python.h>
#include <string>
#include <new>

#if PYTHON_API_VERSION < 1013
typedef int Py_ssize_t;
#endif

/* Define compatibility for Python 3.
 *
 * We will use the names PyString_* to refer to the default string type
 * of the current Python version (PyString on 2.X, PyUnicode on 3.X).
 *
 * When we really need unicode strings, we will use PyUnicode_* directly, as
 * long as it exists in Python 2 and Python 3.
 *
 * When we want bytes in Python 3, we use PyBytes*_ instead of PyString_* and
 * define aliases from PyBytes_* to PyString_* for Python 2.
 */

#if PY_MAJOR_VERSION >= 3
#define PyString_Check PyUnicode_Check
#define PyString_FromString PyUnicode_FromString
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_AsString(op) PyBytes_AsString(PyUnicode_AsUTF8String(op))
#define PyInt_Check PyLong_Check
#define PyInt_AsLong PyLong_AsLong
// Force 0.7 compatibility to be off in Python 3 builds
#undef COMPAT_0_7
#else
#define PyBytes_Check PyString_Check
#define PyBytes_AsString PyString_AsString
#define PyBytes_AsStringAndSize PyString_AsStringAndSize
#endif

template <class T> struct CppPyObject : public PyObject
{
   // We are only using CppPyObject and friends as dumb structs only, ie the
   // c'tor is never called.
   // However if T doesn't have a default c'tor C++ doesn't generate one for
   // CppPyObject (since it can't know how it should initialize Object).
   //
   // This causes problems then in CppOwnedPyObject, for which C++ can't create
   // a c'tor that calls the base class c'tor (which causes a compilation
   // error).
   // So basically having the c'tor here removes the need for T to have a
   // default c'tor, which is not always desireable.
   CppPyObject() { };
   T Object;
};

template <class T> struct CppOwnedPyObject : public CppPyObject<T>
{
   PyObject *Owner;
};

template <class T>
inline T &GetCpp(PyObject *Obj)
{
   return ((CppPyObject<T> *)Obj)->Object;
}

template <class T>
inline PyObject *GetOwner(PyObject *Obj)
{
   return ((CppOwnedPyObject<T> *)Obj)->Owner;
}

// Generic 'new' functions
template <class T>
inline CppPyObject<T> *CppPyObject_NEW(PyTypeObject *Type)
{
   CppPyObject<T> *New = (CppPyObject<T>*)Type->tp_alloc(Type, 0);
   new (&New->Object) T;
   return New;
}

template <class T,class A>
inline CppPyObject<T> *CppPyObject_NEW(PyTypeObject *Type,A const &Arg)
{
   CppPyObject<T> *New = (CppPyObject<T>*)Type->tp_alloc(Type, 0);
   new (&New->Object) T(Arg);
   return New;
}

template <class T>
inline CppOwnedPyObject<T> *CppOwnedPyObject_NEW(PyObject *Owner,
						 PyTypeObject *Type)
{
   CppOwnedPyObject<T> *New = (CppOwnedPyObject<T>*)Type->tp_alloc(Type, 0);
   new (&New->Object) T;
   New->Owner = Owner;
   Py_INCREF(Owner);
   return New;
}

template <class T,class A>
inline CppOwnedPyObject<T> *CppOwnedPyObject_NEW(PyObject *Owner,
						 PyTypeObject *Type,A const &Arg)
{
   CppOwnedPyObject<T> *New = (CppOwnedPyObject<T>*)Type->tp_alloc(Type, 0);
   new (&New->Object) T(Arg);
   New->Owner = Owner;
   if (Owner != 0)
      Py_INCREF(Owner);
   return New;
}

// Generic Dealloc type functions
template <class T>
void CppDealloc(PyObject *Obj)
{
   GetCpp<T>(Obj).~T();
   Obj->ob_type->tp_free(Obj);
}

template <class T>
void CppOwnedDealloc(PyObject *iObj)
{
   CppOwnedPyObject<T> *Obj = (CppOwnedPyObject<T> *)iObj;
   Obj->Object.~T();
   if (Obj->Owner != 0)
      Py_DECREF(Obj->Owner);
   iObj->ob_type->tp_free(iObj);
}

// Pointer deallocation
// Generic Dealloc type functions
template <class T>
void CppDeallocPtr(PyObject *Obj)
{
   delete GetCpp<T>(Obj);
   Obj->ob_type->tp_free(Obj);
}

template <class T>
void CppOwnedDeallocPtr(PyObject *iObj)
{
   CppOwnedPyObject<T> *Obj = (CppOwnedPyObject<T> *)iObj;
   delete Obj->Object;
   if (Obj->Owner != 0)
      Py_DECREF(Obj->Owner);
   iObj->ob_type->tp_free(iObj);
}

inline PyObject *CppPyString(std::string Str)
{
   return PyString_FromStringAndSize(Str.c_str(),Str.length());
}

inline PyObject *Safe_FromString(const char *Str)
{
   if (Str == 0)
      return PyString_FromString("");
   return PyString_FromString(Str);
}

// Convert _error into Python exceptions
PyObject *HandleErrors(PyObject *Res = 0);

// Convert a list of strings to a char **
const char **ListToCharChar(PyObject *List,bool NullTerm = false);
PyObject *CharCharToList(const char **List,unsigned long Size = 0);

#endif
