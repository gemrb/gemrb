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
#include "System/Logging.h"

namespace GemRB {

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
			cap = new Capsule(std::move(ptr));
			PyObject *obj = PyCapsule_New(cap, T::ID.description, PyRelease);
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
		
		Capsule(CAP_T ptr)
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
	const char* str = nullptr;
	PyObject* object = nullptr;
	
public:
	PyStringWrapper(PyObject* obj, const char* encoding) noexcept {
		if (PyUnicode_Check(obj)) {
			PyObject * temp_bytes = PyUnicode_AsEncodedString(obj, encoding, "strict"); // Owned reference
			if (temp_bytes != NULL) {
				str = PyBytes_AS_STRING(temp_bytes); // Borrowed pointer
				object = temp_bytes; // needs to outlive our use of wrap.str
			} else { // raw data...
				PyErr_Clear();
				Py_ssize_t buflen = PyUnicode_GET_LENGTH(obj);
				buffer = new wchar_t[buflen + 1];
				Py_ssize_t strlen = PyUnicode_AsWideChar(obj, buffer, buflen);
				buffer[strlen] = L'\0';
				str = reinterpret_cast<const char*>(buffer);
			}
		} else if (PyObject_TypeCheck(obj, &PyBytes_Type)) {
			str = PyBytes_AS_STRING(obj);
		}
	}
	
	PyStringWrapper(const PyStringWrapper&) = delete;
	PyStringWrapper& operator=(const PyStringWrapper&) = delete;
	
	PyStringWrapper(PyStringWrapper&& wrap) {
		std::swap(wrap.buffer, buffer);
		std::swap(wrap.str, str);
		std::swap(wrap.object, object);
	}
	
	operator const char*() const noexcept {
		return str;
	}
	
	~PyStringWrapper() noexcept {
		Py_XDECREF(object);
		delete[] buffer;
	}
};
PyStringWrapper PyString_AsString(PyObject* obj);
PyObject* PyString_FromString(const char* s);

/*
 Conversions from PyObject
*/

Color ColorFromPy(PyObject* obj);

Point PointFromPy(PyObject* obj);

Region RectFromPy(PyObject* obj);

ResRef ResRefFromPy(PyObject* obj);

std::shared_ptr<SymbolMgr> GetSymbols(PyObject* obj);

Holder<Sprite2D> SpriteFromPy(PyObject* obj);

/*
 Conversions to PyObject
*/

// Like PyString_FromString(), but for (ie)ResRef
PyObject* PyString_FromResRef(const ResRef& resRef);

PyObject* PyString_FromAnimID(const char* AnimID);
	
PyObject* PyString_FromStringObj(const std::string&);
PyObject* PyString_FromStringObj(const String&);

String* PyString_AsStringObj(PyObject *obj);
	
template <typename T>
PyObject* PyObject_FromPtr(T* p)
{
	return CObject<T>(p);
}

template <typename T>
PyObject* PyObject_FromHolder(Holder<T> h)
{
	return CObject<T>(h);
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

class DecRef
{
	PyObject* obj = nullptr;
public:
	template<typename FUNC, typename... ARGS>
	explicit DecRef(FUNC fn, ARGS&&... args) {
		obj = fn(std::forward<ARGS>(args)...);
	}
	
	~DecRef() {
		Py_DecRef(obj);
	}
	
	operator PyObject* () const {
		return obj;
	}
};

#endif
