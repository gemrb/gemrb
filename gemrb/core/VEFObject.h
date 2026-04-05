// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
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
