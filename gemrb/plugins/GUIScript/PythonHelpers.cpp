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
#include "GUI/Window.h"

using namespace GemRB;

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

bool PythonCallback::operator() ()
{
	if (!Function || !Py_IsInitialized()) {
		return false;
	}
	return CallPython(Function);
}

namespace GemRB {

template <>
bool PythonObjectCallback<Control>::operator() (Control* ctrl)
{
	if (!Function || !Py_IsInitialized()) {
		return false;
	}

	PyObject *args = NULL;
	PyObject* func_code = PyObject_GetAttrString(Function, "func_code");
	PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
	const int count = PyInt_AsLong(co_argcount);
	if (/*count*/ false) { // FIXME: this code is incomplete and would break things without being finished
		assert(count == 1);
		const char* type = "Control";
		switch(ctrl->ControlType) {
			case IE_GUI_LABEL:
				type = "Label";
				break;
			case IE_GUI_EDIT:
				type = "TextEdit";
				break;
			case IE_GUI_SCROLLBAR:
				type = "ScrollBar";
				break;
			case IE_GUI_TEXTAREA:
				type = "TextArea";
				break;
			case IE_GUI_BUTTON:
				type = "Button";
				break;
			case IE_GUI_WORLDMAP:
				type = "WorldMap";
				break;
			default:
				break;
		}
		// FIXME: how do you get the window and control index?
		// I really loathe that we use an index for this :/
		PyObject* ctrltuple = Py_BuildValue("(ii)", 0, 0);
		PyObject* obj = gs->ConstructObject(type, ctrltuple);
		Py_DECREF(ctrltuple);
		args = Py_BuildValue("(i)", obj);
	}
	Py_DECREF(func_code);
	Py_DECREF(co_argcount);
	return CallPython(Function, args);
}

template <>
bool PythonObjectCallback<WindowKeyPress>::operator() (WindowKeyPress *wkp) {
	if (!Function || !Py_IsInitialized()) {
		return false;
	}

	PyObject *args = PyTuple_Pack(3, PyInt_FromLong(wkp->windowID),
																	 PyInt_FromLong(wkp->key),
																	 PyInt_FromLong(wkp->mod));
	long result = CallPythonWithReturn(Function, args);

	return result > 0;
}

static PyObject* CallPythonObject(PyObject *Function, PyObject *args) {
	if (!Function) {
		return NULL;
	}

	PyObject *ret = PyObject_CallObject(Function, args);
	Py_XDECREF( args );
	if (ret == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return NULL;
	}

	return ret;
}

bool CallPython(PyObject *Function, PyObject *args)
{
	PyObject *ret = CallPythonObject(Function, args);

	if(ret) {
		Py_DECREF(ret);

		return true;
	} else {
		return false;
	}
}

long CallPythonWithReturn(PyObject *Function, PyObject *args) {
	PyObject *ret = CallPythonObject(Function, args);

	if(ret) {
		long value = PyInt_AsLong(ret);
		Py_DECREF(ret);

		return value;
	}

	return -1;
}

}
