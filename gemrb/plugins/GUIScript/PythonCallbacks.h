// SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PYTHON_HELPERS_H
#define PYTHON_HELPERS_H

// Python.h needs to be included first.
#include "Callback.h"
#include "GUIScript.h"

#include "GUI/Control.h"
#include "GUI/GUIScriptInterface.h"
#include "Logging/Logging.h"

namespace GemRB {

template<typename R>
R noop(PyObject*)
{
	return R();
}

inline PyObject* CallObjectWrapper(PyObject* function, PyObject* args = nullptr)
{
	// PyObject_CallObject can assert if an error was triggered
	// ignore it instead of bailing out
	if (PyErr_Occurred()) {
		PyErr_Print();
		PyErr_Clear();
	}

	return PyObject_CallObject(function, args);
}

template<typename R, R (*F)(PyObject*)>
bool CallPython(PyObject* function, PyObject* args = NULL, R* retVal = NULL)
{
	if (!function) {
		return false;
	}

	PyObject* ret = CallObjectWrapper(function, args);
	Py_XDECREF(args);
	if (ret == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}

	if (retVal) {
		*retVal = F(ret);
	}
	Py_DECREF(ret);

	return true;
}

inline bool CallPython(PyObject* function, PyObject* args = NULL)
{
	int ret(-1);
	return CallPython<int, noop<int>>(function, args, &ret);
}

struct PythonCallback {
	explicit PythonCallback(PyObject* fn)
		: Function(fn)
	{
		assert(Py_IsInitialized());
		if (Function && PyCallable_Check(Function)) {
			Py_INCREF(Function);
		} else {
			Function = NULL;
		}
	}

	PythonCallback(const PythonCallback& pcb)
		: PythonCallback(pcb.Function) {}

	virtual ~PythonCallback()
	{
		Py_XDECREF(Function);
	}

	virtual void operator()() const
	{
		CallPython(Function);
	}

protected:
	PyObject* Function = nullptr;
};

template<class R, class ARG_T>
struct PythonComplexCallback : public PythonCallback {
	explicit PythonComplexCallback(PyObject* fn)
		: PythonCallback(fn) {}

	PyObject* GetArgs(ARG_T arg) const
	{
		PyObject* func_code = PyObject_GetAttrString(Function, "__code__");
		if (!func_code) return nullptr;

		PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
		const long count = PyLong_AsLong(co_argcount);
		PyObject* args = nullptr;
		if (count) {
			PyObject* obj = gs->ConstructObjectForScriptable(arg->GetScriptingRef());
			args = BuildArgs(arg, obj, count);
		}
		Py_DECREF(func_code);
		Py_DECREF(co_argcount);

		return args;
	}

	virtual PyObject* BuildArgs(ARG_T, PyObject* obj, long) const
	{
		// default implementation just passes the py_object to the callback
		// override to pass other args
		return Py_BuildValue("(N)", obj);
	}

	R operator()(ARG_T arg) const
	{
		if (!Function) {
			return;
		}

		CallPython(Function, GetArgs(arg));
	}
};

using PythonWindowCallback = PythonComplexCallback<void, Window*>;
using PythonControlCallback = PythonComplexCallback<void, Control*>;

}

#endif
