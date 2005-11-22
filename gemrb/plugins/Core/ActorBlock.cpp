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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ActorBlock.cpp,v 1.116 2005/11/22 20:49:39 wjpalenstijn Exp $
 */
#include "../../includes/win32def.h"
#include "ActorBlock.h"
#include "Interface.h"
#include "SpriteCover.h"
#include <cassert>

extern Interface* core;

#define YESNO(x) ( (x)?"Yes":"No")

/***********************
 *  Scriptable Class   *
 ***********************/
Scriptable::Scriptable(ScriptableType type)
{
	Type = type;
	CutSceneId = NULL;
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		Scripts[i] = NULL;
	}
	overHeadText = NULL;
	textDisplaying = 0;
	timeStartDisplaying = 0;
	scriptName[0] = 0;
	LastTrigger = 0;
	LastEntered = 0;
	CurrentAction = NULL;
	startTime = 0;
	interval = ( 1000 / AI_UPDATE_TIME );
	WaitCounter = 0;
	playDeadCounter = 0;
	Active = SCR_ACTIVE | SCR_VISIBLE | SCR_ONCREATION;
	area = 0;
	Pos.x = 0;
	Pos.y = 0;

	locals = new Variables();
	locals->SetType( GEM_VARIABLES_INT );
}

Scriptable::~Scriptable(void)
{
	if (CurrentAction) {
		ReleaseCurrentAction();
	}
	ClearActions();
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		if (Scripts[i]) {
			delete( Scripts[i] );
		}
	}
	if (overHeadText) {
		free( overHeadText );
	}
	if (locals) {
		delete( locals );
	}
}

/** Gets the DeathVariable */
char* Scriptable::GetScriptName(void)
{
	 return scriptName;
}

Map* Scriptable::GetCurrentArea()
{
	if (area)
		return area;
	printMessage("Map","Scriptable object has no area!!!\n", LIGHT_RED);
	abort();
}

void Scriptable::SetMap(Map *map)
{
	if (!map) {
		printMessage("Scriptable","Null map set!",LIGHT_RED);
		abort();
	}
	area = map;
}

void Scriptable::SetScript(ieResRef aScript, int idx)
{
	if (idx >= MAX_SCRIPTS) {
		printMessage("Scriptable","Invalid script index!",LIGHT_RED);
		abort();
	}
	if (Scripts[idx]) {
		delete Scripts[idx];
	}
	Scripts[idx] = NULL;
	// NONE is an 'invalid' script name, never used seriously
	// This hack is to prevent flooding of the console
	if (aScript[0] && stricmp(aScript, "NONE") ) {
		Scripts[idx] = new GameScript( aScript, Type, locals, idx );
		Scripts[idx]->MySelf = this;
	}
}

void Scriptable::SetScript(int index, GameScript* script)
{
	if (index >= MAX_SCRIPTS) {
		printMessage("Scriptable","Invalid script index!",LIGHT_RED);
		return;
	}
	if (Scripts[index] ) {
		delete Scripts[index];
	}
	Scripts[index] = script;
}

void Scriptable::DisplayHeadText(const char* text)
{
	if (overHeadText) {
		free( overHeadText );
	}
	overHeadText = (char *) text;
	if (text) {
		GetTime( timeStartDisplaying );
		textDisplaying = 1;
	}
	else {
		timeStartDisplaying = 0;
		textDisplaying = 0;
	}
}

void Scriptable::SetScriptName(const char* text)
{
	strnspccpy( scriptName, text, 32 );
}

void Scriptable::ExecuteScript(GameScript* Script)
{
	if (actionQueue.size()) {
		return;
	}
	if (Script) {
		Script->Update();
	}
}

void Scriptable::AddAction(Action* aC)
{
	if (!aC) {
		printf( "[GameScript]: NULL action encountered for %s!\n",scriptName );
		return;
	}
	Active|=SCR_ACTIVE;
	actionQueue.push_back( aC );
	aC->IncRef();
}

void Scriptable::AddActionInFront(Action* aC)
{
	if (!aC) {
		printf( "[GameScript]: NULL action encountered for %s!\n",scriptName );
		return;
	}
	Active|=SCR_ACTIVE;
	actionQueue.push_front( aC );
	aC->IncRef();
}

