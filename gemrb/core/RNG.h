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

#include <climits>
#include <random>

#define RAND(min, max) RNG::getInstance().rand(min, max)
#define RAND_ALL() RNG::getInstance().rand()

namespace GemRB {

class GEM_EXPORT RNG {
	private:
		RNG();

		std::mt19937_64 engine;
	public:
		static RNG& getInstance();
		int32_t rand(int32_t min = 0, int32_t max = INT_MAX-1);
};

}

#endif
