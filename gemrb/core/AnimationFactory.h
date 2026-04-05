// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ANIMATIONFACTORY_H
#define ANIMATIONFACTORY_H

#include "exports.h"

#include "Animation.h"
#include "FactoryObject.h"
#include "Sprite2D.h"

namespace GemRB {

class GEM_EXPORT AnimationFactory : public FactoryObject {
public:
	using index_t = Animation::index_t;
	static constexpr index_t InvalidIndex = index_t(-1);

	struct CycleEntry {
		index_t FramesCount;
		index_t FirstFrame;
	};

	AnimationFactory(const ResRef& resref,
			 std::vector<Holder<Sprite2D>> frames,
			 std::vector<CycleEntry> cycles,
			 std::vector<index_t> FLTable);

	Holder<Animation> GetCycle(index_t cycle) const noexcept;
	/** No descriptions */
	Holder<Sprite2D> GetFrame(index_t index, index_t cycle = 0) const;
	Holder<Sprite2D> GetFrameWithoutCycle(index_t index) const;
	index_t GetCycleCount() const { return cycles.size(); }
	index_t GetFrameCount() const { return frames.size(); }
	index_t GetCycleSize(index_t idx) const;

private:
	std::vector<Holder<Sprite2D>> frames;
	std::vector<CycleEntry> cycles;
	std::vector<index_t> FLTable; // Frame Lookup Table
	float fps = ANI_DEFAULT_FRAMERATE; // comes from animfps.2da
};

}

#endif