Action* Scriptable::GetNextAction()
{
	if (actionQueue.size() == 0) {
		return NULL;
	}
	return actionQueue.front();
}

Action* Scriptable::PopNextAction()
{
	if (actionQueue.size() == 0) {
		return NULL;
	}
	Action* aC = actionQueue.front();
	actionQueue.pop_front();
	return aC;
}

void Scriptable::ClearActions()
{
	for (unsigned int i = 0; i < actionQueue.size(); i++) {
		Action* aC = actionQueue.front();
		actionQueue.pop_front();
		aC->Release();
	}
	actionQueue.clear();
	WaitCounter = 0;
	playDeadCounter = 0; // i'm not sure about this
}

void Scriptable::ReleaseCurrentAction()
{
	if (CurrentAction) {
		CurrentAction->Release();
		CurrentAction = NULL;
	}
}

void Scriptable::ProcessActions()
{
	unsigned long thisTime;
	GetTime( thisTime );
	if (( thisTime - startTime ) < interval) {
		return;
	}
	startTime = thisTime;
	if (playDeadCounter) {
		playDeadCounter--;
		if (!playDeadCounter) {
			Moveble* mov = ( Moveble* ) this;
			mov->SetStance( IE_ANI_GET_UP );
		}
	}
	if (WaitCounter) {
		WaitCounter--;
		return;
	}

	//don't do anything while moving?
	//maybe this should be fixed
	if (InMove()) {
		return;
	}

	if (CurrentAction) {
		printf("Released action in processAction: %d\n", CurrentAction->actionID);
		CurrentAction->Dump();
		ReleaseCurrentAction();
	}

	while (!CurrentAction) {
		CurrentAction = PopNextAction();
		if (!CurrentAction) {
			if (CutSceneId)
				CutSceneId = NULL;
			break;
		}
		GameScript::ExecuteAction( this, CurrentAction );
		//break execution in case of a Wait flag
		if (WaitCounter) {
			break;
		}
		//break execution in case of movement
		if (InMove()) {
			break;
		}
	}
}

bool Scriptable::InMove()
{
	if (Type!=ST_ACTOR) {
		return false;
	}
	Moveble *me = (Moveble *) this;
	return me->path!=NULL;
}

void Scriptable::SetWait(unsigned long time)
{
	WaitCounter = time;
}

unsigned long Scriptable::GetWait()
{
	return WaitCounter;
}

void Scriptable::InitTriggers()
{
	tolist.clear();
	bittriggers = 0;
}

void Scriptable::ClearTriggers()
{
	for (TriggerObjects::iterator m = tolist.begin(); m != tolist.end (); m++) {
		*(*m) = 0;
	}
	if (bittriggers & BT_DIE) {
		((Actor *) this)->InternalFlags&=~IF_JUSTDIED;
	}
	if (bittriggers & BT_ONCREATION) {
		Active &= ~SCR_ONCREATION;
	}
}

void Scriptable::SetBitTrigger(ieDword bittrigger)
{
	bittriggers |= bittrigger;
}

void Scriptable::AddTrigger(ieDword *actorref)
{
	tolist.push_back(actorref);
}

/********************
 * Selectable Class *
 ********************/

Selectable::Selectable(ScriptableType type)
	: Scriptable( type )
{
	Selected = false;
	Over = false;
	size = 0;
}

Selectable::~Selectable(void)
{
}

void Selectable::SetBBox(Region &newBBox)
{
	BBox = newBBox;
}

void Selectable::DrawCircle(Region &vp)
{
	if (size<=0) {
		return;
	}
	Color* col = NULL;
	if (Selected) {
		col = &selectedColor;
	} else if (Over) {
		col = &overColor;
	} else {
		return;
	}
	//Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->DrawEllipse( Pos.x - vp.x, Pos.y - vp.y,
		 size * 10, ( ( size * 15 ) / 2 ), *col );
}

bool Selectable::IsOver(Point &Pos)
{
	return BBox.PointInside( Pos );
}

bool Selectable::IsSelected()
{
	return Selected==1;
}

void Selectable::SetOver(bool over)
{
	Over = over;
}

void Selectable::Select(int Value)
{
	if (Selected!=0x80 || Value!=1) {
		Selected = Value;
	}
}

