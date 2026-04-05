// SPDX-FileCopyrightText: 2016 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PYTHON_ERRORS_H
#define PYTHON_ERRORS_H

#include <Python.h>
#include <string>

namespace GemRB {
/* Sets RuntimeError exception and returns NULL, so this function
 * can be called in `return'.
 */
PyObject* RuntimeError(const std::string& msg);

/* Prints error msg for invalid function parameters and also the function's
 * doc string (given as an argument). Then returns NULL, so this function
 * can be called in `return'. The exception should be set by previous
 * call to e.g. PyArg_ParseTuple()
 */
PyObject* AttributeError(const std::string& doc_string);

}

#endif
