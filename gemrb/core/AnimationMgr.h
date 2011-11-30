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

class Font;

class GEM_EXPORT AnimationMgr : public Plugin {
public:
	AnimationMgr(void);
	virtual ~AnimationMgr(void);
	virtual bool Open(DataStream* stream) = 0;
	virtual int GetCycleSize(unsigned char Cycle) = 0;
	virtual AnimationFactory* GetAnimationFactory(const char* ResRef,
		unsigned char mode = IE_NORMAL) = 0;
	/** This function will load the Animation as a Font */
	virtual Font* GetFont(ieWord FirstChar) = 0;
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	virtual Sprite2D* GetPalette() = 0;
	virtual int GetCycleCount() = 0;
};

#endif
