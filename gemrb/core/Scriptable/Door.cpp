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

#include "Scriptable/Door.h"

#include "strrefs.h"

#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Projectile.h"
#include "TileMap.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"
#include "Scriptable/InfoPoint.h"
#include "System/StringBuffer.h"

namespace GemRB {

#define YESNO(x) ( (x)?"Yes":"No")

DoorTrigger::DoorTrigger(std::shared_ptr<Gem_Polygon> openTrigger, WallPolygonGroup&& openWalls,
			std::shared_ptr<Gem_Polygon> closedTrigger, WallPolygonGroup&& closedWalls)
: openWalls(std::move(openWalls)), closedWalls(std::move(closedWalls)),
openTrigger(std::move(openTrigger)), closedTrigger(std::move(closedTrigger))
{}

void DoorTrigger::SetState(bool open)
{
	isOpen = open;
	for (auto& wp : openWalls) {
		wp->SetDisabled(!isOpen);
	}
	for (auto& wp : closedWalls) {
		wp->SetDisabled(isOpen);
	}
}

std::shared_ptr<Gem_Polygon> DoorTrigger::StatePolygon() const
{
	return StatePolygon(isOpen);
}

std::shared_ptr<Gem_Polygon> DoorTrigger::StatePolygon(bool open) const
{
	return open ? openTrigger : closedTrigger;
}

Door::Door(TileOverlay* Overlay, DoorTrigger&& trigger)
	: Highlightable( ST_DOOR ), doorTrigger(std::move(trigger))
{
	tiles = NULL;
	tilecount = 0;
	Flags = 0;
	open_ib = NULL;
	oibcount = 0;
	closed_ib = NULL;
	cibcount = 0;
	OpenSound[0] = 0;
	CloseSound[0] = 0;
	LockSound[0] = 0;
	UnLockSound[0] = 0;
	overlay = Overlay;
	LinkedInfo[0] = 0;
	OpenStrRef = (ieDword) -1;
	closedIndex = NameStrRef = hp = ac = 0;
	DiscoveryDiff = LockDifficulty = 0;
}

Door::~Door(void)
{
	if (tiles) {
		free( tiles );
	}
	delete[] open_ib;
	delete[] closed_ib;
}

void Door::ImpedeBlocks(int count, Point *points, PathMapFlags value) const
{
	for(int i = 0;i<count;i++) {
		PathMapFlags tmp = area->GetInternalSearchMap(points[i].x, points[i].y) & PathMapFlags::NOTDOOR;
		area->SetInternalSearchMap(points[i].x, points[i].y, tmp|value);
	}
}

void Door::UpdateDoor()
{
	doorTrigger.SetState(Flags&DOOR_OPEN);
	outline = doorTrigger.StatePolygon();

	if (outline) {
		// update the Scriptable position
		Pos.x = outline->BBox.x + outline->BBox.w/2;
		Pos.y = outline->BBox.y + outline->BBox.h/2;
	}

	PathMapFlags pmdflags;

	if (Flags & DOOR_TRANSPARENT) {
		pmdflags = PathMapFlags::DOOR_IMPASSABLE;
	} else {
		//both door flags are needed here, one for transparency the other
		//is for passability
		pmdflags = PathMapFlags::DOOR_OPAQUE|PathMapFlags::DOOR_IMPASSABLE;
	}
	if (Flags &DOOR_OPEN) {
		ImpedeBlocks(cibcount, closed_ib, PathMapFlags::IMPASSABLE);
		ImpedeBlocks(oibcount, open_ib, pmdflags);
	}
	else {
		ImpedeBlocks(oibcount, open_ib, PathMapFlags::IMPASSABLE);
		ImpedeBlocks(cibcount, closed_ib, pmdflags);
	}

	InfoPoint *ip = area->TMap->GetInfoPoint(LinkedInfo);
	if (ip) {
		if (Flags&DOOR_OPEN) ip->Flags&=~INFO_DOOR;
		else ip->Flags|=INFO_DOOR;
	}
}

void Door::ToggleTiles(int State, int playsound)
{
	int i;
	int state;

	if (State) {
		state = !closedIndex;
		if (playsound && ( OpenSound[0] != '\0' ))
			core->GetAudioDrv()->Play(OpenSound, SFX_CHAN_ACTIONS);
	} else {
		state = closedIndex;
		if (playsound && ( CloseSound[0] != '\0' ))
			core->GetAudioDrv()->Play(CloseSound, SFX_CHAN_ACTIONS);
	}
	for (i = 0; i < tilecount; i++) {
		overlay->tiles[tiles[i]]->tileIndex = (ieByte) state;
	}

	//set door_open as state
	Flags = (Flags & ~DOOR_OPEN) | (State == !core->HasFeature(GF_REVERSE_DOOR) );
}

//this is the short name (not the scripting name)
void Door::SetName(const char* name)
{
	strnlwrcpy( ID, name, 8 );
}

void Door::SetTiles(unsigned short* Tiles, int cnt)
{
	if (tiles) {
		free( tiles );
	}
	tiles = Tiles;
	tilecount = cnt;
}

bool Door::CanDetectTrap() const
{
    // Traps can be detected on all types of infopoint, as long
    // as the trap is detectable and isn't deactivated.
    return ((Flags&DOOR_DETECTABLE) && Trapped);
}

void Door::SetDoorLocked(int Locked, int playsound)
{
	if (Locked) {
		if (Flags & DOOR_LOCKED) return;
		Flags|=DOOR_LOCKED;
		// only close it in pst, needed for Dead nations (see 4a3e1cb4ef)
		if (core->HasFeature(GF_REVERSE_DOOR)) SetDoorOpen(false, playsound, 0);
		if (playsound && ( LockSound[0] != '\0' ))
			core->GetAudioDrv()->Play(LockSound, SFX_CHAN_ACTIONS);
	}
	else {
		if (!(Flags & DOOR_LOCKED)) return;
		Flags&=~DOOR_LOCKED;
		if (playsound && ( UnLockSound[0] != '\0' ))
			core->GetAudioDrv()->Play(UnLockSound, SFX_CHAN_ACTIONS);
	}
}

int Door::IsOpen() const
{
	int ret = core->HasFeature(GF_REVERSE_DOOR);
	if (Flags&DOOR_OPEN) {
		ret = !ret;
	}
	return ret;
}

bool Door::HitTest(const Point& p) const
{
	if (Flags&DOOR_HIDDEN) {
		return false;
	}

	auto doorpoly = doorTrigger.StatePolygon();
	if (doorpoly) {
		if (!doorpoly->PointIn(p)) return false;
	} else if (Flags&DOOR_OPEN) {
		if (!OpenBBox.PointInside(p)) return false;
	} else {
		if (!ClosedBBox.PointInside(p)) return false;
	}

	return true;
}

std::shared_ptr<Gem_Polygon> Door::OpenTriggerArea() const
{
	return doorTrigger.StatePolygon(true);
}

std::shared_ptr<Gem_Polygon> Door::ClosedTriggerArea() const
{
	return doorTrigger.StatePolygon(false);
}

//also mark actors to fix position
bool Door::BlockedOpen(int Open, int ForceOpen) const
{
	bool blocked;
	int count;
	Point *points;

	blocked = false;
	if (Open) {
		count = oibcount;
		points = open_ib;
	} else {
		count = cibcount;
		points = closed_ib;
	}
	//getting all impeded actors flagged for jump
	Region rgn;
	rgn.w = 16;
	rgn.h = 12;
	for(int i = 0;i<count;i++) {
		Actor** ab;
		rgn.x = points[i].x*16;
		rgn.y = points[i].y*12;
		PathMapFlags tmp = area->GetInternalSearchMap(points[i].x, points[i].y) & PathMapFlags::ACTOR;
		if (tmp != PathMapFlags::IMPASSABLE) {
			int ac = area->GetActorsInRect(ab, rgn, GA_NO_DEAD|GA_NO_UNSCHEDULED);
			while(ac--) {
				if (ab[ac]->GetBase(IE_DONOTJUMP)) {
					continue;
				}
				ab[ac]->SetBase(IE_DONOTJUMP, DNJ_JUMP);
				blocked = true;
			}
			if (ab) {
				free(ab);
			}
		}
	}

	if ((Flags&DOOR_SLIDE) || ForceOpen) {
		return false;
	}
	return blocked;
}

void Door::SetDoorOpen(int Open, int playsound, ieDword ID, bool addTrigger)
{
	if (playsound) {
		//the door cannot be blocked when opening,
		//but the actors will be pushed
		//BlockedOpen will mark actors to be pushed
		if (BlockedOpen(Open,0) && !Open) {
			//clear up the blocking actors
			area->JumpActors(false);
			return;
		}
		area->JumpActors(true);
	}
	if (Open) {
		if (addTrigger) {
			if (Trapped) {
				AddTrigger(TriggerEntry(trigger_opened, ID));
			} else {
				AddTrigger(TriggerEntry(trigger_harmlessopened, ID));
			}
		}

		// in PS:T, opening a door does not unlock it
		if (!core->HasFeature(GF_REVERSE_DOOR)) {
			SetDoorLocked(false,playsound);
		}
	} else if (addTrigger) {
		if (Trapped) {
			AddTrigger(TriggerEntry(trigger_closed, ID));
		} else {
			AddTrigger(TriggerEntry(trigger_harmlessclosed, ID));
		}
	}
	ToggleTiles(Open, playsound);
	//synchronising other data with the door state
	UpdateDoor();

	core->SetEventFlag(EF_TARGETMODE);
}

bool Door::TryUnlock(Actor *actor) {
	if (!(Flags&DOOR_LOCKED)) return true;

	// don't remove key in PS:T!
	bool removekey = !core->HasFeature(GF_REVERSE_DOOR) && Flags&DOOR_KEY;
	return Highlightable::TryUnlock(actor, removekey);
}

void Door::TryDetectSecret(int skill, ieDword actorID)
{
	if (Type != ST_DOOR) return;
	if (Visible()) return;
	if (skill > (signed)DiscoveryDiff) {
		Flags |= DOOR_FOUND;
		core->PlaySound(DS_FOUNDSECRET, SFX_CHAN_HITS);
		AddTrigger(TriggerEntry(trigger_detected, actorID));
	}
}

// return true if the door isn't secret or if it is, but was already discovered
bool Door::Visible() const
{
	return (!(Flags & DOOR_SECRET) || (Flags & DOOR_FOUND)) && !(Flags & DOOR_HIDDEN);
}

void Door::SetNewOverlay(TileOverlay *Overlay) {
	overlay = Overlay;
	ToggleTiles(IsOpen(), false);
}

void Highlightable::SetTrapDetected(int x)
{
	if(x == TrapDetected)
		return;
	TrapDetected = x;
	if(TrapDetected) {
		core->PlaySound(DS_FOUNDSECRET, SFX_CHAN_HITS);
		core->Autopause(AP_TRAP, this);
	}
}

void Highlightable::TryDisarm(const Actor *actor)
{
	if (!Trapped || !TrapDetected) return;

	int skill = actor->GetStat(IE_TRAPS);
	int roll = 0;
	int bonus = 0;
	int trapDC = TrapRemovalDiff;

	if (core->HasFeature(GF_3ED_RULES)) {
		skill = actor->GetSkill(IE_TRAPS);
		roll = core->Roll(1, 20, 0);
		bonus = actor->GetAbilityBonus(IE_INT);
		trapDC = TrapRemovalDiff/7 + 10; // oddity from the original
		if (skill == 0) { // a trained skill
			trapDC = 100;
		}
	} else {
		roll = core->Roll(1, skill/2, 0);
		skill /= 2;
	}

	int check = skill + roll + bonus;
	if (check > trapDC) {
		AddTrigger(TriggerEntry(trigger_disarmed, actor->GetGlobalID()));
		//trap removed
		Trapped = 0;
		if (core->HasFeature(GF_3ED_RULES)) {
			// ~Successful Disarm Device - d20 roll %d + Disarm Device skill %d + INT mod %d >= Trap DC %d~
			displaymsg->DisplayRollStringName(39266, DMC_LIGHTGREY, actor, roll, skill-bonus, bonus, trapDC);
		}
		displaymsg->DisplayConstantStringName(STR_DISARM_DONE, DMC_LIGHTGREY, actor);
		int xp = actor->CalculateExperience(XP_DISARM, actor->GetXPLevel(1));
		const Game *game = core->GetGame();
		game->ShareXP(xp, SX_DIVIDE);
		core->GetGameControl()->ResetTargetMode();
		core->PlaySound(DS_DISARMED, SFX_CHAN_HITS);
	} else {
		AddTrigger(TriggerEntry(trigger_disarmfailed, actor->GetGlobalID()));
		if (core->HasFeature(GF_3ED_RULES)) {
			// ~Failed Disarm Device - d20 roll %d + Disarm Device skill %d + INT mod %d >= Trap DC %d~
			displaymsg->DisplayRollStringName(39266, DMC_LIGHTGREY, actor, roll, skill-bonus, bonus, trapDC);
		}
		displaymsg->DisplayConstantStringName(STR_DISARM_FAIL, DMC_LIGHTGREY, actor);
		TriggerTrap(skill, actor->GetGlobalID());
	}
	ImmediateEvent();
}

void Door::TryPickLock(const Actor *actor)
{
	if (LockDifficulty == 100) {
		if (OpenStrRef != (ieDword)-1) {
			displaymsg->DisplayStringName(OpenStrRef, DMC_BG2XPGREEN, actor, IE_STR_SOUND|IE_STR_SPEECH);
		} else {
			displaymsg->DisplayConstantStringName(STR_DOOR_NOPICK, DMC_BG2XPGREEN, actor);
		}
		return;
	}
	int stat = actor->GetStat(IE_LOCKPICKING);
	if (core->HasFeature(GF_3ED_RULES)) {
		int skill = actor->GetSkill(IE_LOCKPICKING);
		if (skill == 0) { // a trained skill, make sure we fail
			stat = 0;
		} else {
			stat *= 7; // convert to percent (magic 7 is from RE)
			int dexmod = actor->GetAbilityBonus(IE_DEX);
			stat += dexmod; // the original didn't use it, so let's not multiply it
			displaymsg->DisplayRollStringName(39301, DMC_LIGHTGREY, actor, stat-dexmod, LockDifficulty, dexmod);
		}
	}
	if (stat < (signed)LockDifficulty) {
		displaymsg->DisplayConstantStringName(STR_LOCKPICK_FAILED, DMC_BG2XPGREEN, actor);
		AddTrigger(TriggerEntry(trigger_picklockfailed, actor->GetGlobalID()));
		core->PlaySound(DS_PICKFAIL, SFX_CHAN_HITS);
		return;
	}
	SetDoorLocked( false, true);
	core->GetGameControl()->ResetTargetMode();
	displaymsg->DisplayConstantStringName(STR_LOCKPICK_DONE, DMC_LIGHTGREY, actor);
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	core->PlaySound(DS_PICKLOCK, SFX_CHAN_HITS);
	ImmediateEvent();
	int xp = actor->CalculateExperience(XP_LOCKPICK, actor->GetXPLevel(1));
	const Game *game = core->GetGame();
	game->ShareXP(xp, SX_DIVIDE);
}

void Door::TryBashLock(Actor *actor)
{
	//Get the strength bonus against lock difficulty
	int bonus;
	unsigned int roll;

	if (core->HasFeature(GF_3ED_RULES)) {
		bonus = actor->GetAbilityBonus(IE_STR);
		roll = actor->LuckyRoll(1, 100, bonus, 0);
	} else {
		int str = actor->GetStat(IE_STR);
		int strEx = actor->GetStat(IE_STREXTRA);
		bonus = core->GetStrengthBonus(2, str, strEx); //BEND_BARS_LIFT_GATES
		roll = actor->LuckyRoll(1, 10, bonus, 0);
	}

	actor->FaceTarget(this);
	if (core->HasFeature(GF_3ED_RULES)) {
		// ~Bash door check. Roll %d + %d Str mod > %d door DC.~
		displaymsg->DisplayRollStringName(20460, DMC_LIGHTGREY, actor, roll, bonus, LockDifficulty);
	}

	if(roll < LockDifficulty || LockDifficulty == 100) {
		displaymsg->DisplayConstantStringName(STR_DOORBASH_FAIL, DMC_BG2XPGREEN, actor);
		return;
	}

	displaymsg->DisplayConstantStringName(STR_DOORBASH_DONE, DMC_LIGHTGREY, actor);
	SetDoorLocked(false, true);
	core->GetGameControl()->ResetTargetMode();
	Flags|=DOOR_BROKEN;

	//This is ok, bashdoor also sends the unlocked trigger
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	ImmediateEvent();
}

void Door::dump() const
{
	StringBuffer buffer;
	buffer.appendFormatted( "Debugdump of Door %s:\n", GetScriptName() );
	buffer.appendFormatted( "Door Global ID: %d\n", GetGlobalID());
	buffer.appendFormatted( "Position: %d.%d\n", Pos.x, Pos.y);
	buffer.appendFormatted( "Door Open: %s\n", YESNO(IsOpen()));
	buffer.appendFormatted( "Door Locked: %s	Difficulty: %d\n", YESNO(Flags&DOOR_LOCKED), LockDifficulty);
	buffer.appendFormatted( "Door Trapped: %s	Difficulty: %d\n", YESNO(Trapped), TrapRemovalDiff);
	if (Trapped) {
		buffer.appendFormatted( "Trap Permanent: %s Detectable: %s\n", YESNO(Flags&DOOR_RESET), YESNO(Flags&DOOR_DETECTABLE) );
	}
	buffer.appendFormatted( "Secret door: %s (Found: %s)\n", YESNO(Flags&DOOR_SECRET),YESNO(Flags&DOOR_FOUND));
	const char *Key = GetKey();
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	buffer.appendFormatted( "Script: %s, Key (%s) removed: %s, Dialog: %s\n", name, Key?Key:"NONE", YESNO(Flags&DOOR_KEY), Dialog );

	Log(DEBUG, "Door", buffer);
}


}
