// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// This class handles VEF files of BG2/ToB it is closely related to VVC (ScriptedAnimation)

#include "VEFObject.h"

#include "RGBAColor.h"

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Orientation.h"
#include "ScriptedAnimation.h"
#include "TableMgr.h"

#include "Logging/Logging.h"
#include "Streams/DataStream.h"

namespace GemRB {

VEFObject::VEFObject(std::unique_ptr<ScriptedAnimation> sca)
	: Pos(sca->Pos), SingleObject(true)
{
	ResName = sca->ResName;
	ScheduleEntry entry;
	entry.start = core->GetGame()->GameTime;
	if (sca->Duration == 0xffffffff)
		entry.length = 0xffffffff;
	else
		entry.length = sca->Duration + entry.start;
	entry.offset = Point(0, 0);
	entry.type = VEFTypes::VVC;
	entry.resourceName = sca->ResName;
	entry.sptr = std::move(sca);
	entries.push_back(std::move(entry));
}

void VEFObject::Init()
{
	for (auto& entry : entries) {
		if (!entry.vptr && !entry.sptr) continue;
		switch (entry.type) {
			case VEFTypes::BAM:
			case VEFTypes::VVC:
				entry.sptr.reset();
				break;
			case VEFTypes::VEF:
			case VEFTypes::_2DA:
				entry.vptr.reset();
				break;
			default:; //error, no suitable destructor
		}
	}
}

void VEFObject::AddEntry(const ResRef& res, ieDword st, ieDword len, Point pos, VEFTypes type, ieDword gtime)
{
	ScheduleEntry entry;

	entry.resourceName = res;
	entry.start = gtime + st;
	if (len != 0xffffffff) len += entry.start;
	entry.length = len;
	entry.offset = pos;
	entry.type = type;
	entries.push_back(std::move(entry));
}

std::unique_ptr<ScriptedAnimation> VEFObject::CreateCell(const ResRef& res, ieDword start, ieDword end) const
{
	auto sca = gamedata->GetScriptedAnimation(res, false);
	if (sca && end != 0xffffffff) {
		sca->SetDefaultDuration(core->Time.defaultTicksPerSec * (end - start));
	}
	return sca;
}

std::unique_ptr<VEFObject> VEFObject::CreateObject(const ResRef& res, SClass_ID id) const
{
	if (gamedata->Exists(res, id, true)) {
		auto obj = std::make_unique<VEFObject>();

		if (id == IE_2DA_CLASS_ID) {
			obj->Load2DA(res);
		} else {
			DataStream* stream = gamedata->GetResourceStream(res, id);
			obj->ResName = res;
			obj->LoadVEF(stream);
		}
		return obj;
	}
	return nullptr;
}

void VEFObject::CreateObjectFromEntry(ScheduleEntry& entry) const
{
	switch (entry.type) {
		case VEFTypes::_2DA: // original gemrb implementation of composite video effects
			entry.vptr = CreateObject(entry.resourceName, IE_2DA_CLASS_ID);
			if (entry.vptr) {
				break;
			}
			// fall back to VEF
			// intentional fallthrough
		case VEFTypes::VEF: // vanilla engine implementation of composite video effects
			entry.vptr = CreateObject(entry.resourceName, IE_VEF_CLASS_ID);
			if (entry.vptr) {
				break;
			}
			// fall back to BAM or VVC
			// intentional fallthrough
		case VEFTypes::BAM: // just a BAM
		case VEFTypes::VVC: // videocell (can contain a BAM)
			entry.sptr = CreateCell(entry.resourceName, entry.length, entry.start);
			break;
		default:;
	}
}

bool VEFObject::UpdateDrawingState(int orientation)
{
	drawQueue.clear();
	ieDword GameTime = core->GetGame()->GameTime;
	for (auto& entry : entries) {
		//don't render the animation if it is outside of the cycle
		if (entry.start > GameTime) continue;
		if (entry.length < GameTime) continue;

		if (!entry.vptr && !entry.sptr) {
			CreateObjectFromEntry(entry);
		}

		if (!entry.vptr && !entry.sptr) entry.type = VEFTypes::INVALID;

		bool ended = true;
		switch (entry.type) {
			case VEFTypes::BAM:
			case VEFTypes::VVC:
				orient_t orient;
				orient = orientation == -1 ? entry.sptr->Orientation : ClampToOrientation(orientation);
				ended = entry.sptr->UpdateDrawingState(orient);
				break;
			case VEFTypes::_2DA:
			case VEFTypes::VEF:
				ended = entry.vptr->UpdateDrawingState(orientation);
				break;
			default:
				break;
		}

		if (ended) return true;

		drawQueue.push_back(&entry);
	}
	return false;
}

void VEFObject::Draw(const Region& vp, const Color& p_tint, int height, BlitFlags flags) const
{
	for (const auto& entry : drawQueue) {
		switch (entry->type) {
			case VEFTypes::BAM:
			case VEFTypes::VVC:
				entry->sptr->Draw(vp, p_tint, height, flags);
				break;
			case VEFTypes::_2DA:
			case VEFTypes::VEF:
				entry->vptr->Draw(vp, p_tint, height, flags);
				break;
			default:
				break;
		}
	}
}

void VEFObject::Load2DA(const ResRef& resource)
{
	Init();
	AutoTable tab = gamedata->LoadTable(resource);

	if (!tab) {
		return;
	}
	SingleObject = false;
	ResName = resource;
	ieDword GameTime = core->GetGame()->GameTime;
	TableMgr::index_t rows = tab->GetRowCount();
	while (rows--) {
		Point offset;
		int delay, duration;
		ResRef subResource;

		offset.x = tab->QueryFieldSigned<int>(rows, 0);
		offset.y = tab->QueryFieldSigned<int>(rows, 1);
		delay = tab->QueryFieldSigned<int>(rows, 3);
		duration = tab->QueryFieldSigned<int>(rows, 4);
		subResource = tab->QueryField(rows, 2);
		AddEntry(subResource, delay, duration, offset, VEFTypes::VVC, GameTime);
	}
}

void VEFObject::ReadEntry(DataStream* stream)
{
	ieDword start;
	ieDword tmp;
	ieDword length;
	ResRef resource;
	ieDword type;
	ieDword continuous;
	Point position;

	stream->ReadDword(start);
	position.x = 0;
	position.y = 0;
	stream->ReadDword(tmp); //unknown field (could be position?)
	stream->ReadDword(length);
	stream->ReadDword(type);
	stream->ReadResRef(resource);
	stream->ReadDword(continuous);
	stream->Seek(49 * 4, GEM_CURRENT_POS); //skip empty fields

	if (continuous) length = -1;
	ieDword GameTime = core->GetGame()->GameTime;
	AddEntry(resource, start, length, position, static_cast<VEFTypes>(type), GameTime);
}

void VEFObject::LoadVEF(DataStream* stream)
{
	Init();
	if (!stream) {
		return;
	}
	ieDword i;
	char Signature[8];
	ieDword offset1, offset2;
	ieDword count1, count2;

	stream->Read(Signature, 8);
	if (memcmp(Signature, "VEF V1.0", 8) != 0) {
		Log(ERROR, "VEFObject", "Not a valid VEF File: {}", ResName);
		delete stream;
		return;
	}
	SingleObject = false;
	stream->ReadDword(offset1);
	stream->ReadDword(count1);
	stream->ReadDword(offset2);
	stream->ReadDword(count2);

	stream->Seek(offset1, GEM_STREAM_START);
	for (i = 0; i < count1; i++) {
		ReadEntry(stream);
	}

	stream->Seek(offset2, GEM_STREAM_START);
	for (i = 0; i < count2; i++) {
		ReadEntry(stream);
	}
}

std::unique_ptr<ScriptedAnimation> VEFObject::GetSingleObjectUptr()
{
	std::unique_ptr<ScriptedAnimation> sca;
	if (SingleObject) {
		if (!entries.empty()) {
			ScheduleEntry& entry = entries[0];
			if (entry.type == VEFTypes::VVC || entry.type == VEFTypes::BAM) {
				sca = std::move(entry.sptr);
			}
		}
	}
	return sca;
}

ScriptedAnimation* VEFObject::GetSingleObject()
{
	if (!SingleObject || entries.empty()) {
		return nullptr;
	}

	const ScheduleEntry& entry = entries[0];
	if (entry.type == VEFTypes::VVC || entry.type == VEFTypes::BAM) {
		return entry.sptr.get();
	}
	return nullptr;
}

}
