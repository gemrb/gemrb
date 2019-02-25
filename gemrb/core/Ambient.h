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
 
#ifndef AMBIENT_H
#define AMBIENT_H

#include "exports.h"
#include "globals.h"
#include "ie_types.h"

#include <bitset>
#include <string>
#include <vector>

namespace GemRB {

#define IE_AMBI_ENABLED 1
#define IE_AMBI_LOOPING 2	/* useless: if unset, the original looped the sound only when the game isn't paused */
#define IE_AMBI_MAIN    4	/* ignore origin/radius */
#define IE_AMBI_RANDOM  8	/* random selection instead of sequential */
#define IE_AMBI_HIMEM	16	/* skip on low-mem systems */
#define IE_AMBI_NOSAVE	32	/* GemRB internal flag: don't save to area */

class GEM_EXPORT Ambient {
public:
	Ambient();
	~Ambient();
	
	const char *getName() const { return name; }
	const Point &getOrigin() const { return origin; }
	ieWord getRadius() const { return radius; }
	ieWord getGain() const { return gain; }
	ieWord getTotalGain() const;
	char *getSound(ieDword i) const
	{
		if(i<sounds.size()) return sounds[i];
		return NULL;
	}
	ieDword getInterval() const { return interval; }
	ieDword getIntervalVariance() const { return intervalVariance; }
	ieDword getTotalInterval() const;
	ieDword getTotalPitch() const;
	ieDword getAppearance() const { return appearance; }
	ieDword getFlags() const { return flags; }
	
	void setActive();
	void setInactive();

public:
	char name[32];
	Point origin;
	std::vector<char *> sounds;
	ieWord radius;
	ieWord gain;	// percent
	ieWord gainVariance;
	ieDword interval;	// no pauses if zero
	ieDword intervalVariance;
	ieDword pitchVariance;
	ieDword appearance;
	ieDword flags;
};

}

#endif
