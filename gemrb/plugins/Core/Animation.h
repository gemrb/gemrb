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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Animation.h,v 1.13 2004/08/22 22:10:00 avenger_teambg Exp $
 *
 */

#ifndef ANIMATION_H
#define ANIMATION_H

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
	std::vector< Sprite2D*> frames;
	std::vector< int> link;
	unsigned short* indices;
	unsigned long indicesCount;
	unsigned int startpos;
	unsigned long starttime;
	bool pastLastFrame;
public:
	bool Active;
	char ResRef[9];
	unsigned char nextStanceID;
	bool autoSwitchOnEnd;
	bool endReached;
	unsigned int pos;
	bool autofree;
	int x, y;
	unsigned char BlitMode;
	unsigned char fps;
	bool playReversed, playOnce;
	Region animArea;
	Animation(unsigned short* frames, int count);
	~Animation(void);
	void AddFrame(Sprite2D* frame, int index);
	Sprite2D* NextFrame(void);
	void release(void);
	/** Gets the i-th frame */
	Sprite2D* GetFrame(unsigned long i);
	/** Sets the Animation Palette */
	void SetPalette(Color* Palette);
	bool ChangePalette;
};

#endif
