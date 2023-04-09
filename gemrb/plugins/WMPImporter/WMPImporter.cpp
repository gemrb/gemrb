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

#include "WMPImporter.h"

#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"

using namespace GemRB;

WMPImporter::~WMPImporter(void)
{
	delete str1;
	delete str2;
}

bool WMPImporter::Open(DataStream* stream1, DataStream* stream2)
{
	if ((stream1 == NULL) && (stream2 == NULL) ) {
		return false;
	}
	delete str1;
	delete str2;
	str1 = stream1;
	str2 = stream2;

	char Signature[8];

	if (str1) {
		str1->Read( Signature, 8 );
		if (strncmp( Signature, "WMAPV1.0", 8 ) != 0) {
			Log(ERROR, "WMPImporter", "'{}' is not a valid WMP File",
				stream1->filename);
			return false;
		}
		str1->ReadDword(WorldMapsCount1);
		str1->ReadDword(WorldMapsOffset1);
	} else {
		WorldMapsCount1 = 0;
		WorldMapsOffset1 = 0;
	}

	if (str2) {
		str2->Read( Signature, 8 );
		if (strncmp( Signature, "WMAPV1.0", 8 ) != 0) {
			Log(ERROR, "WMPImporter", "'{}' is not a valid WMP File",
				stream2->filename);
			return false;
		}
		str2->ReadDword(WorldMapsCount2);
		str2->ReadDword(WorldMapsOffset2);
	} else {
		WorldMapsCount2 = 0;
		WorldMapsOffset2 = 0;
	}

	WorldMapsCount = WorldMapsCount1 + WorldMapsCount2;
	return true;
}

WorldMapArray* WMPImporter::GetWorldMapArray() const
{
	assert(WorldMapsCount == WorldMapsCount1 + WorldMapsCount2);

	WorldMapArray* ma = new WorldMapArray(WorldMapsCount);
	for (unsigned int i = 0; i < WorldMapsCount1; i++) {
		WorldMap *m = ma->NewWorldMap( i );
		GetWorldMap( str1, m, i );
	}

	for (unsigned int i = 0; i < WorldMapsCount2; i++) {
		WorldMap *m = ma->NewWorldMap( i + WorldMapsCount1);
		GetWorldMap( str2, m, i );
	}
	return ma;
}

void WMPImporter::GetWorldMap(DataStream *str, WorldMap *m, unsigned int index) const
{
	unsigned int WorldMapsOffset;
	ieDword AreaEntriesCount, AreaEntriesOffset, AreaLinksCount, AreaLinksOffset;

	if (index && str==str2) {
		WorldMapsOffset=WorldMapsOffset2;
	}
	else {
		WorldMapsOffset=WorldMapsOffset1;
	}

	str->Seek( WorldMapsOffset + index * 184, GEM_STREAM_START );
	str->ReadResRef( m->MapResRef );
	str->ReadDword(m->Width);
	str->ReadDword(m->Height);
	str->ReadDword(m->MapNumber);
	str->ReadStrRef(m->AreaName);
	str->ReadDword(m->unknown1);
	str->ReadDword(m->unknown2);
	str->ReadDword(AreaEntriesCount);
	str->ReadDword(AreaEntriesOffset);
	str->ReadDword(AreaLinksOffset);
	str->ReadDword(AreaLinksCount);
	str->ReadResRef( m->MapIconResRef );
	str->ReadDword(m->Flags); // TODO: use; only present in EE, so make sure to ignore in other games, since it's not guaranteed to be 0

	// Load map bitmap
	ResourceHolder<ImageMgr> mos = gamedata->GetResourceHolder<ImageMgr>(m->MapResRef);
	if (!mos) {
		Log(ERROR, "WMPImporter", "Worldmap image not found.");
	} else {
		m->SetMapMOS(mos->GetSprite2D());
		if (!m->GetMapMOS()) {
			Log(ERROR, "WMPImporter", "Worldmap image malformed!");
		}
	}

	// Load location icon bam
	AnimationFactory* af = static_cast<AnimationFactory*>(gamedata->GetFactoryResource(m->MapIconResRef, IE_BAM_CLASS_ID));
	if (af) {
		m->SetMapIcons(af);
	}

	str->Seek( AreaEntriesOffset, GEM_STREAM_START );


	WMPAreaLink al;
	for (unsigned int i = 0; i < AreaEntriesCount; i++) {
		m->SetAreaEntry(i, GetAreaEntry(str));
	}

	str->Seek( AreaLinksOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < AreaLinksCount; i++) {
		m->SetAreaLink(i,GetAreaLink(str, &al));
	}

}

