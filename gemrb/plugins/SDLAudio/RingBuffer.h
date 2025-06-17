/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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
 */

#ifndef H_SDL_RING_BUFFER
#define H_SDL_RING_BUFFER

#include <mutex>
#include <utility>
#include <vector>

namespace GemRB {

template<typename T>
class RingBuffer {
public:
	using Runner = std::pair<uint8_t, size_t>; // round, offset

	explicit RingBuffer(size_t size)
	{
		buffer.resize(size);
	}

	void Reset()
	{
		std::lock_guard<std::mutex> lock { mutex };
		consumeRunner = { 0, 0 };
		supplyRunner = { 0, 0 };
	}

	size_t Consume(T* memory, size_t length)
	{
		if (length == 0) {
			return 0;
		}

		auto lcr = consumeRunner;
		Runner lsr;
		{
			std::lock_guard<std::mutex> lock { mutex };
			lsr = supplyRunner;
		}

		size_t consumed = 0;

		// behind in same round
		if (lsr.first == lcr.first && lsr.second > lcr.second) {
			size_t ub = std::min(lcr.second + length, std::min(buffer.size(), lsr.second));
			std::copy(buffer.data() + lcr.second, buffer.data() + ub, memory);
			consumed += (ub - lcr.second);
			lcr.second += (ub - lcr.second);
			// ahead but in previous round
		} else if (lsr.first != lcr.first && lsr.second <= lcr.second) {
			size_t ub = std::min(lcr.second + length, buffer.size());
			std::copy(buffer.data() + lcr.second, buffer.data() + ub, memory);
			consumed += (ub - lcr.second);
			lcr.second += (ub - lcr.second);
		}

		if (lcr.second == buffer.size()) {
			lcr.second = 0;
			lcr.first = (lcr.first + 1) % 2;
		}

		// behind in now same round
		if (consumed < length && lsr.first == lcr.first && lsr.second > lcr.second) {
			size_t ub = std::min(lcr.second + (length - consumed), lsr.second);
			std::copy(buffer.data() + lcr.second, buffer.data() + ub, memory);
			consumed += (ub - lcr.second);
			lcr.second += (ub - lcr.second);
		}

		{
			std::lock_guard<std::mutex> lock { mutex };
			consumeRunner = lcr;
		}

		return consumed;
	}

	size_t Fill(const T* memory, size_t length)
	{
		if (length == 0) {
			return 0;
		}

		auto lsr = supplyRunner;
		Runner lcr;
		{
			std::lock_guard<std::mutex> lock { mutex };
			lcr = consumeRunner;
		}

		size_t written = 0;

		if (lsr.first == lcr.first && lsr.second >= lcr.second) {
			size_t wl = std::min(length, buffer.size() - lsr.second);
			std::copy(memory, memory + wl, buffer.data() + lsr.second);
			written += wl;
			lsr.second += wl;
		}

		if (lsr.second == buffer.size()) {
			lsr.second = 0;
			lsr.first = (lsr.first + 1) % 2;
		}

		if (written < length && lsr.first != lcr.first && lsr.second < lcr.second) {
			size_t wl = std::min(length - written, lcr.second - lsr.second);
			std::copy(memory + written, memory + written + wl, buffer.data() + lsr.second);
			written += wl;
			lsr.second += wl;
		}

		{
			std::lock_guard<std::mutex> lock { mutex };
			supplyRunner = lsr;
		}

		return written;
	}

private:
	std::mutex mutex;
	std::vector<T> buffer;
	Runner consumeRunner = { 0, 0 };
	Runner supplyRunner = { 0, 0 };
};

}

#endif
