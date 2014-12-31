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

#include "GUI/Window.h"
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

void PythonCallback::operator() () const
{
	if (!Function || !Py_IsInitialized()) {
		return;
	}
	CallPython(Function);
}


PythonControlCallback::PythonControlCallback(PyObject *Function)
: Function(Function)
{
	if (Function && PyCallable_Check(Function)) {
		Py_INCREF(Function);
	} else {
		Function = NULL;
	}
}

PythonControlCallback::~PythonControlCallback()
{
	if (Py_IsInitialized()) {
		Py_XDECREF(Function);
	}
}

void PythonControlCallback::operator() () const
{
	(*this)(NULL);
}

void PythonControlCallback::operator() (Control* ctrl) const
{
	if (!Function || !Py_IsInitialized()) {
		return;
	}

	PyObject *args = NULL;
	PyObject* func_code = PyObject_GetAttrString(Function, "func_code");
	PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
	const long count = PyInt_AsLong(co_argcount);
	if (count) {
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
		// FIXME: Is this right? I cant tell if these should be built by ID or index. Guess I'll find out...
		PyObject* ctrltuple = Py_BuildValue("(ii)", ctrl->Owner->WindowID, ctrl->ControlID);
		PyObject* obj = gs->ConstructObject(type, ctrltuple);
		Py_DECREF(ctrltuple);
		args = Py_BuildValue("(N)", obj);
	}
	Py_DECREF(func_code);
	Py_DECREF(co_argcount);
	CallPython(Function, args);
}
