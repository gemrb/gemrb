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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/AnimationFactory.h,v 1.4 2004/08/26 20:53:51 avenger_teambg Exp $
 *
 */

#ifndef ANIMATIONFACTORY_H
#define ANIMATIONFACTORY_H

#include "FactoryObject.h"
#include "../../includes/globals.h"
#include "Animation.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT AnimationFactory : public FactoryObject {
private:
	std::vector< unsigned short> links;
	std::vector< Sprite2D*> frames;
	std::vector< CycleEntry> cycles;
	unsigned short* FLTable;	// Frame Lookup Table
public:
	AnimationFactory(const char* ResRef);
	~AnimationFactory(void);
	void AddFrame(Sprite2D* frame, unsigned short index);
	void AddCycle(CycleEntry cycle);
	void LoadFLT(unsigned short* buffer, int count);
	Animation* GetCycle(unsigned char cycle);
	/** No descriptions */
	Sprite2D* GetFrame(unsigned short index, unsigned char cycle=0);
};

#endif
