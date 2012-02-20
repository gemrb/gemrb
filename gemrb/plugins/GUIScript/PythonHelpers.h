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
#include "Holder.h"
#include "Interface.h"

namespace GemRB {

struct PythonCallback : public Callback {
public:
	PythonCallback(PyObject *Function);
	~PythonCallback();
	bool call();
	bool call(int);
private:
	PyObject *Function;
};


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
			PyObject *tuple = PyTuple_New(1);
			PyTuple_SET_ITEM(tuple, 0, obj);
			PyObject *ret = gs->ConstructObject(T::ID.description, tuple);
			Py_DECREF(tuple);
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