void Selectable::SetCircle(int circlesize, Color color)
{
	size = circlesize;
	selectedColor = color;
	overColor.r = color.r >> 1;
	overColor.g = color.g >> 1;
	overColor.b = color.b >> 1;
}

/***********************
 * Highlightable Class *
 ***********************/

Highlightable::Highlightable(ScriptableType type)
	: Scriptable( type )
{
	outline = NULL;
	Highlight = false;
	Cursor = IE_CURSOR_NORMAL;
	KeyResRef[0] = 0;
}

Highlightable::~Highlightable(void)
{
	if (outline) {
		delete( outline );
	}
}

bool Highlightable::IsOver(Point &Pos)
{
	if (!outline) {
		return false;
	}
	return outline->PointIn( Pos );
}

void Highlightable::DrawOutline()
{
	if (!outline) {
		return;
	}
	core->GetVideoDriver()->DrawPolyline( outline, outlineColor, true );
}

void Highlightable::SetCursor(unsigned char CursorIndex)
{
	Cursor = CursorIndex;
}

/*****************
 * Moveble Class *
 *****************/

Moveble::Moveble(ScriptableType type)
	: Selectable( type )
{
	Destination = Pos;
	Orientation = 0;
	StanceID = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	lastFrame = NULL;
	Area[0] = 0;
}

Moveble::~Moveble(void)
{
	if (path) {
		ClearPath();
	}
}

void Moveble::SetStance(unsigned int arg)
{
	if (arg<MAX_ANIMS) {
		 StanceID=(unsigned char) arg;
	} else {
		StanceID=IE_ANI_AWAKE; //
		printf("Tried to set invalid stance id (%u)\n", arg);
	}
}

//this could be used for WingBuffet as well
void Moveble::MoveLine(int steps, int Pass)
{
	//remove previous path
	ClearPath();
	if (!steps)
		return;
	path = area->GetLine( Pos, steps, Orientation, Pass );
}

void Moveble::DoStep()
{
	if (!path) {
		return;
	}
	unsigned long time;
	GetTime( time );
	if (!step) {
		step = path;
		timeStartStep = time;
	}
	if (( time - timeStartStep ) >= STEP_TIME) {
		//printf("[New Step] : Orientation = %d\n", step->orient);
		step = step->Next;
		timeStartStep = time;
	}
	//Orientation = step->orient;
	SetOrientation (step->orient, false);
	StanceID = IE_ANI_WALK;
	Pos.x = ( step->x * 16 ) + 8;
	Pos.y = ( step->y * 12 ) + 6;
	if (!step->Next) {
		ClearPath();
		NewOrientation = Orientation;
		//since clearpath no longer sets currentaction to NULL
		//we set it here
		ReleaseCurrentAction();
		return;
	}
	if (step->Next->x > step->x)
		Pos.x += ( unsigned short )
			( ( ( ( ( step->Next->x * 16 ) + 8 ) - Pos.x ) * ( time - timeStartStep ) ) / STEP_TIME );
	else
		Pos.x -= ( unsigned short )
			( ( ( Pos.x - ( ( step->Next->x * 16 ) + 8 ) ) * ( time - timeStartStep ) ) / STEP_TIME );
	if (step->Next->y > step->y)
		Pos.y += ( unsigned short )
			( ( ( ( ( step->Next->y * 12 ) + 6 ) - Pos.y ) * ( time - timeStartStep ) ) / STEP_TIME );
	else
		Pos.y -= ( unsigned short )
			( ( ( Pos.y - ( ( step->Next->y * 12 ) + 6 ) ) * ( time - timeStartStep ) ) / STEP_TIME );
}

void Moveble::AddWayPoint(Point &Des)
{
	if (!path) {
		WalkTo(Des);
		return;
	}
	Destination = Des;
	PathNode *endNode=path;
	while(endNode->Next) {
		endNode=endNode->Next;
	}
	Point p(endNode->x, endNode->y);
	PathNode *path2 = area->FindPath( p, Des );
	endNode->Next=path2;
}

void Moveble::WalkTo(Point &Des, int distance)
{
	ClearPath();
	path = area->FindPath( Pos, Des, distance );
	//ClearPath sets destination, so Destination must be set after it
	//also we should set Destination only if there is a walkable path
	if (path) {
		Destination = Des;
	}
}

