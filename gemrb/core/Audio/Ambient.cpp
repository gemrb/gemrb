// SPDX-FileCopyrightText: 2004 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Ambient.h"

#include "RNG.h"

#include <algorithm>

namespace GemRB {

ieWord Ambient::GetTotalGain() const
{
	ieWord g = gain;
	if (gainVariance != 0) {
		ieWord var = std::min(gainVariance, (ieWord) (gain / 2));
		g += RAND(0, 2 * var) - var;
	}
	return g;
}

tick_t Ambient::GetTotalInterval() const
{
	tick_t i = interval;
	if (intervalVariance != 0) {
		ieWord var = std::min(intervalVariance, interval / 2);
		i += RAND(0, 2 * var) - var;
	}
	return i;
}

ieDword Ambient::GetTotalPitch() const
{
	ieDword p = 100;
	if (pitchVariance != 0) {
		p += RAND(0u, 2 * pitchVariance) - pitchVariance;
	}
	return p;
}

}
