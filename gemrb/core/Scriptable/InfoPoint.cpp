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

#include "strrefs.h"
#include "win32def.h"

#include "Audio.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "Projectile.h"
#include "Spell.h"
#include "SpriteCover.h"
#include "TileMap.h"
#include "Video.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"

#include <cassert>
#include <cmath>

#define YESNO(x) ( (x)?"Yes":"No")

InfoPoint::InfoPoint(void)
	: Highlightable( ST_TRIGGER )
{
	Destination[0] = 0;
	EntranceName[0] = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	TrapDetected = 0;
	TrapLaunch.empty();
}

InfoPoint::~InfoPoint(void)
{
}

void InfoPoint::SetEnter(const char *resref)
{
	if (gamedata->Exists(resref, IE_WAV_CLASS_ID) ) {
		strnuprcpy(EnterWav, resref, 8);
	}
}

//checks if the actor may use this travel trigger
//bit 1 : can use
//bit 2 : whole team
int InfoPoint::CheckTravel(Actor *actor)
{
	if (Flags&TRAP_DEACTIVATED) return CT_CANTMOVE;
	if (!actor->InParty && (Flags&TRAVEL_NONPC) ) return CT_CANTMOVE;
	if (actor->InParty && (Flags&TRAVEL_PARTY) ) {
		if (core->HasFeature(GF_TEAM_MOVEMENT) || core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE) ) {
			return CT_WHOLE;
		}
		return CT_GO_CLOSER;
	}
	if(actor->IsSelected() ) {
		if(core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE|ENP_ONLYSELECT) ) {
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
// GF_REVERSE_DOOR is the closest game feature (exists only in PST, and about area objects)
bool InfoPoint::IsPortal() const
{
	if (Type!=ST_TRAVEL) return false;
	if (Cursor != IE_CURSOR_PORTAL) return false;
	return core->HasFeature(GF_REVERSE_DOOR);
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
		AddTrigger(TriggerEntry(trigger_entered, ID));
		return true;
	} else if (Highlightable::TriggerTrap(skill, ID)) {
		if (!Trapped) {
			Flags|=TRAP_DEACTIVATED;
		}
		// ok, so this is a pain. Entered() trigger checks Trapped,
		// so it needs to be kept set. how to do this right?
		Trapped = true;
		return true;
	}
	return false;
}

bool InfoPoint::Entered(Actor *actor)
{
	if (outline->PointIn( actor->Pos ) ) {
		goto check;
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
		return true;
	}

	if (actor->GetInternalFlag()&IF_INTRAP) {
		return false;
	}

	if (actor->InParty || (Flags&TRAP_NPC) ) {
		//no need to avoid a travel trigger

		//skill?
		if (TriggerTrap(0, actor->GetGlobalID()) ) {
			return true;
		}
	}
	return false;
}

void InfoPoint::DebugDump() const
{
	switch (Type) {
		case ST_TRIGGER:
			printf( "Debugdump of InfoPoint Region %s:\n", GetScriptName() );
			break;
		case ST_PROXIMITY:
			printf( "Debugdump of Trap Region %s:\n", GetScriptName() );
			break;
		case ST_TRAVEL:
			printf( "Debugdump of Travel Region %s:\n", GetScriptName() );
			break;
		default:
			printf( "Debugdump of Unsupported Region %s:\n", GetScriptName() );
			break;
	}
	printf( "Region Global ID: %d\n", GetGlobalID());
	printf( "Position: %d.%d\n", Pos.x, Pos.y);
	switch(Type) {
	case ST_TRAVEL:
		printf( "Destination Area: %s Entrance: %s\n", Destination, EntranceName);
		break;
	case ST_PROXIMITY:
		printf( "TrapDetected: %d, Trapped: %s\n", TrapDetected, YESNO(Trapped));
		printf( "Trap detection: %d%%, Trap removal: %d%%\n", TrapDetectionDiff,
			TrapRemovalDiff );
		break;
	case ST_TRIGGER:
		printf ( "InfoString: %s\n", overHeadText );
		break;
	default:;
	}
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	printf( "Script: %s, Key: %s, Dialog: %s\n", name, KeyResRef, Dialog );
	printf( "Active: %s\n", YESNO(InternalFlags&IF_ACTIVE));
}

