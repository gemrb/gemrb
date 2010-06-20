/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "PythonHelpers.h"

StringCallback::StringCallback(const char *str)
	: Name(PyString_FromString(const_cast<char*>(str)))
{
}

StringCallback::StringCallback(PyObject *Name)
	: Name(Name)
{
	Py_INCREF(Name);
}

StringCallback::~StringCallback()
{
	Py_DECREF(Name);
}

bool StringCallback::call()
{
	if (!Name || !Py_IsInitialized()) {
		return false;
	}
	/* Borrowed reference */
	PyObject *Function = PyDict_GetItem(gs->pDict, Name);
	if (!Function || !PyCallable_Check(Function)) {
		printMessage("GUIScript", "Missing callback function:", LIGHT_RED);
		printf("%s\n", PyString_AsString(Name));
		return false;
	}
	PyObject *ret = PyObject_CallObject(Function, NULL);
	if (ret == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}
	Py_DECREF(ret);
	return true;
}

PythonCallback::PythonCallback(PyObject *Function)
	: Function(Function)
{
	if (Function && PyCallable_Check(Function)) {
		Py_INCREF(Function);
	} else {
		Function = NULL;
	}
}

PythonCallback::~PythonCallback()
{
	if (Py_IsInitialized()) {
		Py_DECREF(Function);
	}
}

bool PythonCallback::call ()
{
	if (!Function || !Py_IsInitialized()) {
		return false;
	}
	PyObject *ret = PyObject_CallObject(Function, NULL);
	if (ret == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}
	Py_DECREF(ret);
	return true;
}
