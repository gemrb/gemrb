/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PYTHON_HELPERS_H
#define PYTHON_HELPERS_H

// Python.h needs to be included first.
#include "GUIScript.h"

#include "win32def.h" // For Logging

#include "Callback.h"
#include "Interface.h"

#include "GUI/Control.h"
#include "GUI/GUIScriptInterface.h"

namespace GemRB {

template <typename R>
R noop(PyObject*) {
	return R();
}

template <typename R, R (*F)(PyObject*)>
bool CallPython(PyObject* function, PyObject* args = NULL, R* retVal = NULL)
{
	if (!function) {
		return false;
	}

	PyObject *ret = PyObject_CallObject(function, args);
	Py_XDECREF( args );
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
	return CallPython<int, noop<int> >(function, args, &ret);
}

template <class P=void*, class R=void>
struct PythonCallbackBase : public Callback<P, R> {
	PythonCallbackBase(PyObject* fn)
	: Function(fn)
	{
		assert(Py_IsInitialized());
		if (Function && PyCallable_Check(Function)) {
			Py_INCREF(Function);
		} else {
			Function = NULL;
		}
	}

	virtual ~PythonCallbackBase() {
		Py_XDECREF(Function);
	}

	virtual void operator()() const {
		CallPython(Function);
	}

protected:
	PyObject *Function;
};

template <class P, class R>
struct PythonComplexCallback : public PythonCallbackBase<P, R> {
	PythonComplexCallback(PyObject* fn) : PythonCallbackBase<P, R>(fn) {}

	 virtual R operator()(P arg) const {
		// meant to be overridden by specializations
		return PythonCallbackBase<P, R>::operator()(arg);
	 }
};

typedef PythonCallbackBase<void, void> PythonCallback;
typedef PythonComplexCallback<Control*, void> PythonControlCallback;

template <>
void PythonControlCallback::operator() (Control* ctrl) const
{
	if (!ctrl || !Function) {
		return;
	}

	PyObject *args = NULL;
	PyObject* func_code = PyObject_GetAttrString(Function, "func_code");
	PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
	const long count = PyInt_AsLong(co_argcount);
	if (count) {
		assert(count <= 2);
		PyObject* obj = gs->ConstructObjectForScriptable(ctrl->GetScriptingRef());
		if (count > 1) {
			args = Py_BuildValue("(Ni)", obj, ctrl->GetValue());
		} else {
			args = Py_BuildValue("(N)", obj);
		}
	}
	Py_DECREF(func_code);
	Py_DECREF(co_argcount);
	CallPython(Function, args);
}

template <typename T>
class CObject : public Holder<T> {
private:
public:
	operator PyObject* () const
	{
		if (Holder<T>::ptr) {
			Holder<T>::ptr->acquire();
			GUIScript *gs = (GUIScript *) core->GetGUIScriptEngine();
			PyObject *obj = PyCObject_FromVoidPtrAndDesc(Holder<T>::ptr,const_cast<TypeID*>(&T::ID),PyRelease);
			PyObject* kwargs = Py_BuildValue("{s:O}", "ID", obj);
			PyObject *ret = gs->ConstructObject(T::ID.description, NULL, kwargs);
			Py_DECREF(kwargs);
			return ret;
		} else {
			Py_INCREF( Py_None );
			return Py_None;
		}
	}
	CObject(PyObject *obj)
	{
		if (obj == Py_None)
			return;
		PyObject *id = PyObject_GetAttrString(obj, "ID");
		if (id)
			obj = id;
		else
			PyErr_Clear();
		if (!PyCObject_Check(obj) || PyCObject_GetDesc(obj) != const_cast<TypeID*>(&T::ID)) {
			Log(ERROR, "GUIScript", "Bad CObject extracted.");
			Py_XDECREF(id);
			return;
		}
		Holder<T>::ptr = static_cast<T*>(PyCObject_AsVoidPtr(obj));
		Holder<T>::ptr->acquire();
		Py_XDECREF(id);
	}
	CObject(const Holder<T>& ptr)
	: Holder<T>(ptr)
	{
	}
	// This is here because of lookup order issues.
	operator bool () const
	{
		return Holder<T>::ptr;
	}
private:
	static void PyRelease(void *obj, void *desc)
	{
		if (desc != const_cast<TypeID*>(&T::ID)) {
			Log(ERROR, "GUIScript", "Bad CObject deleted.");
			return;
		}
		static_cast<T*>(obj)->release();
	}
};

template <typename T, class Container>
PyObject* MakePyList(const Container &source)
{
	size_t size = source.size();
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		// SET_ITEM might be preferable to SetItem here, but MSVC6 doesn't like it.
		PyList_SetItem(list, i, CObject<T>(source[i]));
	}
	return list;
}
	
}

#endif
