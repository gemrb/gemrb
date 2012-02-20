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
#define IE_AMBI_POINT   2
#define IE_AMBI_MAIN    4
#define IE_AMBI_AREA    8

class GEM_EXPORT Ambient {
public:
	Ambient();
	~Ambient();
	
	/* there is a good reason to have these in the header:
	 * they are automatically inlined, so we have
	 * no roundtrips and no overhead for accessors --Divide */
	const char *getName() const { return name; }
	const Point &getOrigin() const { return origin; }
	ieWord getRadius() const { return radius; }
	ieWord getHeight() const { return height; }
	ieWord getGain() const { return gain; }
	char *getSound(ieDword i) const
	{
		if(i<sounds.size()) return sounds[i];
		return NULL;
	}
	ieDword getInterval() const { return interval; }
	ieDword getPerset() const { return perset; }
	ieDword getAppearance() const { return appearance; }
	ieDword getFlags() const { return flags; }
	
	void setActive();
	void setInactive();

public:
	char name[32];
	Point origin;
	ieWord radius;
	ieWord height;
	ieWord gain;	// percent
	std::vector<char *> sounds;
	ieDword interval;	// no pauses if zero
	ieDword perset;
	ieDword appearance;
	ieDword flags;

};

}

#endif
