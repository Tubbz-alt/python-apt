/*
 * policy.cc - Wrapper around pkgPolicy
 *
 * Copyright 2009 Julian Andres Klode <jak@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <Python.h>
#include "apt_pkgmodule.h"
#include "generic.h"
#include <apt-pkg/policy.h>

static PyObject *Policy_NEW(PyTypeObject *type,PyObject *Args,
                                  PyObject *kwds) {
    PyObject *cache;
    char *kwlist[] = {"cache", NULL};
    if (PyArg_ParseTupleAndKeywords(Args, kwds, "O", kwlist, &cache) == 0)
        return 0;
    if (!PyObject_TypeCheck(cache, &PkgCacheType)) {
        PyErr_SetString(PyExc_TypeError,"`cache` must be a apt_pkg.Cache().");
        return 0;
    }
    pkgPolicy *policy = new pkgPolicy(GetCpp<pkgCache *>(cache));
    return CppOwnedPyObject_NEW<pkgPolicy*>(cache,&PyPolicy_Type,policy);
}

static char *Policy_GetPriority_doc = "get_priority(package: apt_pkg.Package)"
    " -> int\n\n"
    "Return the priority of the package.";

PyObject *Policy_GetPriority(PyObject *self, PyObject *arg) {
    pkgPolicy *policy = GetCpp<pkgPolicy *>(self);
    if (PyObject_TypeCheck(arg, &PackageType)) {
        pkgCache::PkgIterator pkg = GetCpp<pkgCache::PkgIterator>(arg);
        return Py_BuildValue("i", policy->GetPriority(pkg));
    } else {
        PyErr_SetString(PyExc_TypeError,"Argument must be of Package().");
        return 0;
    }
}

static char *Policy_GetCandidateVer_doc = "get_match(package: apt_pkg.Package)"
    " -> apt_pkg.Version\n\n"
    "Get the best package for the job.";

PyObject *Policy_GetCandidateVer(PyObject *self, PyObject *arg) {
    if (PyObject_TypeCheck(arg, &PackageType)) {
        pkgPolicy *policy = GetCpp<pkgPolicy *>(self);
        pkgCache::PkgIterator pkg = GetCpp<pkgCache::PkgIterator>(arg);
        pkgCache::VerIterator ver = policy->GetCandidateVer(pkg);
        return CppOwnedPyObject_NEW<pkgCache::VerIterator>(arg,&VersionType,
                                                           ver);
    } else {
        PyErr_SetString(PyExc_TypeError,"Argument must be of Package().");
        return 0;
    }
}

static char *Policy_GetMatch_doc = "get_match(package: apt_pkg.Package) -> "
    "apt_pkg.Version\n\n"
    "Return a matching version for the given package.";

static PyObject *Policy_GetMatch(PyObject *self, PyObject *arg) {
    if (PyObject_TypeCheck(arg, &PackageType) == 0) {
        PyErr_SetString(PyExc_TypeError,"Argument must be of Package().");
        return 0;
    }
    pkgPolicy *policy = GetCpp<pkgPolicy *>(self);
    pkgCache::PkgIterator pkg = GetCpp<pkgCache::PkgIterator>(arg);
    pkgCache::VerIterator ver = policy->GetMatch(pkg);
    return CppOwnedPyObject_NEW<pkgCache::VerIterator>(arg,&VersionType,ver);
}

static char *Policy_ReadPinFile_doc = "read_pinfile(filename: str) -> bool\n\n"
    "Read the pin file given by filename (e.g. '/etc/apt/preferences') and\n"
    "add it to the policy.";

static PyObject *Policy_ReadPinFile(PyObject *self, PyObject *arg) {
    if (!PyString_Check(arg))
        return 0;
    pkgPolicy *policy = GetCpp<pkgPolicy *>(self);

    return PyBool_FromLong(ReadPinFile(*policy, PyString_AsString(arg)));
}

static char *Policy_CreatePin_doc = "create_pin(type: str, pkg: str, "
    "data: str, priority: int)\n\n"
    "Create a pin for the policy. The parameter 'type' refers to one of the\n"
    "following strings: 'Version', 'Release', 'Origin'. The argument 'pkg'\n"
    "is the name of the package, the parameter 'data' refers to the value\n"
    "e.g. unstable for type='Release' and the other possible options. \n"
    "The parameter 'priority' gives the priority of the pin.";

static PyObject *Policy_CreatePin(PyObject *self, PyObject *args) {
    pkgVersionMatch::MatchType match_type;
    const char *type, *pkg, *data;
    signed short priority;
    if (PyArg_ParseTuple(args, "sssh", &type, &pkg, &data, &priority) == 0)
        return 0;
    pkgPolicy *policy = GetCpp<pkgPolicy *>(self);
    if (type == "Version" || type == "version")
        match_type = pkgVersionMatch::Version;
    if (type == "Release" || type == "release")
        match_type = pkgVersionMatch::Release;
    if (type == "Origin" || type == "origin")
        match_type = pkgVersionMatch::Origin;
    else
        match_type = pkgVersionMatch::None;
    policy->CreatePin(match_type,pkg,data,priority);
    HandleErrors();
    Py_RETURN_NONE;
}

static PyMethodDef Policy_Methods[] = {
    {"get_priority",(PyCFunction)Policy_GetPriority,METH_O,
     Policy_GetPriority_doc},
    {"get_candidate_ver",(PyCFunction)Policy_GetCandidateVer,METH_O,
     Policy_GetCandidateVer_doc},
    {"read_pinfile",(PyCFunction)Policy_ReadPinFile,METH_O,
     Policy_ReadPinFile_doc},
    {"create_pin",Policy_CreatePin,METH_VARARGS,Policy_CreatePin_doc},
    {"get_match",(PyCFunction)Policy_GetMatch,METH_O, Policy_GetMatch_doc},
    {}
};

static char *Policy_doc = "Policy(cache)\n\n"
    "Representation of the policy of the Cache object given by cache. This\n"
    "provides a superset of policy-related functionality compared to the\n"
    "DepCache class. The DepCache can be used for most purposes, but there\n"
    "may be some cases where a special policy class is needed.";

PyTypeObject PyPolicy_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
#if PY_MAJOR_VERSION < 3
    0,                                   // ob_size
#endif
    "apt_pkg.Policy",                    // tp_name
    sizeof(CppOwnedPyObject<pkgPolicy*>),// tp_basicsize
    0,                                   // tp_itemsize
    // Methods
    CppOwnedDeallocPtr<pkgPolicy*>,      // tp_dealloc
    0,                                   // tp_print
    0,                                   // tp_getattr
    0,                                   // tp_setattr
    0,                                   // tp_compare
    0,                                   // tp_repr
    0,                                   // tp_as_number
    0,                                   // tp_as_sequence
    0,                                   // tp_as_mapping
    0,                                   // tp_hash
    0,                                   // tp_call
    0,                                   // tp_str
    0,                                   // tp_getattro
    0,                                   // tp_setattro
    0,                                   // tp_as_buffer
    (Py_TPFLAGS_DEFAULT |                // tp_flags
     Py_TPFLAGS_BASETYPE),
    Policy_doc,                          // tp_doc
    0,                                   // tp_traverse
    0,                                   // tp_clear
    0,                                   // tp_richcompare
    0,                                   // tp_weaklistoffset
    0,                                   // tp_iter
    0,                                   // tp_iternext
    Policy_Methods,                // tp_methods
    0,                                   // tp_members
    0,                                   // tp_getset
    0,                                   // tp_base
    0,                                   // tp_dict
    0,                                   // tp_descr_get
    0,                                   // tp_descr_set
    0,                                   // tp_dictoffset
    0,                                   // tp_init
    0,                                   // tp_alloc
    Policy_NEW,                    // tp_new
};
