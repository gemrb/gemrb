/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2014 The GemRB Project
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

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <algorithm>
#include <deque>

namespace GemRB {

class Palette;

template <typename T>
class CircularBuffer {
private:
	typedef typename std::deque<T>::iterator CacheIterator;
	std::deque<T> _cache;
	size_t _maxSize;
public:
	CircularBuffer(size_t max)
	{
		_maxSize = max;
	};

	void Append(T item, bool unique = true) {
		if (unique) {
			CacheIterator it = std::find(_cache.begin(), _cache.end(), item);
			if (it != _cache.end()) {
				_cache.erase(it);
			}
		}
		if (_cache.size() == _maxSize) {
			_cache.pop_front();
		}
		_cache.push_back(item);
	}

	size_t Size() const
	{
		return _cache.size();
	}

	const T& Retrieve(size_t index) const
	{
		size_t len = _cache.size();
		if (len && index <= len) {
			// remap so that index 0 is the front
			return _cache[len - 1 - index];
		}
		static T none;
		return none;
	}
};

}

#endif
