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

#ifndef PYTHON_CONVERSIONS_H
#define PYTHON_CONVERSIONS_H

// NOTE: Python.h has to be included first.
#include "GUIScript.h"

#include "Region.h"
#include "RGBAColor.h"
#include "Sprite2D.h"
#include "TableMgr.h"
#include "SymbolMgr.h"
#include "Logging/Logging.h"
#include "Strings/String.h"

namespace GemRB {

class DecRef
{
	PyObject* obj = nullptr;
public:
	template<typename FUNC, typename... ARGS>
	explicit DecRef(FUNC fn, ARGS&&... args) {
		obj = fn(std::forward<ARGS>(args)...);
	}
	
	~DecRef() {
		Py_XDECREF(obj);
	}
	
	operator PyObject* () const {
		return obj;
	}
	
	explicit operator bool() const noexcept {
		return obj != nullptr;
	}
};

template <typename T, template<class> class PTR = Holder>
class CObject final {
public:
	using CAP_T = PTR<T>;
	
	operator PyObject* () const
	{
		if (pycap) {
			Py_INCREF(pycap);
			return pycap;
		} else {
			Py_RETURN_NONE;
		}
	}
	
	operator CAP_T () const
	{
		static CAP_T none;
		return cap ? cap->ptr : none;
	}

	explicit CObject(PyObject *obj)
	{
		if (obj == Py_None)
			return;
		PyObject *id = PyObject_GetAttrString(obj, "ID");
		if (id)
			obj = id;
		else
			PyErr_Clear();

		pycap = obj;
		Py_INCREF(pycap);
		cap = static_cast<Capsule*>(PyCapsule_GetPointer(obj, T::ID.description));
		if (cap == nullptr) {
			Log(ERROR, "GUIScript", "Bad CObject extracted.");
		}
		Py_XDECREF(id);
	}

	explicit CObject(CAP_T ptr)
	{
		if (ptr) {
			Capsule* newcap = new Capsule(std::move(ptr));
			PyObject *obj = PyCapsule_New(newcap, T::ID.description, PyRelease);
			if (obj == nullptr) {
				// PyCapsule_New could theoretically fail
				delete newcap;
				return;
			} else {
				cap = newcap;
			}

			PyObject* kwargs = Py_BuildValue("{s:O}", "ID", obj);
			pycap = gs->ConstructObject(T::ID.description, nullptr, kwargs);
			Py_DECREF(kwargs);
		}
	}
	
	~CObject() {
		Py_XDECREF(pycap);
	}
	
private:
	static void PyRelease(PyObject *obj)
	{
		void* ptr = PyCapsule_GetPointer(obj, T::ID.description);
		delete static_cast<Capsule*>(ptr);
	}
	
	struct Capsule {
		CAP_T ptr;
		
		explicit Capsule(CAP_T ptr)
		: ptr(std::move(ptr))
		{}
	};
	Capsule* cap = nullptr;

	PyObject* pycap = nullptr;
};

// Python 3 forward compatibility
// WARNING: dont use these for new code
// they are temporary while we compete the transition to Python 3
class PyStringWrapper {
	wchar_t* buffer = nullptr;
	char* str = nullptr;
	PyObject* object = nullptr;
	Py_ssize_t len = 0;
	
public:
	PyStringWrapper(PyObject* obj, const char* encoding) noexcept {
		if (PyUnicode_Check(obj)) {
			PyObject * temp_bytes = PyUnicode_AsEncodedString(obj, encoding, "strict"); // Owned reference
			if (temp_bytes != NULL) {
				PyBytes_AsStringAndSize(temp_bytes, &str, &len);
				object = temp_bytes; // needs to outlive our use of wrap.str
			} else { // raw data...
				PyErr_Clear();
				Py_ssize_t buflen = PyUnicode_GET_LENGTH(obj);
				buffer = new wchar_t[buflen + 1];
				Py_ssize_t strlen = PyUnicode_AsWideChar(obj, buffer, buflen);
				buffer[strlen] = L'\0';
				str = reinterpret_cast<char*>(buffer);
				len = strlen * sizeof(wchar_t);
			}
		} else if (PyObject_TypeCheck(obj, &PyBytes_Type)) {
			PyBytes_AsStringAndSize(obj, &str, &len);
		}
	}
	
	PyStringWrapper(const PyStringWrapper&) = delete;
	PyStringWrapper& operator=(const PyStringWrapper&) = delete;
	
	PyStringWrapper(PyStringWrapper&& wrap) {
		std::swap(wrap.buffer, buffer);
		std::swap(wrap.str, str);
		std::swap(wrap.object, object);
	}
	
	const char* CString() const noexcept {
		return str;
	}
	
	operator StringView() const noexcept {
		return StringView(str, len);
	}
	
	~PyStringWrapper() noexcept {
		Py_XDECREF(object);
		delete[] buffer;
	}
};
PyStringWrapper PyString_AsStringView(PyObject* obj);

/*
 Conversions from PyObject
*/

Color ColorFromPy(PyObject* obj);

Point PointFromPy(PyObject* obj);

Region RectFromPy(PyObject* obj);

template <class STR>
STR ASCIIStringFromPy(PyObject* obj) {
	if (obj == nullptr) return STR();

	auto ref = DecRef(PyUnicode_AsEncodedString, obj, "ascii", "strict");
	if (ref) {
		return STR(PyBytes_AsString(ref));
	}
	return STR();
}

inline ResRef ResRefFromPy(PyObject* obj) {
	return ASCIIStringFromPy<ResRef>(obj);
}

inline ieVariable ieVariableFromPy(PyObject* obj)
{
	return ASCIIStringFromPy<ieVariable>(obj);
}

ieStrRef StrRefFromPy(PyObject* obj);

std::shared_ptr<SymbolMgr> GetSymbols(PyObject* obj);

Holder<Sprite2D> SpriteFromPy(PyObject* obj);

/*
 Conversions to PyObject
*/

PyObject* PyLong_FromStrRef(ieStrRef);

// Like PyString_FromString(), but for (ie)ResRef
PyObject* PyString_FromResRef(const ResRef& resRef);
PyObject* PyString_FromString(const char* s);
PyObject* PyString_FromStringView(StringView sv);
PyObject* PyString_FromStringObj(const std::string&);
PyObject* PyString_FromStringObj(const String&);

String PyString_AsStringObj(PyObject *obj);

template <typename STR>
PyObject* PyString_FromASCII(const STR& str)
{
	// PyUnicode_FromStringAndSize expects UTF-8 data, ascii is compatible with that
	return PyUnicode_FromStringAndSize(str.c_str(), str.length());
}
	
template <typename T>
PyObject* PyObject_FromPtr(T* p)
{
	return CObject<T>(p);
}

template <typename T>
PyObject* PyObject_FromHolder(Holder<T> h)
{
	return CObject<T>(std::move(h));
}

template <typename T, PyObject* (*F)(T), class Container>
PyObject* MakePyList(const Container &source)
{
	size_t size = source.size();
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		// SET_ITEM might be preferable to SetItem here, but MSVC6 doesn't like it.
		PyList_SetItem(list, i, F(source[i]));
	}
	return list;
}

template <typename T, class Container>
PyObject* MakePyList(const Container &source)
{
	return MakePyList<Holder<T>, &PyObject_FromHolder<T>, Container>(source);
}

}

#endif
