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

#ifndef ANIMATIONMGR_H
#define ANIMATIONMGR_H

#include "globals.h"

#include "Animation.h"
#include "AnimationFactory.h"
#include "Plugin.h"

namespace GemRB {

class Font;

class GEM_EXPORT AnimationMgr : public ImporterBase {
public:
	using index_t = AnimationFactory::index_t;
	
	virtual index_t GetCycleSize(index_t Cycle) = 0;
	virtual std::shared_ptr<AnimationFactory> GetAnimationFactory(const ResRef &resref, bool allowCompression = true) = 0;
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	virtual Holder<Sprite2D> GetPalette() = 0;
	virtual index_t GetCycleCount() = 0;
};

}

#endif
