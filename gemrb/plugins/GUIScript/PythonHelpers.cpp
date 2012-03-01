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

using namespace GemRB;

static bool CallPython(PyObject *Function, PyObject *args = NULL)
{
	if (!Function) {
		return false;
	}
	PyObject *ret = PyObject_CallObject(Function, args);
	Py_XDECREF( args );
	if (ret == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}
	Py_DECREF(ret);
	return true;
}

static bool CallPython(PyObject *Function, int arg)
{
	if (!Function) {
		return false;
	}
	PyObject *args = Py_BuildValue("(i)", arg);
	return CallPython(Function, args);
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
		Py_XDECREF(Function);
	}
}

bool PythonCallback::call ()
{
	if (!Function || !Py_IsInitialized()) {
		return false;
	}
	return CallPython(Function);
}

bool PythonCallback::call (int arg)
{
	if (!Function || !Py_IsInitialized()) {
		return false;
	}
	return CallPython(Function, arg);
}