void Moveble::RunAwayFrom(Point &Des, int PathLength, int flags)
{
	ClearPath();
	path = area->RunAway( Pos, Des, PathLength, flags );
}

void Moveble::RandomWalk(bool can_stop)
{
	ClearPath();
	//if not continous random walk, then stops for a while
	if (can_stop && !(rand()%10) ) {
		SetWait(10);
		return;
	}
	path = area->RunAway( Pos, Pos, 10, 0 );
}

void Moveble::MoveTo(Point &Des)
{
	Pos = Des;
	Destination = Des;
}

void Moveble::ClearPath()
{
	//this is to make sure attackers come to us
	//make sure ClearPath doesn't screw Destination (in the rare cases Destination
	//is set before ClearPath
	Destination = Pos;
	if (StanceID==IE_ANI_WALK || StanceID==IE_ANI_RUN) {
		StanceID = IE_ANI_AWAKE;
	}
	InternalFlags&=~IF_NORECTICLE;
	if (!path) {
		return;
	}
	PathNode* nextNode = path->Next;
	PathNode* thisNode = path;
	while (true) {
		delete( thisNode );
		if (!nextNode)
			break;
		thisNode = nextNode;
		nextNode = thisNode->Next;
	}
	path = NULL;
	step = NULL;
	//don't call ReleaseCurrentAction
}

static unsigned long tp_steps[8]={3,2,1,0,1,2,3,4};

void Moveble::DrawTargetPoint(Region &vp)
{
	if (!path || !Selected || (InternalFlags&IF_NORECTICLE) )
		return;

	// generates "step" from sequence 3 2 1 0 1 2 3 4
	// updated each 1/15 sec
	unsigned long step;
	GetTime( step );
	step = tp_steps [(step >> 6) & 7];

	//Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->DrawEllipse( Destination.x - vp.x,
		Destination.y - vp.y, (unsigned short) (size * 10 - step),
		(unsigned short) ( size * 15 / 2 - step), selectedColor );

}

/**********************
 * Tiled Object Class *
 **********************/

TileObject::TileObject()
{
	opentiles=NULL;
	opencount=0;
	closedtiles=NULL;
	closedcount=0;
	Flags = 0;
}

TileObject::~TileObject()
{
	if (opentiles) {
		free( opentiles );
	}
	if (closedtiles) {
		free( closedtiles );
	}
}

void TileObject::SetOpenTiles(unsigned short* Tiles, int cnt)
{
	if (opentiles) {
		free( opentiles );
	}
	opentiles = Tiles;
	opencount = cnt;
}

void TileObject::SetClosedTiles(unsigned short* Tiles, int cnt)
{
	if (closedtiles) {
		free( closedtiles );
	}
	closedtiles = Tiles;
	closedcount = cnt;
}

/**************
 * Door Class *
 **************/

Door::Door(TileOverlay* Overlay)
	: Highlightable( ST_DOOR )
{
	tiles = NULL;
	count = 0;
	Flags = 0;
	open = NULL;
	closed = NULL;
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
	InternalFlags = 0;
}

Door::~Door(void)
{
	if (Flags&DOOR_OPEN) {
		if (open) {
			delete( open );
		}
	} else {
		if (closed) {
			delete( closed );
		}
	}
	if (tiles) {
		free( tiles );
	}
	if (open_ib) {
		free( open_ib );
	}
	if (closed_ib) {
		free( closed_ib );
	}
}

void Door::UpdateDoor()
{
	if (Flags&DOOR_OPEN) {
		outline = closed;
	} else {
		outline = open;
	}
	int i;

	int oidx, cidx;
	int oval, cval;

	oval = 1; //passable searchmap entry
	if (Flags & DOOR_TRANSPARENT) {
		cval = 8; //transparent door searchmap entry
	}
	else {
		cval = 0; //opaque door searchmap entry
	}
	if (Flags &DOOR_OPEN) {
		oidx=cval;
		cidx=oval;
	}
	else {
		cidx=cval;
		oidx=oval;
	}

	for(i=0;i<oibcount;i++) {
		area->SearchMap->SetPixelIndex( open_ib[i].x, open_ib[i].y, oidx );
	}
	for(i=0;i<cibcount;i++) {
		area->SearchMap->SetPixelIndex( closed_ib[i].x, closed_ib[i].y, cidx );
	}
	InfoPoint *ip=area->TMap->GetInfoPoint(LinkedInfo);
	if (ip) {
		if (Flags&DOOR_OPEN) ip->Flags&=~INFO_DOOR;
		else ip->Flags|=INFO_DOOR;
	}
}

