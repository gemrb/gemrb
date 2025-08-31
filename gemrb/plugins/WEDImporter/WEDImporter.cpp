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

#include "WEDImporter.h"

#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"

#include "Logging/Logging.h"
#include "Plugins/TileSetMgr.h"

#include <iterator>

using namespace GemRB;

//the net sizeof(wed_polygon) is 0x12 but not all compilers know that
#define WED_POLYGON_SIZE 0x12

WEDImporter::~WEDImporter(void)
{
	delete str;
}

bool WEDImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read(Signature, 8);
	if (strncmp(Signature, "WED V1.3", 8) != 0) {
		Log(ERROR, "WEDImporter", "This file is not a valid WED File! Actual signature: {}", Signature);
		return false;
	}
	str->ReadDword(OverlaysCount);
	str->ReadDword(DoorsCount);
	str->ReadDword(OverlaysOffset);
	str->ReadDword(SecHeaderOffset);
	str->ReadDword(DoorsOffset);
	str->ReadDword(DoorTilesOffset);
	// currently unused fields from the original; likely unused completely — even commented out in wed.go implementation
	//   WORD    nVisiblityRange;
	//   WORD    nChanceOfRain; - likely unused, since it's present in the ARE file
	//   WORD    nChanceOfFog; - most likely unused, since it's present in the ARE file
	//   WORD    nChanceOfSnow; - most likely unused, since it's present in the ARE file
	//   DWORD   dwFlags;

	str->Seek(OverlaysOffset, GEM_STREAM_START);
	for (unsigned int i = 0; i < OverlaysCount; i++) {
		Overlay o;
		str->ReadSize(o.size);
		str->ReadResRef(o.TilesetResRef);
		str->ReadWord(o.UniqueTileCount);
		str->ReadWord(o.MovementType);
		str->ReadDword(o.TilemapOffset);
		str->ReadDword(o.TILOffset);
		overlays.push_back(o);
	}
	//Reading the Secondary Header
	str->Seek(SecHeaderOffset, GEM_STREAM_START);
	str->ReadDword(WallPolygonsCount);
	str->ReadDword(PolygonsOffset);
	str->ReadDword(VerticesOffset);
	str->ReadDword(WallGroupsOffset);
	str->ReadDword(PLTOffset);
	ExtendedNight = false;

	ReadWallPolygons();
	return true;
}

int WEDImporter::AddOverlay(TileMap* tm, const Overlay* newOverlays, bool rain) const
{
	int usedoverlays = 0;

	ResRef res = newOverlays->TilesetResRef;
	uint8_t len = res.length();
	// in BG1 extended night WEDs always reference the day TIS instead of the matching night TIS
	if (ExtendedNight && len == 6) {
		res[len] = 'N';
		if (!gamedata->Exists(res, IE_TIS_CLASS_ID)) {
			res[len] = '\0';
		} else {
			len++;
		}
	}
	if (rain && len < 8) {
		res[len] = 'R';
		//no rain tileset available, rolling back
		if (!gamedata->Exists(res, IE_TIS_CLASS_ID)) {
			res[len] = '\0';
		}
	}
	DataStream* tisfile = gamedata->GetResourceStream(res, IE_TIS_CLASS_ID);
	if (!tisfile) {
		return -1;
	}
	PluginHolder<TileSetMgr> tis = MakePluginHolder<TileSetMgr>(IE_TIS_CLASS_ID);
	tis->Open(tisfile);
	auto over = MakeHolder<TileOverlay>(newOverlays->size);
	for (int y = 0; y < newOverlays->size.h; y++) {
		for (int x = 0; x < newOverlays->size.w; x++) {
			str->Seek(newOverlays->TilemapOffset + (y * newOverlays->size.w + x) * 10, GEM_STREAM_START);

			ieWord startindex, count, secondary;
			ieByte overlaymask, animspeed;
			str->ReadWord(startindex);
			str->ReadWord(count);
			str->ReadWord(secondary);
			str->Read(&overlaymask, 1); // bFlags in the original
			str->Read(&animspeed, 1);
			// WORD    wFlags in the original (currently unused)
			if (animspeed == 0) {
				animspeed = ANI_DEFAULT_FRAMERATE;
			}
			str->Seek(newOverlays->TILOffset + startindex * 2, GEM_STREAM_START);
			std::vector<ieWord> indices(count);
			str->Read(indices.data(), count * sizeof(ieWord));

			Tile* tile;
			if (secondary == 0xffff) {
				tile = tis->GetTile(indices);
			} else {
				tile = tis->GetTile(indices, &secondary);
				tile->GetAnimation(1)->fps = animspeed;
			}
			tile->GetAnimation(0)->fps = animspeed;
			tile->om = overlaymask;
			usedoverlays |= overlaymask;
			over->AddTile(std::move(*tile));
			delete tile;
		}
	}

	if (rain) {
		tm->AddRainOverlay(std::move(over));
	} else {
		tm->AddOverlay(std::move(over));
	}
	return usedoverlays;
}

