/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#ifndef ANIMATION_H
#define ANIMATION_H

#include "RGBAColor.h"
#include "exports.h"
#include "globals.h"

#include "Region.h"
#include "Holder.h"
#include "Sprite2D.h"

#include <vector>

namespace GemRB {

#define ANI_DEFAULT_FRAMERATE	15

class GEM_EXPORT Animation {
public:
	using index_t = uint16_t;
	using frame_t = Holder<Sprite2D>;

	bool endReached;
	index_t frameIdx;
	Point pos;
	unsigned char fps;
	bool playReversed;
	bool gameAnimation;
	Region animArea;
	ieDword Flags;

	explicit Animation(std::vector<frame_t>);

	frame_t CurrentFrame() const;
	frame_t LastFrame();
	frame_t NextFrame();
	frame_t GetSyncedNextFrame(const Animation* master);
	void release(void);
	/** Gets the i-th frame */
	frame_t GetFrame(index_t i) const;
	/** Mirrors all the frames vertically */
	void MirrorAnimationVert();
	/** Mirrors all the frames horizontally */
	void MirrorAnimation();
	/** sets frame index */
	void SetFrame(index_t index);
	/** Sets ScriptName for area animation */
	void SetScriptName(const char *name);
	/** returns the frame count */
	index_t GetFrameCount() const { return indicesCount; }
	/** returns the current frame's index */
	index_t GetCurrentFrameIndex() const;
	/** add other animation's animarea to self */
	void AddAnimArea(const Animation* slave);
private:
	std::vector<frame_t> frames;
	index_t indicesCount;
	tick_t starttime;
};

}

#endif
