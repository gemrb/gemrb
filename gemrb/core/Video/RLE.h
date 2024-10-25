/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2021 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef RLE_H
#define RLE_H

#include "Pixels.h"

namespace GemRB {

inline uint8_t* DecodeRLEData(const uint8_t* p, const Size& size, colorkey_t colorKey)
{
	size_t pixelCount = size.w * size.h;
	uint8_t* buffer = (uint8_t*) malloc(pixelCount);

	size_t transQueue = 0;
	for (size_t i = 0; i < pixelCount;) {
		if (transQueue) {
			std::fill_n(&buffer[i], transQueue, colorKey);
			i += transQueue;
			transQueue = 0;
		} else {
			uint8_t px = *p++;
			if (px == colorKey) {
				transQueue = std::min<size_t>(1 + *p++, pixelCount - i);
			} else {
				buffer[i] = px;
				++i;
			}
		}
	}

	return buffer;
}

inline uint8_t* FindRLEPos(uint8_t* rledata, int pitch, const Point& p, colorkey_t ck)
{
	int skipcount = p.y * pitch + p.x;
	while (skipcount > 0) {
		if (*rledata++ == ck)
			skipcount -= (*rledata++) + 1;
		else
			skipcount--;
	}

	return rledata;
}

class RLEIterator : public PixelIterator<uint8_t> {
	uint8_t* dataPos = nullptr;
	colorkey_t colorkey = 0;
	uint16_t transQueue = 0;

public:
	RLEIterator(uint8_t* p, Size s, colorkey_t ck)
		: RLEIterator(p, Forward, Forward, s, ck)
	{}

	RLEIterator(uint8_t* p, Direction x, Direction y, Size s, colorkey_t ck)
		: PixelIterator(p, x, y, s, s.w), dataPos(p), colorkey(ck)
	{}

	IPixelIterator* Clone() const noexcept override
	{
		return new RLEIterator(*this);
	}

	void Advance(int dx) noexcept override
	{
		if (dx == 0 || size.IsInvalid()) return;

		int pixelsToAdvance = xdir * dx;
		int rowsToAdvance = std::abs(pixelsToAdvance / size.w);
		int tmpx = pos.x + pixelsToAdvance % size.w;

		if (tmpx < 0) {
			++rowsToAdvance;
			tmpx = size.w + tmpx;
		} else if (tmpx >= size.w) {
			++rowsToAdvance;
			tmpx = tmpx - size.w;
		}

		if (dx < 0) {
			pos.y -= rowsToAdvance;
		} else {
			pos.y += rowsToAdvance;
		}

		pos.x = tmpx;
		assert(pos.x >= 0 && pos.x < size.w);

		while (pixelsToAdvance) {
			if (transQueue) {
				// dataPos is pointing to a "count" field
				// transQueue is the remaining number of pixels we must move "right" to increment dataPos
				// *dataPos - transQueue is the number of pixels we must move "left" to decrement dataPos
				if (pixelsToAdvance > 0) {
					// moving to the right
					if (transQueue < pixelsToAdvance) {
						pixelsToAdvance -= transQueue;
						transQueue = 0;
					} else {
						pixelsToAdvance = 0;
					}
				} else {
					// moving to the left
					if (*dataPos - transQueue < -pixelsToAdvance) {
						pixelsToAdvance += *dataPos - transQueue;
						transQueue = 0;
					} else {
						pixelsToAdvance = 0;
					}
				}
			} else {
				// dataPos is pointing to a pixel
				pixel = dataPos;
				if (*dataPos == colorkey) {
					transQueue = *++dataPos;
				}
				--pixelsToAdvance;
			}
		}
	}

	const Point& Position() const noexcept override
	{
		return pos;
	}
};

}

#endif // RLE_H
