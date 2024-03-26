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

namespace GemRB {

class GEM_EXPORT Bitmap final
{
	uint8_t* data = nullptr;
	Size size;
	int bytes;
	
	struct BitProxy {
		uint8_t& byte;
		uint8_t bit;
		
		BitProxy(uint8_t& byte, uint8_t bit) noexcept
		: byte(byte), bit(bit)
		{}
		
		void operator=(bool set) noexcept {
			if (set) {
				byte |= (1 << bit);
			} else {
				byte &= ~(1 << bit);
			}
		}
	};
	
public:
	explicit Bitmap(const Size& size) noexcept
	: size(size)
	{
		bytes = CeilDiv<int>(size.w * size.h, 8);
		data = new uint8_t[bytes];
	}
	
	Bitmap(const Size& s, uint8_t pattern) noexcept
	: Bitmap(s)
	{
		fill(pattern);
	}
	
	explicit Bitmap(const Size& size, const uint8_t* in) noexcept
	: Bitmap(size) {
		std::copy(in, in + bytes, data);
	}

	~Bitmap() noexcept {
		delete [] data;
	}
	
	Bitmap(const Bitmap& bm) noexcept
	: Bitmap(bm.size, bm.data) {}
	
	Bitmap& operator=(const Bitmap& bm) noexcept {
		if (&bm != this) {
			size = bm.size;
			bytes = bm.bytes;
			std::copy(bm.data, bm.data + bm.bytes, data);
		}
		return *this;
	}
	
	Bitmap(Bitmap&& other) noexcept {
		std::swap(data, other.data);
		size = other.size;
		bytes = other.bytes;
	}

	Bitmap& operator=(Bitmap&& other) noexcept {
		if (&other != this) {
			std::swap(data, other.data);
			size = other.size;
			bytes = other.bytes;
		}
		return *this;
	}
	
	BitProxy operator[](int i) noexcept {
		div_t res = std::div(i, 8);
		return BitProxy(data[res.quot], res.rem);
	}
	
	bool operator[](int i) const noexcept {
		div_t res = std::div(i, 8);
		return data[res.quot] & (1 << res.rem);
	}
	
	BitProxy operator[](const Point& p) noexcept {
		return operator[](p.y * size.w + p.x);
	}
	
	bool operator[](const Point& p) const noexcept {
		return operator[](p.y * size.w + p.x);
	}
	
	bool GetAt(const Point& p, bool oobval) const noexcept {
		if (!size.PointInside(p)) {
			return oobval;
		}
		return operator[](p.y * size.w + p.x);
	}
	
	uint8_t* begin() noexcept {
		return data;
	}
	
	uint8_t* end() noexcept {
		return data + bytes;
	}
	
	const uint8_t* begin() const noexcept {
		return data;
	}
	
	const uint8_t* end() const noexcept {
		return data + bytes;
	}

	Size GetSize() const noexcept {
		return size;
	}
	
	int Bytes() const noexcept {
		return bytes;
	}
	
	void fill(uint8_t pattern) noexcept {
		std::fill(begin(), end(), pattern);
	}
};

}

#endif
