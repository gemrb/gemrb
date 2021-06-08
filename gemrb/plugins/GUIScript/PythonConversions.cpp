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
#include "Interface.h"
#include "ImageMgr.h"

namespace GemRB {

/*
 If we ever move to C++11 we can simpy use a variadic template for these...
*/

Color ColorFromPy(PyObject* obj) {
	if (PyDict_Check(obj)) {
		Color c;
		// PyInt_AsLong may return -1 on error and we would like this to = 0
		long pyVal = PyInt_AsLong(PyDict_GetItemString(obj, "r"));
		c.r = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);
		pyVal = PyInt_AsLong(PyDict_GetItemString(obj, "g"));
		c.g = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);
		pyVal = PyInt_AsLong(PyDict_GetItemString(obj, "b"));
		c.b = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);

		PyObject* alpha = PyDict_GetItemString(obj, "a");
		if (alpha) {
			pyVal = PyInt_AsLong(alpha);
			c.a = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);
		} else {
			c.a = 0xff;
		}

		return c;
	}
	return Color();
}

Point PointFromPy(PyObject* obj) {
	if (PyDict_Check(obj)) {
		Point p;
		PyObject* pyVal = PyDict_GetItemString(obj, "x");
		p.x = PyInt_AsLong(pyVal);
		pyVal = PyDict_GetItemString(obj, "y");
		p.y = PyInt_AsLong(pyVal);
		return p;
	}
	return Point();
}

Region RectFromPy(PyObject* obj) {
	if (PyDict_Check(obj)) {
		Region r;
		PyObject* pyVal = PyDict_GetItemString(obj, "x");
		r.x = int(PyInt_AsLong(pyVal));
		pyVal = PyDict_GetItemString(obj, "y");
		r.y = int(PyInt_AsLong(pyVal));
		pyVal = PyDict_GetItemString(obj, "w");
		r.w = int(PyInt_AsLong(pyVal));
		pyVal = PyDict_GetItemString(obj, "h");
		r.h = int(PyInt_AsLong(pyVal));
		return r;
	}
	return Region();
}

ResRef ResRefFromPy(PyObject* obj) {
	if (obj && PyString_Check(obj)) {
		return ResRef(PyString_AsString(obj));
	}
	return ResRef();
}

Holder<TableMgr> GetTable(PyObject* obj) {
	Holder<TableMgr> tm;

	PyObject* id = PyObject_GetAttrString(obj, "ID");
	if (!id) {
		RuntimeError("Invalid Table reference, no ID attribute.");
	} else {
		tm = gamedata->GetTable( PyInt_AsLong( id ) );
	}
	return tm;
}

Holder<SymbolMgr> GetSymbols(PyObject* obj)
{
	Holder<SymbolMgr> sm;

	PyObject* id = PyObject_GetAttrString(obj, "ID");
	if (!id) {
		RuntimeError("Invalid Table reference, no ID attribute.");
	} else {
		sm = core->GetSymbol( PyInt_AsLong( id ) );
	}
	return sm;
}

Holder<Sprite2D> SpriteFromPy(PyObject* pypic)
{
	Holder<Sprite2D> pic;
	if (PyObject_TypeCheck( pypic, &PyString_Type )) {
		ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(PyString_AsString(pypic));
		if (im) {
			pic = im->GetSprite2D();
		}
	} else if (pypic != Py_None) {
		pic = CObject<Sprite2D>(pypic);
	}
	return pic;
}

#if PY_MAJOR_VERSION >= 3
char* PyString_AsString(PyObject* obj)
{
	char* str = nullptr;
	const char* encoding;
	strlcpy(encoding, core->SystemEncoding, sizeof(encoding));
	if (!encoding) strlcpy(encoding, "UTF-8", sizeof(encoding));
	if (PyUnicode_Check(obj)) {
		PyObject * temp_bytes = PyUnicode_AsEncodedString(obj, encoding, "strict"); // Owned reference
		if (temp_bytes != NULL) {
			str = PyBytes_AS_STRING(temp_bytes); // Borrowed pointer
			Py_DECREF(temp_bytes);
		}
	}
	return str;
}
#endif

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
	
PyObject* PyString_FromStringObj(const std::string& s)
{
	return PyString_FromString(s.c_str());
}
	
PyObject* PyString_FromStringObj(const String& s)
{
	// FIXME: this is wrong, python needs to know the encoding
	char* cstr = MBCStringFromString(s);
	PyObject* pyStr = PyString_FromString(cstr);
	free(cstr);
	return pyStr;
}

}
