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

Color ColorFromPy(PyObject* obj) {
	if (obj && PyDict_Check(obj)) {
		Color c;
		// PyLong_AsLong may return -1 on error and we would like this to = 0
		long pyVal = PyLong_AsLong(PyDict_GetItemString(obj, "r"));
		c.r = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);
		pyVal = PyLong_AsLong(PyDict_GetItemString(obj, "g"));
		c.g = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);
		pyVal = PyLong_AsLong(PyDict_GetItemString(obj, "b"));
		c.b = Clamp<unsigned char>(pyVal == -1 ? 0 : pyVal, 0, 255);

		PyObject* alpha = PyDict_GetItemString(obj, "a");
		if (alpha) {
			pyVal = PyLong_AsLong(alpha);
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
		p.x = PyLong_AsLong(pyVal);
		pyVal = PyDict_GetItemString(obj, "y");
		p.y = PyLong_AsLong(pyVal);
		return p;
	}
	return Point();
}

Region RectFromPy(PyObject* obj) {
	if (PyDict_Check(obj)) {
		Region r;
		PyObject* pyVal = PyDict_GetItemString(obj, "x");
		r.x = int(PyLong_AsLong(pyVal));
		pyVal = PyDict_GetItemString(obj, "y");
		r.y = int(PyLong_AsLong(pyVal));
		pyVal = PyDict_GetItemString(obj, "w");
		r.w = int(PyLong_AsLong(pyVal));
		pyVal = PyDict_GetItemString(obj, "h");
		r.h = int(PyLong_AsLong(pyVal));
		return r;
	}
	return Region();
}

ieStrRef StrRefFromPy(PyObject* str)
{
	return ieStrRef(PyLong_AsLong(str));
}

PyObject* PyLong_FromStrRef(ieStrRef str)
{
	return PyLong_FromLong(ieDword(str));
}

PluginHolder<SymbolMgr> GetSymbols(PyObject* obj)
{
	PluginHolder<SymbolMgr> sm;

	PyObject* id = PyObject_GetAttrString(obj, "ID");
	if (!id) {
		RuntimeError("Invalid Table reference, no ID attribute.");
	} else {
		sm = core->GetSymbol(PyLong_AsLong(id));
	}
	return sm;
}

Holder<Sprite2D> SpriteFromPy(PyObject* pypic)
{
	Holder<Sprite2D> pic;
	if (PyObject_TypeCheck( pypic, &PyUnicode_Type )) {
		ResourceHolder<ImageMgr> im = gamedata->GetResourceHolder<ImageMgr>(PyString_AsStringView(pypic));
		if (im) {
			pic = im->GetSprite2D();
		}
	} else if (pypic != Py_None) {
		pic = CObject<Sprite2D>(pypic);
	}
	return pic;
}

PyObject* PyString_FromString(const char* s)
{
	return PyUnicode_Decode(s, strlen(s), core->TLKEncoding.encoding.c_str(), "strict");
}

PyObject* PyString_FromStringView(StringView sv)
{
	return PyUnicode_Decode(sv.c_str(), sv.length(), core->TLKEncoding.encoding.c_str(), "strict");
}

// Like PyString_FromString(), but for ResRef
PyObject* PyString_FromResRef(const ResRef& resRef)
{
	return PyString_FromASCII(resRef);
}
	
PyObject* PyString_FromStringObj(const std::string& s)
{
	return PyUnicode_Decode(s.c_str(), s.length(), core->TLKEncoding.encoding.c_str(), "strict");
}
	
PyObject* PyString_FromStringObj(const String& s)
{
	// FIXME: this is wrong, python needs to know the encoding
	std::string mbstr = MBStringFromString(s);
	return PyString_FromStringObj(mbstr);
}

String PyString_AsStringObj(PyObject* obj)
{
	auto wrap = PyStringWrapper(obj, core->TLKEncoding.encoding.c_str());
	return StringFromCString(wrap.CString());
}

PyStringWrapper PyString_AsStringView(PyObject* obj)
{
	// TODO: this is the same as PyString_AsString
	// it exists to diferentiate uses so that we can weed out PyString_AsString
	// and replace them with PyUnicode_Decode or other alternatives
	return PyStringWrapper(obj, core->config.SystemEncoding.c_str());
}

}
