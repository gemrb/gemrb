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

#include "PythonConversions.h"
#include "PythonErrors.h"

#include "GameData.h"

namespace GemRB {

Holder<TableMgr> GetTable(PyObject* obj) {
	Holder<TableMgr> tm;

	PyObject* attr = PyObject_GetAttrString(obj, "ID");
	if (!attr) {
		RuntimeError("Invalid Table reference, no ID attribute.");
	} else {
		tm = gamedata->GetTable( PyInt_AsLong( attr ) );
	}
	return tm;
}

// Like PyString_FromString(), but for ResRef
PyObject* PyString_FromResRef(const ieResRef& ResRef)
{
	size_t i = strnlen(ResRef,sizeof(ieResRef));
	return PyString_FromStringAndSize( ResRef, i );
}

// Like PyString_FromString(), but for ResRef
PyObject* PyString_FromAnimID(const char* AnimID)
{
	unsigned int i = strnlen(AnimID,2);
	return PyString_FromStringAndSize( AnimID, i );
}

}