//this will replace the tileset of an existing tilemap, or create a new one
TileMap* WEDImporter::GetTileMap(TileMap* tm) const
{
	int usedoverlays;
	bool freenew = false;

	if (overlays.empty()) {
		return NULL;
	}

	if (!tm) {
		tm = new TileMap();
		freenew = true;
	}

	usedoverlays = AddOverlay(tm, &overlays.at(0), false);
	if (usedoverlays == -1) {
		if (freenew) {
			delete tm;
		}
		return NULL;
	}
	// rain_overlays[0] is never used
	tm->AddRainOverlay(NULL);

	//reading additional overlays
	int mask = 2;
	for (ieDword i = 1; i < OverlaysCount; i++) {
		//skipping unused overlays
		if (!(mask & usedoverlays)) {
			tm->AddOverlay(NULL);
			tm->AddRainOverlay(NULL);
		} else {
			// FIXME: should fix AddOverlay not to load an overlay twice if there's no rain version!!
			AddOverlay(tm, &overlays.at(i), false);
			AddOverlay(tm, &overlays.at(i), true);
		}
		mask <<= 1;
	}
	return tm;
}

void WEDImporter::GetDoorPolygonCount(ieWord count, ieDword offset)
{
	ieDword basecount = offset - PolygonsOffset;
	if (basecount % WED_POLYGON_SIZE) {
		basecount += WED_POLYGON_SIZE;
		Log(WARNING, "WEDImporter", "Found broken door polygon header!");
	}
	ieDword polycount = basecount / WED_POLYGON_SIZE + count - WallPolygonsCount;
	if (polycount > DoorPolygonsCount) {
		DoorPolygonsCount = polycount;
	}
}

WallPolygonGroup WEDImporter::ClosedDoorPolygons() const
{
	size_t index = (ClosedPolyOffset - PolygonsOffset) / WED_POLYGON_SIZE;
	size_t count = ClosedPolyCount;
	return MakeGroupFromTableEntries(index, count);
}

WallPolygonGroup WEDImporter::OpenDoorPolygons() const
{
	size_t index = (OpenPolyOffset - PolygonsOffset) / WED_POLYGON_SIZE;
	size_t count = OpenPolyCount;
	return MakeGroupFromTableEntries(index, count);
}

std::vector<ieWord> WEDImporter::GetDoorIndices(const ResRef& resref, bool& BaseClosed)
{
	ieWord DoorClosed, DoorTileStart, DoorTileCount;
	ResRef Name;
	unsigned int i;

	for (i = 0; i < DoorsCount; i++) {
		str->Seek(DoorsOffset + (i * 0x1A), GEM_STREAM_START);
		str->ReadResRef(Name);
		if (Name == resref)
			break;
	}
	//The door has no representation in the WED file
	if (i == DoorsCount) {
		Log(ERROR, "WEDImporter", "Found door without WED entry!");
		return {};
	}

	str->ReadWord(DoorClosed);
	str->ReadWord(DoorTileStart);
	str->ReadWord(DoorTileCount);
	str->ReadWord(OpenPolyCount);
	str->ReadWord(ClosedPolyCount);
	str->ReadDword(OpenPolyOffset);
	str->ReadDword(ClosedPolyOffset);

	//Reading Door Tile Cells
	str->Seek(DoorTilesOffset + (DoorTileStart * 2), GEM_STREAM_START);
	auto DoorTiles = std::vector<ieWord>(DoorTileCount);
	str->Read(DoorTiles.data(), DoorTileCount * sizeof(ieWord));

	BaseClosed = DoorClosed != 0;
	return DoorTiles;
}

