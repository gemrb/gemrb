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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Animation.h,v 1.27 2006/03/29 17:37:34 avenger_teambg Exp $
 *
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include "../../includes/globals.h"
#include "../../includes/RGBAColor.h"
#include "Sprite2D.h"
#include "Region.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Animation {
private:
	Sprite2D **frames;
	unsigned int indicesCount;
	unsigned int startpos;
	unsigned long starttime;
public:
	bool endReached;
	unsigned int pos;
	bool autofree;
	int x, y;
	unsigned char fps;
	bool playReversed;
	bool gameAnimation;
	Region animArea;
	ieDword Flags;
	Animation(int count);
	~Animation(void);
	void AddFrame(Sprite2D* frame, unsigned int index);
	Sprite2D* NextFrame(void);
	Sprite2D* GetSyncedNextFrame(Animation* master);
	void release(void);
	/** Gets the i-th frame */
	Sprite2D* GetFrame(unsigned int i);
	/** Mirrors all the frames vertically */
	void MirrorAnimationVert();
	/** Mirrors all the frames horizontally */
	void MirrorAnimation();
	/** sets frame index */
	void SetPos(unsigned int index);
	/** Sets ScriptName for area animation */
	void SetScriptName(const char *name);
	/** returns the frame count */
	unsigned int GetFrameCount() { return indicesCount; }
	/** returns the current frame's index */
	int GetCurrentFrame();
	/** add other animation's animarea to self */
	void AddAnimArea(Animation* slave);
};

#endif
