/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

#include <map>

namespace GemRB {

template <typename KeyType>
class SpriteSheet {
protected:
	Sprite2D* Sheet;
	Region SheetRegion;
	std::map<KeyType, Region> RegionMap;

	SpriteSheet() {
		Sheet = NULL;
	}
public:
	SpriteSheet(Sprite2D* sheet) : Sheet(sheet) {
		Sheet->acquire();
		SheetRegion = Region(0,0, Sheet->Width, Sheet->Height);
	};
	virtual ~SpriteSheet() {
		if (Sheet) Sheet->release();
	}

	const Region& operator[](KeyType key) {
		return RegionMap[key];
	}

	// return the passed in region, clipped to the sprite dimensions or a region with -1 w/h if outside the sprite bounds
	const Region& MapSheetSegment(KeyType key, Region rgn) {
		Region intersection = rgn.Intersect(SheetRegion);
		if (!intersection.Dimensions().IsEmpty()) {
			if (RegionMap.insert(std::make_pair(key, intersection)).second) {
				return RegionMap[key];
			}
			// FIXME: should we return something like Region(0,0,0,0) here?
			// maybe we should just use Atlas[key] = intersection too.
		}
		const static Region nullRgn(0,0, -1,-1);
		return nullRgn;
	}

	void Draw(KeyType key, const Region& dest) const {
		if (RegionMap.find(key) != RegionMap.end()) {
			core->GetVideoDriver()->BlitSprite(Sheet, RegionMap.at(key), dest);
		}
	}
};

}

#endif  // ! SPRITESHEET_H