WMPAreaEntry WMPImporter::GetAreaEntry(DataStream *str) const
{
	WMPAreaEntry ae;

	str->ReadResRef(ae.AreaName);
	str->ReadResRef(ae.AreaResRef);
	str->ReadVariable(ae.AreaLongName);
	ieDword tmpDword;
	str->ReadDword(tmpDword);
	str->ReadDword(ae.IconSeq);
	//this should be set after iconseq is known
	ae.SetAreaStatus(tmpDword, BitOp::SET);
	ieDword coord;
	str->ReadDword(coord);
	ae.pos.x = coord;
	str->ReadDword(coord);
	ae.pos.y = coord;
	str->ReadStrRef(ae.LocCaptionName);
	str->ReadStrRef(ae.LocTooltipName);
	str->ReadResRef(ae.LoadScreenResRef);

	for (WMPDirection dir : EnumIterator<WMPDirection>()) {
		str->ReadDword(ae.AreaLinksIndex[dir]);
		str->ReadDword(ae.AreaLinksCount[dir]);
	}
	str->Seek( 128, GEM_CURRENT_POS );

	return ae;
}

WMPAreaLink* WMPImporter::GetAreaLink(DataStream *str, WMPAreaLink* al) const
{
	str->ReadDword(al->AreaIndex);
	str->ReadVariable(al->DestEntryPoint);
	str->ReadDword(al->DistanceScale);
	str->ReadDword(al->DirectionFlags);
	for (auto& ref : al->EncounterAreaResRef) {
		str->ReadResRef(ref);
	}
	str->ReadDword(al->EncounterChance);
	str->Seek( 128, GEM_CURRENT_POS );

	return al;
}

int WMPImporter::GetStoredFileSize(WorldMapArray *wmap, unsigned int index)
{
	assert(!index || !wmap->IsSingle());

	int headersize = 16;
	int WorldMapsOffset;

	WorldMapsCount = wmap->GetMapCount();
	if (index>WorldMapsCount || index>1) return 0;

	WorldMapsOffset = headersize;
	if (index) {
		WorldMapsCount2 = 0;
	} else {
		WorldMapsCount1 = 0;
	}

	for (unsigned int i=index;i<WorldMapsCount; i++) {
		if (index) {
			WorldMapsCount2++;
		} else {
			WorldMapsCount1++;
		}

		headersize += 184;
		const WorldMap *map = wmap->GetWorldMap(i);

		headersize += map->GetEntryCount() * 240;
		headersize += map->GetLinkCount() * 216;

		//put the first array into the first map
		//the rest into the second map if not single
		if (!wmap->IsSingle() && !index) {
			break;
		}
	}

	if (index) {
		WorldMapsOffset2 = WorldMapsOffset;
	}
	else {
		WorldMapsOffset1 = WorldMapsOffset;
	}
	return headersize;
}

int WMPImporter::PutWorldMap(DataStream *stream1, DataStream *stream2, WorldMapArray *wmap) const
{
	if (!stream1 || !wmap) {
		return -1;
	}

	stream1->Write( "WMAPV1.0", 8);
	stream1->WriteDword(WorldMapsCount1);
	stream1->WriteDword(WorldMapsOffset1);

	if (stream2 && !wmap->IsSingle()) {
		stream2->Write( "WMAPV1.0", 8);
		stream2->WriteDword(WorldMapsCount2);
		stream2->WriteDword(WorldMapsOffset2);
	}
	return PutMaps( stream1, stream2, wmap);
}

int WMPImporter::PutLinks(DataStream *stream, const WorldMap *wmap) const
{
	unsigned int cnt = wmap->GetLinkCount();
	for (unsigned i = 0; i < cnt; i++) {
		const WMPAreaLink *al = wmap->GetLink(i);

		stream->WriteDword(al->AreaIndex);
		stream->WriteVariableUC(al->DestEntryPoint);
		stream->WriteDword(al->DistanceScale);
		stream->WriteDword(al->DirectionFlags);
		for (const auto& ref : al->EncounterAreaResRef) {
			stream->WriteResRefUC(ref);
		}
		stream->WriteDword(al->EncounterChance);
		stream->WriteFilling(128);
	}
	return 0;
}

