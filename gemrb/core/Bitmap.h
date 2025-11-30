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

#include "globals.h"

#include "Region.h"

#include <vector>

namespace GemRB {

class GEM_EXPORT Bitmap final {
	std::vector<uint8_t> storage;
	Size size;
	int bytes;

	struct BitProxy {
		uint8_t& byte;
		uint8_t bit;

		BitProxy(uint8_t& byte, uint8_t bit) noexcept
			: byte(byte), bit(bit)
		{}

		void operator=(bool set) noexcept
		{
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
		storage.resize(bytes);
	}

	Bitmap(const Size& s, uint8_t pattern) noexcept
		: Bitmap(s)
	{
		fill(pattern);
	}

	explicit Bitmap(const Size& size, const std::vector<uint8_t>& in) noexcept
		: Bitmap(size)
	{
		storage = in;
	}

	Bitmap(const Bitmap& bm) noexcept
		: Bitmap(bm.size, bm.storage) {}

	BitProxy operator[](int i) noexcept
	{
		div_t res = std::div(i, 8);
		return BitProxy(storage[res.quot], res.rem);
	}

	bool operator[](int i) const noexcept
	{
		div_t res = std::div(i, 8);
		return storage[res.quot] & (1 << res.rem);
	}

	BitProxy operator[](const BasePoint& p) noexcept
	{
		return operator[](p.y * size.w + p.x);
	}

	bool operator[](const BasePoint& p) const noexcept
	{
		return operator[](p.y * size.w + p.x);
	}

	bool GetAt(const BasePoint& p, bool oobval) const noexcept
	{
		if (!size.PointInside(p)) {
			return oobval;
		}
		return operator[](p.y * size.w + p.x);
	}

	auto begin() noexcept
	{
		return storage.begin();
	}

	auto end() noexcept
	{
		return storage.end();
	}

	auto begin() const noexcept
	{
		return storage.cbegin();
	}

	auto end() const noexcept
	{
		return storage.cend();
	}

	auto data() noexcept
	{
		return storage.data();
	}

	auto data() const noexcept
	{
		return storage.data();
	}

	Size GetSize() const noexcept
	{
		return size;
	}

	int Bytes() const noexcept
	{
		return bytes;
	}

	void fill(uint8_t pattern) noexcept
	{
		std::fill(begin(), end(), pattern);
	}
};

}

#endif
