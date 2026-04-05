// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ANIMATION_H
#define ANIMATION_H

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
	bool paused = false;
	Region animArea;
	Flags flags = Flags::None;

	explicit Animation(std::vector<frame_t>, float customFPS) noexcept;
	Animation() noexcept = default;

	explicit operator bool() const
	{
		return GetFrameCount();
	}

	frame_t CurrentFrame() const;
	frame_t LastFrame();
	frame_t NextFrame();
	frame_t GetSyncedNextFrame(const Animation& master);
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
	bool IsActive() const;

private:
	std::vector<frame_t> frames;
	tick_t starttime = 0;
	tick_t lastTime = 0;
};

static_assert(std::is_nothrow_move_constructible<Animation>::value, "Animation should be noexcept MoveConstructible");

}

#endif