int WMPImporter::PutAreas(DataStream *stream, const WorldMap *wmap) const
{
	ieDword tmpDword;
	unsigned int cnt = wmap->GetEntryCount();

	for(unsigned i=0;i<cnt;i++) {
		const WMPAreaEntry *ae = wmap->GetEntry(i);

		stream->WriteResRefUC(ae->AreaName);
		stream->WriteResRefUC(ae->AreaResRef);
		stream->WriteVariableUC(ae->AreaLongName);
		tmpDword = ae->GetAreaStatus();
		stream->WriteDword(tmpDword);
		stream->WriteDword(ae->IconSeq);
		ieDword coord = ae->pos.x;
		stream->WriteDword(coord);
		coord = ae->pos.y;
		stream->WriteDword(coord);
		stream->WriteStrRef(ae->LocCaptionName);
		stream->WriteStrRef(ae->LocTooltipName);
		stream->WriteResRef( ae->LoadScreenResRef );

		for (WMPDirection dir : EnumIterator<WMPDirection>()) {
			stream->WriteDword(ae->AreaLinksIndex[dir]);
			stream->WriteDword(ae->AreaLinksCount[dir]);
		}
		stream->WriteFilling(128);
	}
	return 0;
}

int WMPImporter::PutMaps(DataStream *stream1, DataStream *stream2, const WorldMapArray *wmap) const
{
	int ret = PutMap(stream1, wmap, 0);
	if (ret) return ret;

	if (stream2 && !wmap->IsSingle() ) {
		ret = PutMap(stream2, wmap, 1);
	}
	return ret;
}

int WMPImporter::PutMap(DataStream *stream, const WorldMapArray *wmap, unsigned int index) const
{
	unsigned int WorldMapsOffset;
	unsigned int count;
	int ret;

	assert(!index || !wmap->IsSingle());

	if (index) {
		WorldMapsOffset = WorldMapsOffset2;
		count = WorldMapsCount2;
	} else {
		WorldMapsOffset = WorldMapsOffset1;
		count = WorldMapsCount1;
	}

	ieDword AreaEntriesOffset = WorldMapsOffset + count * 184;
	ieDword AreaLinksOffset = AreaEntriesOffset;
	for (unsigned int i = index; i < WorldMapsCount; i++) {
		const WorldMap *map = wmap->GetWorldMap(i);

		AreaLinksOffset += map->GetEntryCount() * 240;
		if (!wmap->IsSingle() && !index) {
			break;
		}
	}

	//map headers
	for (unsigned int i = index; i < WorldMapsCount; i++) {
		ieDword AreaEntriesCount, AreaLinksCount;

		const WorldMap *map = wmap->GetWorldMap(i);
		AreaLinksCount = map->GetLinkCount();
		AreaEntriesCount = map->GetEntryCount();

		stream->WriteResRef( map->MapResRef );
		stream->WriteDword(map->Width);
		stream->WriteDword(map->Height);
		stream->WriteDword(map->MapNumber);
		stream->WriteStrRef(map->AreaName);
		stream->WriteDword(map->unknown1);
		stream->WriteDword(map->unknown2);
		//???

		stream->WriteDword(AreaEntriesCount);
		stream->WriteDword(AreaEntriesOffset);
		stream->WriteDword(AreaLinksOffset);
		stream->WriteDword(AreaLinksCount);
		stream->WriteResRef( map->MapIconResRef );
		stream->WriteDword(map->Flags);
		AreaEntriesOffset += AreaEntriesCount * 240;
		AreaLinksOffset += AreaLinksCount * 216;

		stream->WriteFilling(124);

		if (!wmap->IsSingle() && !index) {
			break;
		}
	}

	//area entries
	for (unsigned int i = index; i < WorldMapsCount; i++) {
		const WorldMap *map = wmap->GetWorldMap(i);

		ret = PutAreas( stream, map);
		if (ret) {
			return ret;
		}
		if (!wmap->IsSingle() && !index) {
			break;
		}
	}

	//links
	for (unsigned int i = index; i < WorldMapsCount; i++) {
		const WorldMap *map = wmap->GetWorldMap(i);

		ret = PutLinks( stream, map);
		if (ret) {
			return ret;
		}
		if (!wmap->IsSingle() && !index) {
			break;
		}
	}
	return 0;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x77918C6, "WMP File Importer")
PLUGIN_CLASS(IE_WMP_CLASS_ID, WMPImporter)
END_PLUGIN()
