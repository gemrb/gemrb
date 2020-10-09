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

#include <vector>

namespace GemRB {

class Sprite2D;

#define ANI_DEFAULT_FRAMERATE	15

class GEM_EXPORT Animation {
private:
	std::vector<Holder<Sprite2D>> frames;
	unsigned int indicesCount;
	unsigned long starttime;
public:
	bool endReached;
	unsigned int pos;
	int x, y;
	unsigned char fps;
	bool playReversed;
	bool gameAnimation;
	Region animArea;
	ieDword Flags;

	Animation(int count);
	~Animation();
	void AddFrame(Holder<Sprite2D> frame, unsigned int index);
	Holder<Sprite2D> CurrentFrame() const;
	Holder<Sprite2D> LastFrame();
	Holder<Sprite2D> NextFrame();
	Holder<Sprite2D> GetSyncedNextFrame(Animation* master);
	void release(void);
	/** Gets the i-th frame */
	Holder<Sprite2D> GetFrame(unsigned int i) const;
	/** Mirrors all the frames vertically */
	void MirrorAnimationVert();
	/** Mirrors all the frames horizontally */
	void MirrorAnimation();
	/** sets frame index */
	void SetPos(unsigned int index);
	/** Sets ScriptName for area animation */
	void SetScriptName(const char *name);
	/** returns the frame count */
	unsigned int GetFrameCount() const { return indicesCount; }
	/** returns the current frame's index */
	unsigned int GetCurrentFrameIndex() const;
	/** add other animation's animarea to self */
	void AddAnimArea(Animation* slave);
};

}

#endif
