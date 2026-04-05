// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RNG.h"

#include <ctime>

namespace GemRB {

/**
 * The constructor initializes the random number generator with a 32bit integer seed
 * based on a hashed value of the current timestamp.
 * The singleton implementation guarantees that the constructor is called only once,
 * which means that the RNG is seeded only once which means that it is ok to use the
 * timestamp for seeding (because it can only be used once per second).
 */
RNG::RNG()
{
	time_t now = time(NULL);
	const unsigned char* ptr = (unsigned char*) &now;
	uint32_t seed = 0;

	/* The actual value of a time_t may not be portable, so we compute a “hash” of the
	 * bytes in it using a multiply-and-add technique. The factor used for
	 * multiplication normally comes out as 257, a prime and therefore a good candidate.
	 * According to Knuth the seed can be chosen arbitrarily, so whatever makes the
	 * time_t value into a compatible integer, will work.
	 */
	for (size_t i = 0; i < sizeof(now); ++i) {
		seed = seed * (std::numeric_limits<unsigned char>::max() + 2u) + ptr[i];
	}

	engine.seed(seed);
}

/**
 * Singleton.
 */
RNG& RNG::getInstance()
{
	static thread_local RNG instance;
	return instance;
}

}
