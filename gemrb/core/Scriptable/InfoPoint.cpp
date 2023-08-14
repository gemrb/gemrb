/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "Scriptable/InfoPoint.h"

#include "voodooconst.h"
#include "strrefs.h"
#include "ie_cursors.h"

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "TileMap.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"
#include "GUI/TextSystem/TextContainer.h"

namespace GemRB {

#define YESNO(x) ( (x)?"Yes":"No")

//crazy programmers couldn't decide which bit marks the alternative point
static ieDword TRAP_USEPOINT = _TRAP_USEPOINT;
static bool inited = false;

InfoPoint::InfoPoint(void)
	: Highlightable( ST_TRIGGER )
{
	if (!inited) {
		inited = true;
		//TRAP_USEPOINT may have three values
		//0     - PST - no such flag
		//0x200 - IWD2 - it has no TRAP_NONPC flag, the usepoint flag takes it over
		//0x400 - all other engines (some don't use it)
		if (core->HasFeature(GFFlags::USEPOINT_400)) TRAP_USEPOINT = _TRAP_USEPOINT;
		else if (core->HasFeature(GFFlags::USEPOINT_200)) TRAP_USEPOINT = _TRAVEL_NONPC;
		else TRAP_USEPOINT = 0;
	}
}

void InfoPoint::SetEnter(const ResRef& resref)
{
	if (gamedata->Exists(resref, IE_WAV_CLASS_ID) ) {
		EnterWav = resref;
	}
}

ieDword InfoPoint::GetUsePoint() const {
	return Flags&TRAP_USEPOINT;
}

//checks if the actor may use this travel trigger
//bit 1 : can use
//bit 2 : whole team
int InfoPoint::CheckTravel(const Actor *actor) const
{
	if (Flags&TRAP_DEACTIVATED) return CT_CANTMOVE;
	bool pm = actor->IsPartyMember();

	//this flag does not exist in IWD2, we cannot use it if the infopoint flag took it over
	if (TRAP_USEPOINT!=_TRAVEL_NONPC) {
		if (!pm && (Flags&_TRAVEL_NONPC) ) return CT_CANTMOVE;
	}

	// pst doesn't care about distance, selection or the party bit at all
	static const bool teamMove = core->HasFeature(GFFlags::TEAM_MOVEMENT);
	if (pm && ((Flags&TRAVEL_PARTY) || teamMove)) {
		if (teamMove || core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE) ) {
			return CT_WHOLE;
		}
		return CT_GO_CLOSER;
	}
	if (actor->IsSelected() ) {
		if (core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE|ENP_ONLYSELECT) ) {
			return CT_MOVE_SELECTED;
		}
		return CT_SELECTED;
	}
	return CT_ACTIVE;
}


bool InfoPoint::PossibleToSeeTrap() const
{
	// Only detectable trap-type infopoints.
	return (CanDetectTrap() && (Type == ST_PROXIMITY) );
}

bool InfoPoint::CanDetectTrap() const
{
	// Traps can be detected on all types of infopoint, as long
	// as the trap is detectable and isn't deactivated.
	return ((Flags&TRAP_DETECTABLE) && !(Flags&TRAP_DEACTIVATED));
}

// returns true if the infopoint is a PS:T portal
// GFFlags::REVERSE_DOOR is the closest game feature (exists only in PST, and about area objects)
bool InfoPoint::IsPortal() const
{
	if (Type!=ST_TRAVEL) return false;
	if (Cursor != IE_CURSOR_PORTAL) return false;
	return core->HasFeature(GFFlags::REVERSE_DOOR);
}

// returns the appropriate cursor over an active region (trap, infopoint, travel region)
int InfoPoint::GetCursor(int targetMode) const
{
	if (targetMode == TARGET_MODE_PICK) {
		if (VisibleTrap(0)) {
			return IE_CURSOR_TRAP;
		}
		return IE_CURSOR_STEALTH | IE_CURSOR_GRAY;
	}

	// traps always display a walk cursor
	if (Type == ST_PROXIMITY) {
		return IE_CURSOR_WALK;
	}
	return Cursor;
}

//trap that is visible on screen (marked by red)
//if TrapDetected is a bitflag, we could show traps selectively for
//players, really nice for multiplayer
bool Highlightable::VisibleTrap(int see_all) const
{
	if (!Trapped) return false;
	if (!PossibleToSeeTrap()) return false;
	if (!Scripts[0]) return false;
	if (see_all) return true;
	if (TrapDetected ) return true;
	return false;
}

//trap that will fire now
bool InfoPoint::TriggerTrap(int skill, ieDword ID)
{
	if (Type!=ST_PROXIMITY) {
		return true;
	}
	if (Flags&TRAP_DEACTIVATED) {
		return false;
	}
	if (!Trapped) {
		// we have to set Entered somewhere, here seems best..
		// FIXME: likely not best :)
		// NOTE: while only pst has this trigger, sending the normal trigger_entered in other
		// games breaks them. See git log -p -Strigger_entered for some previous attempts (already 3)
		AddTrigger(TriggerEntry(trigger_harmlessentered, ID));
		return true;
	} else if (Highlightable::TriggerTrap(skill, ID)) {
		return true;
	}
	return false;
}

