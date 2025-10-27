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

#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "PythonErrors.h"

#include "Strings/Iconv.h"

namespace GemRB {

Color ColorFromPy(PyObject* obj)
{
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

Point PointFromPy(PyObject* obj)
{
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

Region RectFromPy(PyObject* obj)
{
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

Holder<Sprite2D> SpriteFromPy(PyObject* pypic, Size* size)
{
	Holder<Sprite2D> pic;
	if (PyObject_TypeCheck(pypic, &PyUnicode_Type)) {
		ResourceHolder<ImageMgr> im = gamedata->GetResourceHolder<ImageMgr>(PyString_AsStringView(pypic));
		if (im) {
			pic = im->GetSprite2D();
			if (size) *size = im->GetSize();
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
	return PyUnicode_Decode(reinterpret_cast<const char*>(s.c_str()), s.length() * sizeof(char16_t), "UTF-16", "strict");
}

String PyString_AsStringObj(PyObject* obj)
{
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 12
	if (PyUnicode_READY(obj) != 0) {
		Log(ERROR, "PythonConversions", "Failed to prepare a Python string for encoding.");
		return u"";
	}
#endif

	auto unicodeKind = PyUnicode_KIND(obj);
	std::string encoding = "ISO-8859-1";
	bool isWide = false;
	uint8_t encodingSize = 1;

	if (unicodeKind == PyUnicode_4BYTE_KIND) {
		encoding = "UCS-4";
		isWide = true;
		encodingSize = 4;
	} else if (unicodeKind == PyUnicode_2BYTE_KIND) {
		encoding = "UCS-2";
		isWide = true;
		encodingSize = 2;
	} else if (unicodeKind != PyUnicode_1BYTE_KIND) {
		// technically there is PyUnicode_WCHAR_KIND before v3.12
		assert(false);
	}

	if (isWide) {
		encoding += (IsBigEndian() ? "BE" : "LE");
	}

	iconv_t cd = nullptr;
	if (IsBigEndian()) {
		cd = iconv_open("UTF-16BE", encoding.c_str());
	} else {
		cd = iconv_open("UTF-16LE", encoding.c_str());
	}

	if (cd == (iconv_t) -1) {
		Log(ERROR, "PythonConversions", "iconv_open(UTF-16, {}) failed with error: {}", encoding, strerror(errno));
		return u"";
	}

	auto numCodepoints = PyUnicode_GET_LENGTH(obj);
	size_t inLen = numCodepoints * encodingSize;
	size_t outLen = numCodepoints * 4;
	size_t outLenLeft = outLen;
	String buffer(numCodepoints * 2, u'\0');

	auto in = reinterpret_cast<char*>(PyUnicode_DATA(obj));
	auto outBuf = reinterpret_cast<char*>(const_cast<char16_t*>(buffer.data()));

	size_t ret = portableIconv(cd, &in, &inLen, &outBuf, &outLenLeft);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "PythonConversions", "iconv failed to convert a Python string from {} to UTF-16 with error: {}", encoding, strerror(errno));
		return u"";
	}

	auto zero = buffer.find(u'\0');
	if (zero != decltype(buffer)::npos) {
		buffer.resize(zero);
	}

	return buffer;
}

PyStringWrapper PyString_AsStringView(PyObject* obj)
{
	// TODO: this is the same as PyString_AsString
	// it exists to differentiate uses so that we can weed out PyString_AsString
	// and replace them with PyUnicode_Decode or other alternatives
	return PyStringWrapper(obj, core->config.SystemEncoding.c_str());
}

}
