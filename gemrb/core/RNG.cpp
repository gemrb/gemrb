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

#include <ctime>

#include "RNG.h"
#include "System/Logging.h"

namespace GemRB {

/**
 * The constructor initializes the random number generator with a 32bit integer seed
 * based on a hashed value of the current timestamp.
 * The singleton implementation guarantees that the constructor is called only once,
 * which means that the RNG is seeded only once which means that it is ok to use the
 * timestamp for seeding (because it can only be used once per second).
 */
RNG::RNG() {
	time_t now = time(NULL);
	unsigned char *ptr = (unsigned char *) &now;
	uint32_t seed = 0;

	/* The actual value of a time_t may not be portable, so we compute a “hash” of the
	 * bytes in it using a multiply-and-add technique. The factor used for
	 * multiplication normally comes out as 257, a prime and therefore a good candidate.
	 * According to Knuth the seed can be chosen arbitrarily, so whatever makes the
	 * time_t value into a compatible integer, will work.
	 */
	for (size_t i = 0; i < sizeof(now); ++i) {
		seed = seed * (UCHAR_MAX + 2u) + ptr[i];
	}

	engine.seed(seed);
}

/**
 * Singleton.
 */
RNG& RNG::getInstance() {
	static RNG instance;

	return instance;
}

/**
 * It is possible to generate random numbers from [-min, +/-max].
 * It is only necessary that the upper bound is larger or equal to the lower bound - with the exception
 * that someone wants something like rand() % -foo.
 */
int32_t RNG::rand(int32_t min, int32_t max) {
	int32_t signum = 1;
	if (min == max) {
		// For complete fairness and equal timing, this should be a roll, but let's skip it anyway
		return max;
	} else if (min == 0 && max < 0) {
		// Someone wants rand() % -foo, so we compute -rand(0, +foo)
		// This is the only time where min > max is (sort of) legal.
		// Not handling this will cause the application to crash.
		signum = -1;
		max = -max;
	} else if (min > max) {
		// makes no sense, but also gives unexpected results
		GemRB::error("RNG", "Invalid bounds for RNG! Got min %d, max %d\n", min, max);
	}

	std::uniform_int_distribution<int32_t> distribution(min, max);
	int32_t randomNum = distribution(engine);
	return signum * randomNum;
}

}
