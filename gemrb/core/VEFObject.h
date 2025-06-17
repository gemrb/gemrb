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
#ifndef VEFOBJECT_H
#define VEFOBJECT_H

#include "SClassID.h"
#include "exports.h"
#include "ie_types.h"

#include "Region.h"
#include "Sprite2D.h" // for BlitFlags until they become an enum class

#include <vector>

namespace GemRB {

class DataStream;
class Region;
class ScriptedAnimation;
struct Color;

enum class VEFTypes { INVALID = -1,
		      BAM,
		      VVC,
		      VEF,
		      _2DA };

struct ScheduleEntry {
	ResRef resourceName;
	ieDword start;
	ieDword length;
	Point offset;
	VEFTypes type;
	void* ptr;
};

class GEM_EXPORT VEFObject {
public:
	ResRef ResName;
	Point Pos; // position of the effect in game coordinates

	VEFObject() noexcept = default;
	explicit VEFObject(ScriptedAnimation* sca);
	VEFObject(const VEFObject&) = delete;
	~VEFObject();
	VEFObject& operator=(const VEFObject&) = delete;

private:
	std::vector<ScheduleEntry> entries;
	std::vector<ScheduleEntry> drawQueue;
	bool SingleObject = false;

public:
	//adds a new entry (use when loading)
	void AddEntry(const ResRef& res, ieDword st, ieDword len, Point pos, VEFTypes type, ieDword gtime);
	//renders the object
	bool UpdateDrawingState(int orientation);
	void Draw(const Region& screen, const Color& p_tint, int height, BlitFlags flags) const;
	void Load2DA(const ResRef& resource);
	void LoadVEF(DataStream* stream);
	ScriptedAnimation* GetSingleObject() const;

private:
	//clears the schedule, used internally
	void Init();
	//load a 2DA/VEF resource into the object
	VEFObject* CreateObject(const ResRef& res, SClass_ID id) const;
	// just a helper function
	void CreateObjectFromEntry(ScheduleEntry& entry) const;
	//load a BAM/VVC resource into the object
	ScriptedAnimation* CreateCell(const ResRef& res, ieDword start, ieDword end) const;
	//load a single entry from stream
	void ReadEntry(DataStream* stream);
};

}

#endif
