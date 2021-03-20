/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2013 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This class handles VEF files of BG2/ToB it is closely related to VVC (ScriptedAnimation)

#include "VEFObject.h"

#define YESNO(x) ( (x)?"Yes":"No")

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "ScriptedAnimation.h"
#include "TableMgr.h"
#include "Video.h"
#include "System/DataStream.h"

namespace GemRB {

VEFObject::VEFObject()
{
	ResName[0]=0;
	SingleObject=false;
}

VEFObject::VEFObject(ScriptedAnimation *sca)
{
	Pos = sca->Pos;
	strnlwrcpy(ResName, sca->ResName, 8);
	SingleObject=true;
	ScheduleEntry entry;
	entry.start = core->GetGame()->GameTime;
	if (sca->Duration==0xffffffff) entry.length = 0xffffffff;
	else entry.length = sca->Duration+entry.start;
	entry.offset = Point(0,0);
	entry.type = VEF_VVC;
	entry.ptr = sca;
	memcpy(entry.resourceName, sca->ResName, sizeof(ieResRef) );
	entries.push_back(entry);
}

VEFObject::~VEFObject()
{
	Init();
}

void VEFObject::Init()
{
	for(auto& entry : entries) {
		if (!entry.ptr) continue;
		switch(entry.type) {
			case VEF_BAM:
			case VEF_VVC:
				delete (ScriptedAnimation *)entry.ptr;
				break;
			case VEF_VEF:
			case VEF_2DA:
				delete (VEFObject *)entry.ptr;
				break;
			default:; //error, no suitable destructor
		}
	}
}

void VEFObject::AddEntry(const ieResRef res, ieDword st, ieDword len, Point pos, ieDword type, ieDword gtime)
{
	ScheduleEntry entry;

	memcpy(entry.resourceName, res, sizeof(ieResRef) );
	entry.start = gtime+st;
	if (len!=0xffffffff) len+=entry.start;
	entry.length = len;
	entry.offset = pos;
	entry.type = type;
	entry.ptr = NULL;
	entries.push_back(entry);
}

ScriptedAnimation *VEFObject::CreateCell(const ieResRef res, ieDword start, ieDword end)
{
	ScriptedAnimation *sca = gamedata->GetScriptedAnimation( res, false);
	if (sca && end!=0xffffffff) {
		sca->SetDefaultDuration(AI_UPDATE_TIME*(end-start) );
	}
	return sca;
}

VEFObject *VEFObject::CreateObject(const ieResRef res, SClass_ID id)
{
	if (gamedata->Exists( res, id, true) ) {
		VEFObject *obj = new VEFObject();

		if (id==IE_2DA_CLASS_ID) {
			obj->Load2DA(res);
		} else {
			DataStream* stream = gamedata->GetResource(res, id);
			strnlwrcpy(obj->ResName, res, 8);
			obj->LoadVEF(stream);
		}
		return obj;
	}
	return NULL;
}

bool VEFObject::UpdateDrawingState(int orientation)
{
	drawQueue.clear();
	ieDword GameTime = core->GetGame()->GameTime;
	for (auto& entry : entries) {
		//don't render the animation if it is outside of the cycle
		if (entry.start > GameTime) continue;
		if (entry.length < GameTime) continue;

		if (!entry.ptr) {
			switch(entry.type) {
				case VEF_2DA: //original gemrb implementation of composite video effects
					entry.ptr = CreateObject(entry.resourceName, IE_2DA_CLASS_ID);
					if (entry.ptr) {
						break;
					}
					// fall back to VEF
					// intentional fallthrough
				case VEF_VEF: //vanilla engine implementation of composite video effects
					entry.ptr = CreateObject(entry.resourceName, IE_VEF_CLASS_ID);
					if (entry.ptr ) {
						break;
					}
					// fall back to BAM or VVC
					// intentional fallthrough
				case VEF_BAM: //just a BAM
				case VEF_VVC: //videocell (can contain a BAM)
					entry.ptr = CreateCell(entry.resourceName, entry.length, entry.start);
					break;
				default:;
			}
		}
		
		if (!entry.ptr) entry.type = VEF_INVALID;
				
		bool ended = true;
		switch(entry.type) {
		case VEF_BAM:
		case VEF_VVC:
			ended = ((ScriptedAnimation *) entry.ptr)->UpdateDrawingState(orientation);
			break;
		case VEF_2DA:
		case VEF_VEF:
			ended = ((VEFObject *) entry.ptr)->UpdateDrawingState(orientation);
			break;
		}
		
		if (ended) return true;
		
		drawQueue.push_back(entry);
	}
	return false;
}

void VEFObject::Draw(const Region &vp, const Color &p_tint, int height, uint32_t flags) const
{
	for (const auto& entry : drawQueue) {
		switch (entry.type) {
		case VEF_BAM:
		case VEF_VVC:
			((ScriptedAnimation *)entry.ptr)->Draw(vp, p_tint, height, flags);
			break;
		case VEF_2DA:
		case VEF_VEF:
			((VEFObject *)entry.ptr)->Draw(vp, p_tint, height, flags);
			break;
		}
	}
}

void VEFObject::Load2DA(const ieResRef resource)
{
	Init();
	AutoTable tab(resource);

	if (!tab) {
		return;
	}
	SingleObject = false;
	strnlwrcpy(ResName, resource, 8);
	ieDword GameTime = core->GetGame()->GameTime;
	int rows = tab->GetRowCount();
	while(rows--) {
		Point offset;
		int delay, duration;
		ieResRef resource;

		offset.x=atoi(tab->QueryField(rows,0));
		offset.y=atoi(tab->QueryField(rows,1));
		delay = atoi(tab->QueryField(rows,3));
		duration = atoi(tab->QueryField(rows,4));
		strnuprcpy(resource, tab->QueryField(rows,2), 8);
		AddEntry(resource, delay, duration, offset, VEF_VVC, GameTime);
	}
}

void VEFObject::ReadEntry(DataStream *stream)
{
	ieDword start;
	ieDword tmp;
	ieDword length;
	ieResRef resource;
	ieDword type;
	ieDword continuous;
	Point position;

	stream->ReadDword( &start);
	position.x = 0;
	position.y = 0;
	stream->ReadDword( &tmp); //unknown field (could be position?)
	stream->ReadDword( &length);
	stream->ReadDword( &type);
	stream->ReadResRef( resource);
	stream->ReadDword( &continuous);
	stream->Seek( 49*4, GEM_CURRENT_POS); //skip empty fields

	if (continuous) length = -1;
	ieDword GameTime = core->GetGame()->GameTime;
	AddEntry(resource, start, length, position, type, GameTime);
}

void VEFObject::LoadVEF(DataStream *stream)
{
	Init();
	if (!stream) {
		return;
	}
	ieDword i;
	ieResRef Signature;
	ieDword offset1, offset2;
	ieDword count1, count2;

	stream->ReadResRef( Signature);
	if (strncmp( Signature, "VEF V1.0", 8 ) != 0) {
		Log(ERROR, "VEFObject", "Not a valid VEF File: %s", ResName);
		delete stream;
		return;
	}
	SingleObject = false;
	stream->ReadDword( &offset1);
	stream->ReadDword( &count1);
	stream->ReadDword( &offset2);
	stream->ReadDword( &count2);

	stream->Seek(offset1, GEM_STREAM_START);
	for (i=0;i<count1;i++) {
		ReadEntry(stream);
	}

	stream->Seek(offset2, GEM_STREAM_START);
	for (i=0;i<count2;i++) {
		ReadEntry(stream);
	}
}

ScriptedAnimation *VEFObject::GetSingleObject() const
{
	ScriptedAnimation *sca = NULL;

	if (SingleObject) {
		if (entries.size()) {
			const ScheduleEntry& entry = entries[0];
			if (entry.type==VEF_VVC || entry.type==VEF_BAM) {
				sca = (ScriptedAnimation *)entry.ptr;
			}
		}
	}
	return sca;
}

}
