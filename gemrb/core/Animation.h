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

#include "Holder.h"
#include "Region.h"
#include "Sprite2D.h"

#include <vector>

namespace GemRB {

#define ANI_DEFAULT_FRAMERATE 15.0f

class GEM_EXPORT Animation {
public:
	using index_t = uint16_t;
	using frame_t = Holder<Sprite2D>;

	enum class Flags : uint8_t {
		None = 0,
		Active = 1,
		BlendBlack = 2,
		Unused = 4, // keep compatible with AreaAnimation::Flags
		Once = 8,
		Sync = 16,
		RandStart = 32,
		AnimMask = 0xc4, // ignore unused bits
	};

	bool endReached = false;
	index_t frameIdx = 0;
	Point pos;
	float fps = ANI_DEFAULT_FRAMERATE;
	bool playReversed = false;
	bool gameAnimation = false; // is it affected by pausing?
	Region animArea;
	Flags flags = Flags::None;

	explicit Animation(std::vector<frame_t>) noexcept;
	Animation() noexcept = default;

	explicit operator bool() const
	{
		return GetFrameCount();
	}

	frame_t CurrentFrame() const;
	frame_t LastFrame();
	frame_t NextFrame();
	frame_t GetSyncedNextFrame(const Animation* master);
	/** Gets the i-th frame */
	frame_t GetFrame(index_t i) const;

	void MirrorAnimation(BlitFlags flags);
	/** sets frame index */
	void SetFrame(index_t index);
	/** returns the frame count */
	index_t GetFrameCount() const { return frames.size(); }
	/** returns the current frame's index */
	index_t GetCurrentFrameIndex() const;
	/** add other animation's animarea to self */
	void AddAnimArea(const Animation* slave);

private:
	std::vector<frame_t> frames;
	tick_t starttime = 0;
};

static_assert(std::is_nothrow_move_constructible<Animation>::value, "Animation should be noexcept MoveConstructible");

}

#endif
