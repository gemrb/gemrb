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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WMPImporter/WMPImp.cpp,v 1.11 2005/02/20 13:00:54 avenger_teambg Exp $
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
	str->ReadDword( &WorldMapsCount );
	str->ReadDword( &WorldMapsOffset );

	return true;
}

WorldMap* WMPImp::GetWorldMap(unsigned int index)
{
	unsigned int i;

	str->Seek( WorldMapsOffset + index * 184, GEM_STREAM_START );

	WorldMap* m = core->NewWorldMap();
	str->ReadResRef( m->MapResRef );
	str->ReadDword( &m->Width );
	str->ReadDword( &m->Height );
	str->ReadDword( &m->MapNumber );
	str->ReadDword( &m->AreaName );
	str->ReadDword( &m->unknown1 );
	str->ReadDword( &m->unknown2 );
	str->ReadDword( &m->AreaEntriesCount );
	str->ReadDword( &m->AreaEntriesOffset );
	str->ReadDword( &m->AreaLinksOffset );
	str->ReadDword( &m->AreaLinksCount );
	str->ReadResRef( m->MapIconResRef );

	str->Seek( m->AreaEntriesOffset, GEM_STREAM_START );

	WMPAreaLink al;
	WMPAreaEntry ae;
	for (i = 0; i < m->AreaEntriesCount; i++) {
		m->SetAreaEntry(i,GetAreaEntry(&ae));
	}

	str->Seek( m->AreaLinksOffset, GEM_STREAM_START );
	for (i = 0; i < m->AreaLinksCount; i++) {
		m->SetAreaLink(i,GetAreaLink(&al));
	}

	// Load map bitmap
	if (!core->IsAvailable( IE_MOS_CLASS_ID )) {
		printf( "[WMPImporter]: No MOS Importer Available.\n" );
		return m;
	}
	ImageMgr* mos = ( ImageMgr* ) core->GetInterface( IE_MOS_CLASS_ID );
	DataStream* mosfile = core->GetResourceMgr()->GetResource( m->MapResRef, IE_MOS_CLASS_ID );
	mos->Open( mosfile, true ); //autofree
	m->MapMOS = mos->GetImage();
	core->FreeInterface( mos );

	// Load location icon
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "[WMPImporter]: No BAM Importer Available.\n" );
		return m;
	}
	AnimationMgr* icon = ( AnimationMgr* ) core->GetInterface( IE_BAM_CLASS_ID );
	DataStream* iconfile = core->GetResourceMgr()->GetResource( m->MapIconResRef, IE_BAM_CLASS_ID );
	icon->Open( iconfile, true ); //autofree

	std::vector< WMPAreaEntry*>::iterator ei;
	for (ei = m->area_entries.begin(); ei != m->area_entries.end(); ++ei) {
		(*ei)->MapIcon = icon->GetFrameFromCycle( (*ei)->IconSeq, 0 );
	}
	core->FreeInterface( icon );

	return m;
}

WMPAreaEntry* WMPImp::GetAreaEntry(WMPAreaEntry* ae)
{
	str->ReadResRef( ae->AreaName );
	str->ReadResRef( ae->AreaResRef );
	str->Read( ae->AreaLongName, 32 );
	str->ReadDword( &ae->AreaStatus );
	str->ReadDword( &ae->IconSeq );
	str->ReadDword( &ae->X );
	str->ReadDword( &ae->Y );
	str->ReadDword( &ae->LocCaptionName );
	str->ReadDword( &ae->LocTooltipName );
	str->ReadResRef( ae->LoadScreenResRef );

	for (unsigned int dir = 0; dir < 4; dir++) {
		str->ReadDword( &ae->AreaLinksIndex[dir] );
		str->ReadDword( &ae->AreaLinksCount[dir] );
	}
	str->Seek( 128, GEM_CURRENT_POS );

	return ae;
}

WMPAreaLink* WMPImp::GetAreaLink(WMPAreaLink* al)
{
	str->ReadDword( &al->AreaIndex );
	str->Read( al->DestEntryPoint, 32 );
	str->ReadDword( &al->DistanceScale );
	str->ReadDword( &al->Flags );
	for (unsigned k = 0; k < 5; k++) {
		str->ReadResRef( al->EncounterAreaResRef[k] );
	}
	str->ReadDword( &al->EncounterChance );
	str->Seek( 128, GEM_CURRENT_POS );

	return al;
}

unsigned int WMPImp::GetWorldMapsCount()
{
	return WorldMapsCount;
}

