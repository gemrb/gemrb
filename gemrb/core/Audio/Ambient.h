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

#include "exports-core.h"
#include "globals.h"
#include "ie_types.h"

#include <vector>

namespace GemRB {

#define IE_AMBI_ENABLED 1
#define IE_AMBI_LOOPING 2 /* useless: if unset, the original looped the sound only when the game isn't paused */
#define IE_AMBI_MAIN    4 /* ignore origin/radius */
#define IE_AMBI_RANDOM  8 /* random selection instead of sequential */
#define IE_AMBI_HIMEM   16 /* skip on low-mem systems */
#define IE_AMBI_NOSAVE  32 /* GemRB internal flag: don't save to area */

class GEM_EXPORT Ambient {
public:
	Ambient() noexcept = default;
	bool operator==(const Ambient& b) const { return origin == b.origin && name == b.name; }

	const ieVariable& GetName() const { return name; }
	const Point& GetOrigin() const { return origin; }
	ieWord GetRadius() const { return radius; }
	ieWord GetGain() const { return gain; }
	ieWord GetTotalGain() const;
	ResRef GetSound(size_t i) const
	{
		if (i < sounds.size()) return sounds[i];
		return ResRef();
	}
	tick_t GetInterval() const { return interval; }
	tick_t GetIntervalVariance() const { return intervalVariance; }
	tick_t GetTotalInterval() const;
	ieDword GetTotalPitch() const;
	ieDword GetAppearance() const { return appearance; }
	ieDword GetFlags() const { return flags; }

	void SetActive() { flags |= IE_AMBI_ENABLED; };
	void SetInactive() { flags &= ~IE_AMBI_ENABLED; };

public:
	ieVariable name;
	Point origin;
	std::vector<ResRef> sounds;
	ieWord radius = 0;
	ieWord gain = 0; // percent
	ieWord gainVariance = 0;
	tick_t interval = 0; // no pauses if zero
	tick_t intervalVariance = 0;
	ieDword pitchVariance = 0;
	ieDword appearance = 0;
	ieDword flags = 0;
};

}

#endif
