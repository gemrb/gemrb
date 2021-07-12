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

#ifndef HOLDER_H
#define HOLDER_H

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace GemRB {

template <class T>
class Held {
public:
	Held() noexcept : RefCount(0) {}
	virtual ~Held() noexcept = default;
	void acquire() noexcept { ++RefCount; }
	void release() noexcept {
		assert(RefCount && "Broken Held usage.");
		if (--RefCount == 0) delete static_cast<T*>(this);
	}
private:
	size_t RefCount;
};

/**
 * @class Holder
 * Intrusive smart pointer.
 *
 * The class T must have member function acquire and release, such that
 * acquire increases the refcount, and release decreses the refcount and
 * frees the object if needed.
 */

template <class T>
class Holder final {
public:
	Holder(T* ptr = nullptr) noexcept
	: ptr(ptr)
	{
		if (ptr)
			ptr->acquire();
	}

	Holder(const Holder& rhs) noexcept
	: ptr(rhs.ptr)
	{
		if (ptr)
			ptr->acquire();
	}
	
	Holder(Holder&& rhs) noexcept
	{
		std::swap(rhs.ptr, ptr);
	}
	
	~Holder() noexcept
	{
		if (ptr)
			ptr->release();
	}
	
	Holder& operator=(const Holder& rhs) noexcept
	{
		if (&rhs != this) {
			ptr = rhs.ptr;
			if (ptr)
				ptr->acquire();
		}
		return *this;
	}
	
	Holder& operator=(Holder&& rhs) noexcept
	{
		if (&rhs != this) {
			std::swap(rhs.ptr, ptr);
		}
		return *this;
	}
	
	T &operator*() const noexcept {
		return *ptr;
	}
	
	T *operator->() const noexcept {
		return ptr;
	}
	
	explicit operator bool() const noexcept {
		return ptr != nullptr;
	}

	T *get() const noexcept {
		return ptr;
	}

	void release() noexcept {
		if (ptr)
			ptr->release();
		ptr = nullptr;
	}

protected:
	T *ptr = nullptr;
};

template<class T>
inline bool operator==(const Holder<T>& lhs, const Holder<T>& rhs) noexcept
{
	return lhs.get() == rhs.get();
}

template<class T>
inline bool operator==(const Holder<T>& lhs, std::nullptr_t) noexcept
{
	return !bool(lhs);
}

template<class T>
inline bool operator==(std::nullptr_t, const Holder<T>& rhs) noexcept
{
	return !bool(rhs);
}

template<class T>
inline bool operator!=(const Holder<T>& lhs, const Holder<T>& rhs) noexcept
{
	return lhs.get() != rhs.get();
}

template<class T>
inline bool operator!=(const Holder<T>& lhs, std::nullptr_t) noexcept
{
	return bool(lhs);
}

template<class T>
inline bool operator!=(std::nullptr_t, const Holder<T>& rhs) noexcept
{
	return bool(rhs);
}

template<class T, typename... ARGS>
inline Holder<T> MakeHolder(ARGS&&... args) noexcept
{
	return Holder<T>(new T(std::forward<ARGS>(args)...));
}

}

#endif
