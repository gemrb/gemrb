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

#include "FactoryObject.h"
#include "globals.h"
#include "Animation.h"
#include "exports.h"

class GEM_EXPORT AnimationFactory : public FactoryObject {
private:
	std::vector< Sprite2D*> frames;
	std::vector< CycleEntry> cycles;
	unsigned short* FLTable;	// Frame Lookup Table
	unsigned char* FrameData;
	int datarefcount;
public:
	AnimationFactory(const char* ResRef);
	~AnimationFactory(void);
	void AddFrame(Sprite2D* frame);
	void AddCycle(CycleEntry cycle);
	void LoadFLT(unsigned short* buffer, int count);
	void SetFrameData(unsigned char* FrameData);
	Animation* GetCycle(unsigned char cycle);
	/** No descriptions */
	Sprite2D* GetFrame(unsigned short index, unsigned char cycle=0);
	Sprite2D* GetFrameWithoutCycle(unsigned short index);
	size_t GetCycleCount() { return cycles.size(); }
	size_t GetFrameCount() { return frames.size(); }
	int GetCycleSize(int idx) { return cycles[idx].FramesCount; }
	Sprite2D* GetPaperdollImage(ieDword *Colors, Sprite2D *&Picture2,
		unsigned int type);

	void IncDataRefCount();
	void DecDataRefCount();
};

#endif
