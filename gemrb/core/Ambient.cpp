/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
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
 
#include "Ambient.h"
#include "RNG.h"

#include <algorithm>

namespace GemRB {

Ambient::Ambient()
{
	name[0] = 0;
	radius = 0;
	gain = gainVariance = interval = intervalVariance = 0;
	pitchVariance = appearance = flags = 0;
}

Ambient::~Ambient()
{
	unsigned int i=sounds.size();
	while(i--) {
		free(sounds[i]);
	}
}

ieWord Ambient::getTotalGain() const
{
	ieWord g = gain;
	if (gainVariance != 0) {
		ieWord var = std::min(gainVariance, (ieWord) (gain/2));
		g += RAND(0, 2 * var) - var;
	}
	return g;
}

ieDword Ambient::getTotalInterval() const
{
	ieDword i = interval;
	if (intervalVariance != 0) {
		ieWord var = std::min(intervalVariance, (ieDword) (interval/2));
		i += RAND(0, 2 * var) - var;
	}
	return i;
}

ieDword Ambient::getTotalPitch() const
{
	ieDword p = 100;
	if (pitchVariance != 0) {
		p += RAND(0, 2 * pitchVariance) - pitchVariance;
	}
	return p;
}

void Ambient::setActive() { flags |= IE_AMBI_ENABLED; }
void Ambient::setInactive() { flags &= ~IE_AMBI_ENABLED; }

}
