// SPDX-FileCopyrightText: 2004 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef AMBIENT_H
#define AMBIENT_H

#include "exports.h"
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
