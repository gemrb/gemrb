// SPDX-FileCopyrightText: 2016 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PythonErrors.h"

#include "Logging/Logging.h"

namespace GemRB {

static PyObject* PyError(PyObject* err, const char* msg)
{
	PyErr_Print();
	PyErr_SetString(err, msg);
	return NULL; // must return NULL
}

/* Sets RuntimeError exception and returns NULL, so this function
 * can be called in `return'.
 */
PyObject* RuntimeError(const std::string& msg)
{
	Log(ERROR, "GUIScript", "Runtime Error:");
	return PyError(PyExc_RuntimeError, msg.c_str());
}

/* Prints error msg for invalid function parameters and also the function's
 * doc string (given as an argument). Then returns NULL, so this function
 * can be called in `return'. The exception should be set by previous
 * call to e.g. PyArg_ParseTuple()
 */
PyObject* AttributeError(const std::string& doc_string)
{
	Log(ERROR, "GUIScript", "Attribute Error:");
	return PyError(PyExc_AttributeError, doc_string.c_str());
}

}