void Door::ToggleTiles(int State, bool playsound)
{
	int i;
	int state;

	if (State) {
		state = !closedIndex;
		if (playsound && ( OpenSound[0] != '\0' ))
			core->GetSoundMgr()->Play( OpenSound );
	} else {
		state = closedIndex;
		if (playsound && ( CloseSound[0] != '\0' ))
			core->GetSoundMgr()->Play( CloseSound );
	}
	for (i = 0; i < count; i++) {
		overlay->tiles[tiles[i]]->tileIndex = state;
	}
	//set door_open as state
	Flags = (Flags & ~DOOR_OPEN) | (State==!core->HasFeature(GF_REVERSE_DOOR) );
}

//this is the short name (not the scripting name)
void Door::SetName(const char* name)
{
	strnuprcpy( ID, name, 8 );
}

void Door::SetTiles(unsigned short* Tiles, int cnt)
{
	if (tiles) {
		free( tiles );
	}
	tiles = Tiles;
	count = cnt;
}

void Door::SetDoorLocked(bool Locked, bool playsound)
{
	if (Locked) {
		if (Flags & DOOR_LOCKED) return;
		Flags|=DOOR_LOCKED;
		if (playsound && ( LockSound[0] != '\0' ))
			core->GetSoundMgr()->Play( LockSound );
	}
	else {
		if (!(Flags & DOOR_LOCKED)) return;
		Flags&=~DOOR_LOCKED;
		if (playsound && ( UnLockSound[0] != '\0' ))
			core->GetSoundMgr()->Play( UnLockSound );
	}
}

bool Door::IsOpen() const
{
	return (Flags&DOOR_OPEN) == !core->HasFeature(GF_REVERSE_DOOR);
}

void Door::SetDoorOpen(bool Open, bool playsound)
{
	if (Open) SetDoorLocked(false,playsound);
	ToggleTiles (Open, playsound);
	UpdateDoor ();
	area->FixAllPositions();
}

void Door::SetPolygon(bool Open, Gem_Polygon* poly)
{
	if (Open) {
		if (open)
			delete( open );
		open = poly;
	} else {
		if (closed)
			delete( closed );
		closed = poly;
	}
}

void Door::TryPickLock(Actor *actor)
{
	if (TrapFlags) {
		//trap fired
		if (!(Flags & DOOR_RESET) ) {
			//trap removed
		}
		return;
	}
	if (actor->GetStat(IE_LOCKPICKING)<LockDifficulty) {
		if (LockDifficulty==100) {
			//special message
		}
		return;
	}
	SetDoorLocked( false, true);
	//add XP ?
}

void Door::DebugDump()
{
	printf( "Debugdump of Door %s:\n", GetScriptName() );
	printf( "Door Open: %s\n", YESNO(IsOpen()));
	printf( "Door Locked: %s\n", YESNO(Flags&DOOR_LOCKED));
	printf( "Door Trapped: %s (permanent:%s)\n", YESNO(TrapFlags), YESNO(Flags&DOOR_RESET));
	printf( "Trap Detectable: %s\n", YESNO(Flags&DOOR_DETECTABLE) );
	printf( "Secret door: %s (Found: %s)\n", YESNO(Flags&DOOR_SECRET),YESNO(Flags&DOOR_FOUND));
	const char *Key = GetKey();
	printf( "Key (%s) removed: %s\n", Key?Key:"NONE", YESNO(Flags&DOOR_KEY) );
}

/*******************
 * InfoPoint Class *
 *******************/

InfoPoint::InfoPoint(void)
	: Highlightable( ST_TRIGGER )
{
	Destination[0] = 0;
	EntranceName[0] = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	TrapDetected = 0;
	TrapLaunch.x = 0; 
	TrapLaunch.y = 0;
	DialogResRef[0] = 0;
}

