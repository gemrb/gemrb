// SPDX-FileCopyrightText: 2014 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <algorithm>
#include <deque>

namespace GemRB {

class Palette;

template<typename T>
class CircularBuffer {
private:
	using CacheIterator = typename std::deque<T>::iterator;
	std::deque<T> _cache;
	size_t _maxSize;

public:
	explicit CircularBuffer(size_t max)
	{
		_maxSize = max;
	};

	void Append(T item, bool unique = true)
	{
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
		if (len && index < len) {
			// remap so that index 0 is the front
			return _cache[len - 1 - index];
		}
		static T none;
		return none;
	}

	CacheIterator begin()
	{
		return _cache.begin();
	}

	CacheIterator end()
	{
		return _cache.end();
	}
};

}

#endif