void WEDImporter::ReadWallPolygons()
{
	for (ieDword i = 0; i < DoorsCount; i++) {
		constexpr uint8_t doorSize = 0x1A;
		constexpr uint8_t polyOffset = 14;
		str->Seek(DoorsOffset + polyOffset + (i * doorSize), GEM_STREAM_START);

		str->ReadWord(OpenPolyCount);
		str->ReadWord(ClosedPolyCount);
		str->ReadDword(OpenPolyOffset);
		str->ReadDword(ClosedPolyOffset);

		GetDoorPolygonCount(OpenPolyCount, OpenPolyOffset);
		GetDoorPolygonCount(ClosedPolyCount, ClosedPolyOffset);
	}

	ieDword polygonCount = WallPolygonsCount + DoorPolygonsCount;

	struct wed_polygon {
		ieDword FirstVertex;
		ieDword CountVertex;
		ieByte Flags;
		ieByte Height; // typically set to -1, unsure if used
		Region rect;
	};

	polygonTable.resize(polygonCount);
	wed_polygon* PolygonHeaders = new wed_polygon[polygonCount];

	str->Seek(PolygonsOffset, GEM_STREAM_START);

	for (ieDword i = 0; i < polygonCount; i++) {
		str->ReadDword(PolygonHeaders[i].FirstVertex);
		str->ReadDword(PolygonHeaders[i].CountVertex);
		str->Read(&PolygonHeaders[i].Flags, 1);
		str->Read(&PolygonHeaders[i].Height, 1);

		// Note: unlike the rest, the layout is minX, maxX, minY, maxY
		auto& rect = PolygonHeaders[i].rect;
		str->ReadScalar<int, ieWord>(rect.x);
		str->ReadScalar<int, ieWord>(rect.w);
		str->ReadScalar<int, ieWord>(rect.y);
		str->ReadScalar<int, ieWord>(rect.h);

		rect.w -= rect.x;
		rect.h -= rect.y;
	}

	for (ieDword i = 0; i < polygonCount; i++) {
		str->Seek(PolygonHeaders[i].FirstVertex * 4 + VerticesOffset, GEM_STREAM_START);
		//compose polygon
		ieDword count = PolygonHeaders[i].CountVertex;
		if (count < 3) {
			//danger, danger
			continue;
		}
		ieDword flags = PolygonHeaders[i].Flags & ~(WF_BASELINE | WF_HOVER);
		Point base0, base1;
		if (PolygonHeaders[i].Flags & WF_HOVER) {
			count -= 2;
			str->ReadPoint(base0);
			str->ReadPoint(base1);
			flags |= WF_BASELINE;
		}
		std::vector<Point> points(count);
		for (size_t j = 0; j < count; ++j) {
			Point vertex;
			str->ReadPoint(vertex);
			points[j] = vertex;
		}

		if (!(flags & WF_BASELINE)) {
			if (PolygonHeaders[i].Flags & WF_BASELINE) {
				base0 = points[0];
				base1 = points[1];
				flags |= WF_BASELINE;
			}
		}

		const Region& rgn = PolygonHeaders[i].rect;
		if (!rgn.size.IsInvalid()) { // PST AR0600 is known to have a polygon with 0 height
			polygonTable[i] = std::make_shared<WallPolygon>(std::move(points), &rgn);
			if (flags & WF_BASELINE) {
				polygonTable[i]->SetBaseline(base0, base1);
			}
			if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
				flags |= WF_COVERANIMS;
			}
			polygonTable[i]->SetPolygonFlag(flags);
		}
	}
	delete[] PolygonHeaders;
}

WallPolygonGroup WEDImporter::MakeGroupFromTableEntries(size_t idx, size_t cnt) const
{
	auto begin = polygonTable.begin() + idx;
	auto end = begin + cnt;
	WallPolygonGroup grp;
	std::copy_if(begin, end, std::back_inserter(grp), [](const std::shared_ptr<WallPolygon>& wp) {
		return wp != nullptr;
	});
	return grp;
}

std::vector<WallPolygonGroup> WEDImporter::GetWallGroups() const
{
	str->Seek(PLTOffset, GEM_STREAM_START);
	size_t PLTSize = (VerticesOffset > PLTOffset ? VerticesOffset - PLTOffset : PLTOffset - VerticesOffset) / 2;
	std::vector<ieWord> PLT(PLTSize);

	for (ieWord& idx : PLT) {
		str->ReadWord(idx);
	}

	auto ceilInt = [](int32_t v, int32_t div) {
		if (v % div == 0) {
			return v / div;
		}

		return (v + (div - (v % div))) / div;
	};

	// was error-prone w.r.t. IEEE754 optimization: ceilf(overlays[0].size.w / 10.0f) * ceilf(overlays[0].size.h / 7.5f)
	auto w = overlays[0].size.w;
	auto h = overlays[0].size.h * 2;
	size_t groupSize = ceilInt(w, 10) * ceilInt(h, 15);

	std::vector<WallPolygonGroup> polygonGroups;
	polygonGroups.reserve(groupSize);

	str->Seek(WallGroupsOffset, GEM_STREAM_START);
	for (size_t i = 0; i < groupSize; ++i) {
		ieWord index, count;
		str->ReadWord(index);
		str->ReadWord(count);

		polygonGroups.emplace_back();
		WallPolygonGroup& group = polygonGroups.back();

		for (ieWord j = index; j < index + count; ++j) {
			ieWord polyIndex = PLT[j];
			auto wp = polygonTable[polyIndex];
			if (wp) {
				group.push_back(wp);
			}
		}
	}

	return polygonGroups;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x7486BE7, "WED File Importer")
PLUGIN_CLASS(IE_WED_CLASS_ID, WEDImporter)
END_PLUGIN()