InfoPoint::~InfoPoint(void)
{
}

//checks if the actor may use this travel trigger
//bit 1 : can use
//bit 2 : whole team
int InfoPoint::CheckTravel(Actor *actor)
{
	if (!(Flags&TRAP_RESET)) return CT_CANTMOVE; //experimental hack
	if (Flags&TRAP_DEACTIVATED) return CT_CANTMOVE;
	if (!actor->InParty && (Flags&TRAVEL_NONPC) ) return CT_CANTMOVE;
	if (Flags&TRAVEL_PARTY) {
		if (core->HasFeature(GF_TEAM_MOVEMENT) || core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE) ) {
			return CT_WHOLE;
		}
		return CT_GO_CLOSER;
	}
	if(actor->IsSelected() )
	{
		if(core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE|ENP_ONLYSELECT) )
		{
			return CT_MOVE_SELECTED;
		}
		return CT_SELECTED;
	}
	return CT_ACTIVE;
}

//detect this trap, using a skill, skill could be set to 256 for 'sure'
//skill is the all around modified trap detection skill 
//a trapdetectiondifficulty of 100 means impossible detection short of a spell
void InfoPoint::DetectTrap(int skill)
{
	if (Type!=ST_TRIGGER) return;
	if (!Scripts[0]) return;
	if (Flags&(TRAP_DEACTIVATED|TRAP_INVISIBLE) ) return;
	if ((skill>=100) && (skill!=256) ) skill=100;
	if (skill/2+core->Roll(1,skill/2,0)>TrapDetectionDiff) {
		TrapDetected=1; //probably could be set to the player #?
	}
}

//trap that is visible on screen (marked by red)
//if TrapDetected is a bitflag, we could show traps selectively for
//players, really nice for multiplayer
bool InfoPoint::VisibleTrap(bool see_all)
{
	if (Type!=ST_TRIGGER) return false;
	if (!Scripts[0]) return false;
	if (Flags&(TRAP_DEACTIVATED|TRAP_INVISIBLE) ) return false;
	if (see_all) return true;
	if (TrapDetected ) return true;
	return false;
}

//trap that will fire now
bool InfoPoint::TriggerTrap(int skill)
{
	if (Type!=ST_PROXIMITY) {
		return false;
	}
	//actually this could be script name[0]
	if (!Scripts[0]) {
		return false;
	}
	if (Flags&TRAP_DEACTIVATED) {
		return false;
	}
	if (Flags&TRAP_DETECTABLE) {
		TrapDetected=1; //probably too late :)
		if ((skill>=100) && (skill!=256) ) skill=100;
		if (skill/2+core->Roll(1,skill/2,0)>TrapDetectionDiff) {
			//tumble???
			return false;
		}
	}
	if (Flags&TRAP_RESET) {
		return true;
	}
	Flags|=TRAP_DEACTIVATED;
	return true;
}

void InfoPoint::Entered(Actor *actor)
{
	if (actor->InParty || (Flags&TRAP_NPC) ) {
		//skill?
		if (TriggerTrap(0) ) {
			LastTrigger = LastEntered = actor->GetID();
		}
	}
}

void InfoPoint::DebugDump()
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
	printf( "TrapDetected: %d, Trapped: %s\n", TrapDetected, YESNO(Scripts[0]));
	printf( "Trap detection: %d, Trap removal: %d\n", TrapDetectionDiff,
		TrapRemovalDiff );
	printf( "Key: %s, Dialog: %s\n", KeyResRef, DialogResRef );
	printf( "Active: %s\n", YESNO(Active));
}

/*******************
 * Container Class *
 *******************/

Container::Container(void)
	: Highlightable( ST_CONTAINER )
{
	Type = 0;
	LockDifficulty = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	Trapped = 0;
	TrapDetected = 0;
	inventory.SetInventoryType(INVENTORY_HEAP);
	// NULL should be 0 for this 
	memset (groundicons, 0, sizeof(groundicons) );
	groundiconcover = 0;
}

void Container::FreeGroundIcons()
{
	Video* video = core->GetVideoDriver();

	for (int i=0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			video->FreeSprite( groundicons[i] );
			groundicons[i]=NULL;
		}
	}
	delete groundiconcover;
	groundiconcover = 0;
}

