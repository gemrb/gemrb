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

#include "win32def.h"

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
	XPos=0;
	YPos=0;
	ZPos=0;
	ResName[0]=0;
	SingleObject=false;
}

VEFObject::VEFObject(ScriptedAnimation *sca)
{
	XPos=sca->XPos;
	YPos=sca->YPos;
	ZPos=sca->ZPos; //sometimes this is not an actual ZPos - PST portals, don't use it for rendering?
	ResName[0]=0;
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
	std::list<ScheduleEntry>::iterator iter;

	for(iter=entries.begin();iter!=entries.end();iter++) {
		if (!(*iter).ptr) continue;
		switch((*iter).type) {
			case VEF_BAM:
			case VEF_VVC:
				delete (ScriptedAnimation *) (*iter).ptr;
				break;
			case VEF_VEF:
			case VEF_2DA:
				delete (VEFObject *) (*iter).ptr;
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

bool VEFObject::Draw(const Region &screen, Point &position, const Color &p_tint, Map *area, int dither, int orientation, int height)
{
	bool ret = true;

	if (!area) return true; //end immediately
	ieDword GameTime = core->GetGame()->GameTime;

	std::list<ScheduleEntry>::iterator iter;

	for(iter=entries.begin();iter!=entries.end();iter++) {
		//don't render the animation if it is outside of the cycle
		if ( (*iter).start>GameTime) continue;
		if ( (*iter).length<GameTime) continue;

		Point pos = ((*iter).offset);
		pos.x+=position.x;
		pos.y+=position.y;

		bool tmp;

		if (!(*iter).ptr) {
			switch((*iter).type) {
				case VEF_2DA: //original gemrb implementation of composite video effects
					(*iter).ptr = CreateObject( (*iter).resourceName, IE_2DA_CLASS_ID);
					if ( (*iter).ptr ) {
						break;
					}
					//fall back to VEF
				case VEF_VEF: //vanilla engine implementation of composite video effects
					(*iter).ptr = CreateObject( (*iter).resourceName, IE_VEF_CLASS_ID);
					if ( (*iter).ptr ) {
						break;
					}
					//fall back to BAM or VVC
				case VEF_BAM: //just a BAM
				case VEF_VVC: //videocell (can contain a BAM)
					(*iter).ptr = CreateCell( (*iter).resourceName, (*iter).length, (*iter).start);
					break;
				default:;
			}
		}

		void *ptr = (*iter).ptr;
		if (!ptr) (*iter).type = VEF_INVALID;

		switch((*iter).type) {
		case VEF_BAM:
		case VEF_VVC:
			tmp = ((ScriptedAnimation *) (*iter).ptr)->Draw(screen, pos, p_tint, area, dither, orientation, height);
			break;
		case VEF_2DA:
		case VEF_VEF:
			tmp = ((VEFObject *) (*iter).ptr)->Draw(screen, pos, p_tint, area, dither, orientation, height);
			break;
		default:
			tmp = true; //unknown/invalid type
		}
		if (tmp) {
			(*iter).length = 0; //stop playing this if reached end
		}
		ret &= tmp;
	}
	return ret;
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

ScriptedAnimation *VEFObject::GetSingleObject()
{
	ScriptedAnimation *sca = NULL;

	if (SingleObject) {
		std::list<ScheduleEntry>::iterator iter = entries.begin();
		if (iter!=entries.end() ) {
			if ( (*iter).type==VEF_VVC || (*iter).type==VEF_BAM ) {
				sca = (ScriptedAnimation *) (*iter).ptr;
			}
		}
	}
	return sca;
}

}
