/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007 The GemRB Project
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
 */

#ifndef BITMAP_H
#define BITMAP_H

#include "Region.h"
#include "Video/Pixels.h"

namespace GemRB {

class GEM_EXPORT Bitmap final
{
	uint8_t* data = nullptr;
	Size size;
	
public:
	explicit Bitmap(const Size& size) noexcept
	: data(new uint8_t[size.Area()]), size(size)
	{}
	
	Bitmap(const Size& size, const uint8_t* in) noexcept
	: Bitmap(size) {
		std::copy(in, in + size.Area(), data);
	}

	~Bitmap() noexcept {
		delete [] data;
	}
	
	Bitmap(const Bitmap&) = delete;
	Bitmap& operator=(const Bitmap&) = delete;
	
	Bitmap(Bitmap&& other) noexcept {
		std::swap(data, other.data);
		size = other.size;
	}

	Bitmap& operator=(Bitmap&& other) noexcept {
		if (&other != this) {
			std::swap(data, other.data);
			size = other.size;
		}
		return *this;
	}
	
	uint8_t& operator[](size_t i) noexcept {
		return data[i];
	}
	
	const uint8_t& operator[](size_t i) const noexcept {
		return data[i];
	}
	
	uint8_t& operator[](const Point& p) noexcept {
		return data[p.y * size.w + p.x];
	}
	
	const uint8_t& operator[](const Point& p) const noexcept {
		return data[p.y * size.w + p.x];
	}
	
	uint8_t GetAt(const Point& p, uint8_t oobval) const noexcept {
		if (p.x >= size.w || p.y >= size.h) {
			return oobval;
		}
		return data[p.y * size.w + p.x];
	}

	Size GetSize() const noexcept {
		return size;
	}
	
	template <typename... ARGS>
	PixelIterator<uint8_t> GetIterator(ARGS&&... args) noexcept {
		return PixelIterator<uint8_t>(std::forward<ARGS>(args)...);
	}
};

}

#endif