Container::~Container()
{
	FreeGroundIcons();
}

void Container::DrawPile(bool highlight, Region screen, Color tint)
{
	Video* video = core->GetVideoDriver();
	for (int i=0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			if (!groundiconcover || !groundiconcover->Covers(Pos.x, Pos.y, groundicons[i]->XPos, groundicons[i]->YPos, groundicons[i]->Width, groundicons[i]->Height)) {
				// FIXME: somewhat suboptimal...
				int xpos = 0;
				int ypos = 0;
				int width = 0;
				int height = 0;
				if (groundiconcover) {
					xpos = groundiconcover->XPos;
					ypos = groundiconcover->YPos;
					width = groundiconcover->Width;
					height = groundiconcover->Height;
				}
				if (xpos < groundicons[i]->XPos) {
					width += groundicons[i]->XPos - xpos;
					xpos = groundicons[i]->XPos;
				}
				if (ypos < groundicons[i]->YPos) {
					height += groundicons[i]->YPos - ypos;
					ypos = groundicons[i]->YPos;
				}
				if (width < groundicons[i]->Width) {
					width = groundicons[i]->Width;
				}
				if (height < groundicons[i]->Height) {
					height = groundicons[i]->Height;
				}
				delete groundiconcover;
				groundiconcover = GetCurrentArea()->BuildSpriteCover(Pos.x, Pos.y,
												   xpos, ypos, width, height);
				assert(groundiconcover->Covers(Pos.x, Pos.y, groundicons[i]->XPos, groundicons[i]->YPos, groundicons[i]->Width, groundicons[i]->Height));
			}

			//draw it with highlight
			if (highlight) {
				video->BlitSpriteCovered (groundicons[i], screen.x + Pos.x, screen.y + Pos.y, tint, groundiconcover);
			} else {
				//draw with second color transparent too
				video->BlitSpriteNoShadow (groundicons[i], screen.x + Pos.x, screen.y + Pos.y, tint, groundiconcover);
			}
		}
	}
}

//This function doesn't exist in the original IE, destroys a container
//turning it to a ground pile
void Container::DestroyContainer()
{
	//it is already a groundpile?
	if (Type==IE_CONTAINER_PILE)
		return;
	Type = IE_CONTAINER_PILE;
	RefreshGroundIcons();
	//probably we should stop the script or trigger it, whatever
}

//Takes an item from the container's inventory and returns its pointer
CREItem *Container::RemoveItem(unsigned int idx, unsigned int count)
{
	CREItem *ret = inventory.RemoveItem(idx, count);
	//we just took the 3. or less item, groundpile changed
	if ((Type==IE_CONTAINER_PILE) && (inventory.GetSlotCount()<3)) {
		RefreshGroundIcons();
	}
	return ret;
}

//Adds an item to the container's inventory
//containers always have enough capacity (so far), thus we always return 2
int Container::AddItem(CREItem *item)
{
	inventory.AddItem(item);
	//we just added a 3. or less item, groundpile changed
	if ((Type==IE_CONTAINER_PILE) && (inventory.GetSlotCount()<4)) {
		RefreshGroundIcons();
	}
	return 2;
}

void Container::RefreshGroundIcons()
{
	int i = inventory.GetSlotCount();
	if (i>MAX_GROUND_ICON_DRAWN)
		i=MAX_GROUND_ICON_DRAWN;
	FreeGroundIcons();
	while (i--) {
		CREItem *slot = inventory.GetSlotItem(i); //borrowed reference
		Item *itm = core->GetItem( slot->ItemResRef ); //cached reference
		//well, this is required in PST, needs more work if some other
		//game is broken by not using -1,0
		groundicons[i] = core->GetBAMSprite( itm->GroundIcon, 0, 0 );
		core->FreeItem( itm, slot->ItemResRef ); //decref
	}
}

void Container::DebugDump()
{
	printf( "Debugdump of Container %s\n", GetScriptName() );
	printf( "Type: %d,  LockDifficulty: %d\n", Type, LockDifficulty );
	printf( "Flags: %d, Trapped: %d\n", Flags, Trapped );
	printf( "Trap detection: %d, Trap removal: %d\n", TrapDetectionDiff,
		TrapRemovalDiff );
}
