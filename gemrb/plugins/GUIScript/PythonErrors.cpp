/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2016 The GemRB Project
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
