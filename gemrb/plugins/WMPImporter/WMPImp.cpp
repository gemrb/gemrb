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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WMPImporter/WMPImp.cpp,v 1.2 2004/02/24 22:20:37 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "WMPImp.h"

WMPImp::WMPImp(void)
{
	str = NULL;
	autoFree = false;
}

WMPImp::~WMPImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool WMPImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "WMAPV1.0", 8 ) != 0) {
		printf( "[WMPImporter]: This file is not a valid WMP File\n" );
		return false;
	}
	str->Read( &WorldMapsCount, 4 );
	str->Read( &WorldMapsOffset, 4 );

	return true;
}

WorldMap* WMPImp::GetWorldMap(unsigned int index)
{
	WorldMap* m = new WorldMap();

	str->Seek( WorldMapsOffset + index * 184, GEM_STREAM_START );

	str->Read( m->MapResRef, 8 );
	str->Read( &m->Width, 4 );
	str->Read( &m->Height, 4 );
	str->Read( &m->MapNumber, 4 );
	str->Read( &m->AreaName, 4 );
	str->Read( &m->unknown1, 4 );
	str->Read( &m->unknown2, 4 );
	str->Read( &m->AreaEntriesCount, 4 );
	str->Read( &m->AreaEntriesOffset, 4 );
	str->Read( &m->AreaLinksOffset, 4 );
	str->Read( &m->AreaLinksCount, 4 );
	str->Read( m->MapIconResRef, 8 );
	str->Read( m->unknown3, 128 );

	str->Seek( m->AreaEntriesOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < m->AreaEntriesCount; i++) {
		WMPAreaEntry* ae = GetAreaEntry();
		m->area_entries.push_back( ae );
	}

	str->Seek( m->AreaLinksOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < m->AreaLinksCount; i++) {
		WMPAreaLink* al = GetAreaLink();
		m->area_links.push_back( al );
	}


	DataStream* mosfile = core->GetResourceMgr()->GetResource( m->MapResRef,
													IE_MOS_CLASS_ID );
	if (!core->IsAvailable( IE_MOS_CLASS_ID )) {
		printf( "[WMPImporter]: No MOS Importer Available.\n" );
		return NULL;
	}
	ImageMgr* mos = ( ImageMgr* ) core->GetInterface( IE_MOS_CLASS_ID );
	mos->Open( mosfile );

	m->MapMOS = mos->GetImage();


	return m;
}

WMPAreaEntry* WMPImp::GetAreaEntry()
{
	WMPAreaEntry* ae = new WMPAreaEntry();

	str->Read( ae->AreaName, 8 );
	str->Read( ae->AreaResRef, 8 );
	str->Read( ae->AreaLongName, 32 );
	str->Read( &ae->AreaStatus, 4 );
	str->Read( &ae->IconSeq, 4 );
	str->Read( &ae->X, 4 );
	str->Read( &ae->Y, 4 );
	str->Read( &ae->LocCaptionName, 4 );
	str->Read( &ae->LocTooltipName, 4 );
	str->Read( ae->LoadScreenResRef, 8 );

	for (unsigned int dir = 0; dir < 4; dir++) {
		str->Read( &ae->AreaLinksIndex[dir], 4 );
		str->Read( &ae->AreaLinksCount[dir], 4 );
	}
	str->Read( ae->unknown, 128 );

	return ae;
}


WMPAreaLink* WMPImp::GetAreaLink()
{
	WMPAreaLink* al = new WMPAreaLink();

	str->Read( &al->AreaIndex, 4 );
	str->Read( al->DestEntryPoint, 32 );
	str->Read( &al->DistanceScale, 4 );
	str->Read( &al->Flags, 4 );
	for (unsigned k = 0; k < 5; k++) {
		str->Read( al->EncounterAreaResRef[k], 8 );
	}
	str->Read( &al->EncounterChance, 4 );
	str->Read( al->unknown, 128 );

	return al;
}


unsigned int WMPImp::GetWorldMapsCount()
{
	return WorldMapsCount;
}

