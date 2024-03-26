/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2020 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef RNG_H
#define RNG_H

#include "exports.h"

#include "Region.h"

#include <cassert>
#include <limits>
#include <random>
#include <type_traits>

namespace GemRB {

// silence bogus warning
#ifdef _MSC_VER
#pragma warning(disable : 4146)
#endif

class GEM_EXPORT RNG {
	private:
	RNG();

	std::mt19937_64 engine;
	public:
	static RNG& getInstance();
	
	/**
	 * It is possible to generate random numbers from [-min, +/-max].
	 * It is only necessary that the upper bound is larger or equal to the lower bound - with the exception
	 * that someone wants something like rand() % -foo.
	 */
	template<typename NUM_T = int32_t>
	NUM_T rand(NUM_T min = 0, NUM_T max = std::numeric_limits<NUM_T>::max() - 1) noexcept {
		NUM_T signum = 1;
		if (min == max) {
			// For complete fairness and equal timing, this should be a roll, but let's skip it anyway
			return max;
		} else if (std::is_signed<NUM_T>::value && min == 0 && max < 0) {
			// Someone wants rand() % -foo, so we compute -rand(0, +foo)
			// This is the only time where min > max is (sort of) legal.
			// Not handling this will cause the application to crash.
			signum = -1;
			max = -max;
		} else if (min > max) {
			// makes no sense, but also gives unexpected results
			assert(false);
		}

		std::uniform_int_distribution<NUM_T> distribution(min, max);
		NUM_T randomNum = distribution(engine);
		return signum * randomNum;
	}
	
	bool randPct(float_t pct) noexcept {
		std::bernoulli_distribution distribution(pct);
		return distribution(engine);
	}
};

template<typename NUM_T = int32_t>
std::enable_if_t<sizeof(NUM_T) >= sizeof(short), NUM_T>
RAND(NUM_T min = 0, NUM_T max = std::numeric_limits<NUM_T>::max() - 1) noexcept {
	return RNG::getInstance().rand(min, max);
}

template<typename NUM_T>
std::enable_if_t<sizeof(NUM_T) < sizeof(short), NUM_T>
RAND(NUM_T min = 0, NUM_T max = std::numeric_limits<NUM_T>::max() - 1) noexcept {
	return NUM_T(RNG::getInstance().rand<short>(min, max));
}

inline Point RandomPoint(int xmin = 0, int xmax = std::numeric_limits<int>::max() - 1,
						 int ymin = 0, int ymax = std::numeric_limits<int>::max() - 1)
{
	auto x = RAND(xmin, xmax);
	auto y = RAND(ymin, ymax);
	return Point(x, y);
}

inline bool RandomFlip() noexcept {
	return RNG::getInstance().rand(0, 1);
}

}

#ifdef _MSC_VER
#pragma warning(default : 4146)
#endif

#endif
