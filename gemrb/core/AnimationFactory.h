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

#ifndef ANIMATIONFACTORY_H
#define ANIMATIONFACTORY_H

#include "exports.h"
#include "globals.h"

#include "Animation.h"
#include "FactoryObject.h"

namespace GemRB {

class GEM_EXPORT AnimationFactory : public FactoryObject {
public:
	using index_t = Animation::index_t;
	static constexpr index_t InvalidIndex = index_t(-1);
	
	struct CycleEntry {
		index_t FramesCount;
		index_t FirstFrame;
	};

	AnimationFactory(const ResRef &resref,
					 std::vector<Holder<Sprite2D>> frames,
					 std::vector<CycleEntry> cycles,
					 std::vector<index_t> FLTable);

	Animation* GetCycle(index_t cycle) const noexcept;
	/** No descriptions */
	Holder<Sprite2D> GetFrame(index_t index, index_t cycle = 0) const;
	Holder<Sprite2D> GetFrameWithoutCycle(index_t index) const;
	index_t GetCycleCount() const { return cycles.size(); }
	index_t GetFrameCount() const { return frames.size(); }
	index_t GetCycleSize(index_t idx) const;
	
private:
	std::vector<Holder<Sprite2D>> frames;
	std::vector<CycleEntry> cycles;
	std::vector<index_t> FLTable;	// Frame Lookup Table
};

}

#endif