bool InfoPoint::Entered(Actor *actor)
{
	if (outline) {
		// be more lenient for travel regions, fixed iwd2 ar1100 to1101 region
		if (Type == ST_TRAVEL && outline->BBox.PointInside(actor->Pos)) goto check;
		if (outline->PointIn( actor->Pos)) goto check;
	} else if (!BBox.size.IsInvalid()) {
		if (BBox.PointInside(actor->Pos)) goto check;
	} else {
		// this is to trap possible bugs in our understanding of ARE polygons that arent actually polygons
		assert(Type == ST_TRAVEL || Flags&TRAP_USEPOINT);
	}

	// why is this here? actors which aren't *in* a trap get IF_INTRAP
	// repeatedly unset, so this triggers again and again and again.
	// i disabled it for ST_PROXIMITY for now..
	/*if (Type != ST_PROXIMITY && (PersonalDistance(Pos, actor)<MAX_OPERATING_DISTANCE) ) {
		goto check;
	}*/
	// this method is better (fuzzie, 2009) and also works for the iwd ar6002 northeast exit
	if (Type == ST_TRAVEL && PersonalDistance(TrapLaunch, actor)<MAX_OPERATING_DISTANCE) {
		goto check;
	}
	// fuzzie can't escape pst's ar1405 without this one, maybe we should really be checking
	// for distance from the outline for travel regions instead?
	if (Type == ST_TRAVEL && PersonalDistance(TalkPos, actor)<MAX_OPERATING_DISTANCE) {
		goto check;
	}
	if (Flags&TRAP_USEPOINT) {
		if (PersonalDistance(UsePoint, actor)<MAX_OPERATING_DISTANCE) {
			goto check;
		}
	}
	return false;
check:
	if (Type==ST_TRAVEL) {
		actor->objects.LastMarked = GetGlobalID();
		return true;
	}

	if (actor->GetInternalFlag()&IF_INTRAP) {
		return false;
	}

	// allow entering trap when trying to disarm
	if (Type == ST_PROXIMITY && actor->GetDisarmingTrap() == GetGlobalID()) {
		return false;
	}

	// recheck ar1404 mirror trap Shadow1 still works if you modify TRAP_NPC logic
	if ((Flags&TRAP_NPC) ^ (!!actor->InParty)) {
		//no need to avoid a travel trigger

		//skill?
		if (TriggerTrap(0, actor->GetGlobalID()) ) {
			actor->objects.LastMarked = GetGlobalID();
			return true;
		}
	}
	return false;
}

std::string InfoPoint::dump() const
{
	std::string buffer;
	switch (Type) {
		case ST_TRIGGER:
			AppendFormat(buffer, "Debugdump of InfoPoint Region {}:\n", GetScriptName());
			break;
		case ST_PROXIMITY:
			AppendFormat(buffer, "Debugdump of Trap Region {}:\n", GetScriptName());
			break;
		case ST_TRAVEL:
			AppendFormat(buffer, "Debugdump of Travel Region {}:\n", GetScriptName());
			break;
		default:
			AppendFormat(buffer, "Debugdump of Unsupported Region {}:\n", GetScriptName());
			break;
	}
	AppendFormat(buffer, "Region Global ID: {}\n", GetGlobalID());
	AppendFormat(buffer, "Position: {}\n", Pos);
	AppendFormat(buffer, "TalkPos: {}\n", TalkPos);
	AppendFormat(buffer, "UsePoint: {}  (on: {})\n", UsePoint, YESNO(GetUsePoint()));
	AppendFormat(buffer, "TrapLaunch: {}\n", TrapLaunch);
	switch(Type) {
	case ST_TRAVEL:
		AppendFormat(buffer, "Destination Area: {} Entrance: {}\n", Destination, EntranceName);
		break;
	case ST_PROXIMITY:
		AppendFormat(buffer, "TrapDetected: {}, Trapped: {}\n", TrapDetected, YESNO(Trapped));
		AppendFormat(buffer, "Trap detection: {}%, Trap removal: {}%\n", TrapDetectionDiff, TrapRemovalDiff);
		break;
	case ST_TRIGGER:
			AppendFormat(buffer, "InfoString: {}\n", fmt::WideToChar{overHead.GetText()});
		break;
	default:;
	}
	ResRef name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	AppendFormat(buffer, "Script: {}, Key: {}, Dialog: {}\n", name, KeyResRef, Dialog);
	AppendFormat(buffer, "Deactivated: {}\n", YESNO(Flags&TRAP_DEACTIVATED));
	AppendFormat(buffer, "Active: {}\n", YESNO(InternalFlags&IF_ACTIVE));
	Log(DEBUG, "InfoPoint", "{}", buffer);
	return buffer;
}


}
