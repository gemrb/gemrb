/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ActorBlock.cpp,v 1.66 2004/11/24 21:47:04 avenger_teambg Exp $
 */
#include "../../includes/win32def.h"
#include "ActorBlock.h"
#include "Interface.h"

extern Interface* core;

/***********************
 *	Scriptable Class   *
 ***********************/
Scriptable::Scriptable(ScriptableType type)
{
	Type = type;
	MySelf = NULL;
	CutSceneId = NULL;
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		Scripts[i] = NULL;
	}
	overHeadText = NULL;
	textDisplaying = 0;
	timeStartDisplaying = 0;
	scriptName[0] = 0;
	LastTrigger = NULL;
	LastEntered = NULL;
	Active = true;
	CurrentAction = NULL;
	startTime = 0;
	interval = ( 1000 / AI_UPDATE_TIME );
	WaitCounter = 0;
	playDeadCounter = 0;
	resetAction = false;
	neverExecuted = true;
	OnCreation = true;
	Pos.x = 0;
	Pos.y = 0;

	locals = new Variables();
	locals->SetType( GEM_VARIABLES_INT );
}

Scriptable::~Scriptable(void)
{
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

void Scriptable::SetScript(const char* aScript, int idx)
{
	if (Scripts[idx]) {
		delete Scripts[idx];
	}
	Scripts[idx] = 0;
	if (aScript[0]) {
		Scripts[idx] = new GameScript( aScript, 0, locals );
		Scripts[idx]->MySelf = this;
	}
}

/* unused
void Scriptable::SetPosition(Map *map, Point &Position)
{
	Pos = Position;
	area = map;
}
*/
void Scriptable::SetMySelf(Scriptable* MySelf)
{
	this->MySelf = MySelf;
}

void Scriptable::SetScript(int index, GameScript* script)
{
	if (index >= MAX_SCRIPTS) {
		return;
	}
	Scripts[index] = script;
}

void Scriptable::DisplayHeadText(const char* text)
{
	if (overHeadText) {
		free( overHeadText );
	}
	overHeadText = (char *) text;
	GetTime( timeStartDisplaying );
	textDisplaying = 1;
}

void Scriptable::SetScriptName(const char* text)
{
	strncpy( scriptName, text, 32 );
	scriptName[32] = 0;
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
		printf( "[IEScript]: NULL action encountered for %s!\n",scriptName );
		return;
	}
	actionQueue.push_back( aC );
	aC->IncRef();
}

void Scriptable::AddActionInFront(Action* aC)
{
	if (!aC) {
		printf( "[IEScript]: NULL action encountered for %s!\n",scriptName );
		return;
	}
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
	if (CurrentAction) {
		//CurrentAction->Release();
		CurrentAction = NULL;
	}
	for (unsigned int i = 0; i < actionQueue.size(); i++) {
		Action* aC = actionQueue.front();
		actionQueue.pop_front();
		aC->Release();
	}
	actionQueue.clear();
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
			Moveble* mov = ( Moveble* ) MySelf;
			mov->SetStance( IE_ANI_GET_UP );
		}
	}
	if (WaitCounter) {
		WaitCounter--;
		if (!WaitCounter)
			CurrentAction = NULL;
		return;
	}
	if (resetAction) {
		CurrentAction = NULL;
		resetAction = false;
	}
	while (!CurrentAction) {
		CurrentAction = PopNextAction();
		if (!CurrentAction) {
			/* we don't need this
			if (!neverExecuted) {
				switch (Type) {
					case ST_PROXIMITY:
						 {
							if (!( Flags & TRAP_RESET ))
								Active = false;
						}
						break;

					case ST_TRIGGER:
						 {
							LastTrigger = NULL;
							neverExecuted = true;
						}
						break;
					default: //all others are unused
						break;
				}
			}
			*/
			if (CutSceneId)
				CutSceneId = NULL;
			break;
		}
		if (Type == ST_ACTOR) {
			Moveble* actor = ( Moveble* )this;
			if (actor->GetStance() == IE_ANI_SLEEP )
				actor->SetStance( IE_ANI_GET_UP );
		}
		GameScript::ExecuteAction( this, CurrentAction );
		neverExecuted = false;
	}
}

void Scriptable::SetWait(unsigned long time)
{
	WaitCounter = time;
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

void Selectable::DrawCircle()
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
	Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->DrawEllipse( Pos.x - vp.x, Pos.y - vp.y,
		 size * 10, ( ( size * 15 ) / 2 ), *col );
}

bool Selectable::IsOver(Point &Pos)
{
	return BBox.PointInside( Pos );
}

bool Selectable::IsSelected()
{
	return Selected;
}

void Selectable::SetOver(bool over)
{
	Over = over;
}

void Selectable::Select(bool Value)
{
	Selected = Value;
}

void Selectable::SetCircle(int size, Color color)
{
	this->size = size;
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
	TalkCount = 0;
}

Moveble::~Moveble(void)
{
}

unsigned char Moveble::GetStance()
{
	return StanceID;
}

void Moveble::SetStance(unsigned int arg)
{
	if(arg<MAX_ANIMS) StanceID=(unsigned char) arg;
	else
	{
		StanceID=IE_ANI_AWAKE; //
		printMessage("Actor","Tried to set invalid stance id",LIGHT_RED);
	}
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
	Orientation = step->orient;
	StanceID = IE_ANI_WALK;
	Pos.x = ( step->x * 16 ) + 8;
	Pos.y = ( step->y * 12 ) + 6;
	if (!step->Next) {
		//printf("Last Step\n");
		ClearPath();
	} else {
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
}

void Moveble::AddWayPoint(Point &Des)
{
	if(!path) {
		WalkTo(Des);
		return;
	}
	PathNode *endNode=path;
	while(endNode->Next) {
		endNode=endNode->Next;
	}
	Game* game = core->GetGame();
	Map* map = game->GetMap(Area);
	Point p = {endNode->x, endNode->y};
	PathNode *path2 = map->FindPath( p, Des );
	endNode->Next=path2;
}

void Moveble::WalkTo(Point &Des)
{
	Destination = Des;
	ClearPath();
	Game* game = core->GetGame();
	Map* map = game->GetMap(Area);
	path = map->FindPath( Pos, Destination );
}

void Moveble::RunAwayFrom(Point &Des, int PathLength, bool Backing)
{
	ClearPath();
	Game* game = core->GetGame();
	Map* map = game->GetMap(Area);
	path = map->RunAway( Pos, Des, PathLength, Backing );
}

void Moveble::MoveTo(Point &Des)
{
	Pos = Des;
	Destination = Des;
}

void Moveble::ClearPath()
{
	if (!path) {
		return;
	}
	InternalFlags&=~IF_NORECTICLE;
	PathNode* nextNode = path->Next;
	PathNode* thisNode = path;
	while (true) {
		delete( thisNode );
		thisNode = nextNode;
		if (!thisNode)
			break;
		nextNode = thisNode->Next;
	}
	path = NULL;
	step = NULL;
	StanceID = IE_ANI_AWAKE;
	CurrentAction = NULL;
}

static unsigned long tp_steps[8]={3,2,1,0,1,2,3,4};

void Moveble::DrawTargetPoint()
{
	if (!path || !Selected || (InternalFlags&IF_NORECTICLE) )
		return;

	// generates "step" from sequence 3 2 1 0 1 2 3 4
	//   updated each 1/15 sec
	unsigned long step;
	GetTime( step );
	step = tp_steps [(step >> 6) & 7];

	Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->DrawEllipse( Destination.x - vp.x,
		Destination.y - vp.y, (unsigned short) (size * 10 - step),
		(unsigned short) ( size * 15 / 2 - step), selectedColor );

}

/**************
 * Door Class *
 **************/

Door::Door(TileOverlay* Overlay)
	: Highlightable( ST_DOOR )
{
	Name[0] = 0;
	tiles = NULL;
	count = 0;
	Flags = 0;
	open = NULL;
	closed = NULL;
	Cursor = 0;
	OpenSound[0] = 0;
	CloseSound[0] = 0;
	overlay = Overlay;
}

Door::~Door(void)
{
	if (Flags&1) {
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
}

void Door::ToggleTiles(int State, bool playsound)
{
	int i;
	int state;

	if (State) {
		state = !closedIndex;
		if (playsound && ( OpenSound[0] != '\0' ))
			core->GetSoundMgr()->Play( OpenSound );
		Pos.x = open->BBox.x + ( open->BBox.w / 2 );
		Pos.y = open->BBox.y + ( open->BBox.h / 2 );
	} else {
		state = closedIndex;
		if (playsound && ( CloseSound[0] != '\0' ))
			core->GetSoundMgr()->Play( CloseSound );
		Pos.x = closed->BBox.x + ( closed->BBox.w / 2 );
		Pos.y = closed->BBox.y + ( closed->BBox.h / 2 );
	}
	for (i = 0; i < count; i++) {
		overlay->tiles[tiles[i]]->tileIndex = state;
	}
	Flags = (Flags & ~1) | State;
}

void Door::SetName(char* Name)
{
	strncpy( this->Name, Name, 8 );
	this->Name[8] = 0;
}

void Door::SetTiles(unsigned short* Tiles, int count)
{
	if (tiles) {
		free( tiles );
	}
	tiles = Tiles;
	this->count = count;
}

void Door::SetDoorLocked(bool Locked, bool playsound)
{
	if(Locked) {
		if(!(Flags&1) ) {
			SetDoorClosed(1,playsound); // or just return?
		}
		Flags|=2;
	}
	else {
		if(Flags&1) {
			SetDoorClosed(0,playsound); // or just return?
		}
		Flags&=~2;
	}
}

void Door::SetDoorClosed(bool Closed, bool playsound)
{
	ToggleTiles(Closed, playsound );
	if (Closed) {
		outline = closed;
	} else {
		outline = open;
	}
	if (Closed) {
		Pos.x = closed->BBox.x + ( closed->BBox.w / 2 );
		Pos.y = closed->BBox.y + ( closed->BBox.h / 2 );
	} else {
		Pos.x = open->BBox.x + ( open->BBox.w / 2 );
		Pos.y = open->BBox.y + ( open->BBox.h / 2 );
	}
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
	if (Flags&1) {
		outline = closed;
	} else {
		outline = open;
	}
}

void Door::SetCursor(unsigned char CursorIndex)
{
	Cursor = CursorIndex;
}

void Door::DebugDump()
{
	printf( "Debugdump of Door %s:\n", Name );
	printf( "Door Closed: %s\n", Flags&1 ? "Yes":"No");
	printf( "Door Locked: %s\n", Flags&2 ? "Yes":"No");
	printf( "Door Trapped: %s\n", Flags&4 ? "Yes":"No");
	printf( "Trap removable: %s\n", Flags&8 ? "Yes":"No");
}

/*******************
 * InfoPoint Class *
 *******************/

InfoPoint::InfoPoint(void)
	: Highlightable( ST_TRIGGER )
{
	Name[0] = 0;
	Destination[0] = 0;
	EntranceName[0] = 0;
	Flags = 0;
	TrapDetectionDifficulty = 0;
	TrapRemovalDifficulty = 0;
	TrapDetected = 0;
	TrapLaunch.x = 0; 
	TrapLaunch.y = 0;
	KeyResRef[0] = 0;
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
	if(!(Flags&TRAP_RESET)) return 0; //experimental hack
	if(Flags&TRAP_DEACTIVATED) return 0;
	if(!actor->InParty && (Flags&TRAVEL_NONPC) ) return 0;
	if(Flags&TRAVEL_PARTY) {
		if(core->HasFeature(GF_TEAM_MOVEMENT) || core->GetGame()->EveryoneNearPoint(actor->Area, actor->Pos, true) ) {
			return 3;
		}
		return 2;
	}
	return 1;
}

//detect this trap, using a skill, skill could be set to 256 for 'sure'
//skill is the all around modified trap detection skill 
//a trapdetectiondifficulty of 100 means impossible detection short of a spell
void InfoPoint::DetectTrap(int skill)
{
	if(Type!=ST_TRIGGER) return;
	if(!Scripts[0]) return;
	if(Flags&(TRAP_DEACTIVATED|TRAP_INVISIBLE) ) return;
	if((skill>=100) && (skill!=256) ) skill=100;
	if(skill/2+core->Roll(1,skill/2,0)>TrapDetectionDifficulty) {
		TrapDetected=1; //probably could be set to the player #?
	}
}

//trap that is visible on screen (marked by red)
//if TrapDetected is a bitflag, we could show traps selectively for
//players, really nice for multiplayer
bool InfoPoint::VisibleTrap(bool see_all)
{
	if(Type!=ST_TRIGGER) return false;
	if(!Scripts[0]) return false;
	if(Flags&(TRAP_DEACTIVATED|TRAP_INVISIBLE) ) return false;
	if(see_all) return true;
	if(TrapDetected ) return true;
	return false;
}

//trap that will fire now
bool InfoPoint::TriggerTrap(int skill)
{
	if(Type!=ST_TRIGGER) return false;
	//actually this could be script name[0]
	if(!Scripts[0]) return false;
	if(Flags&TRAP_DEACTIVATED) return false;
	TrapDetected=1; //probably too late :)
	if((skill>=100) && (skill!=256) ) skill=100;
	if(skill/2+core->Roll(1,skill/2,0)>TrapDetectionDifficulty) {
		//tumble???
		return false;
	}
	if(Flags&TRAP_RESET) return true;
	Flags|=TRAP_DEACTIVATED;
	return true;
}

void InfoPoint::DebugDump()
{
	switch (Type) {
		case ST_TRIGGER:
			printf( "Debugdump of InfoPoint Region %s:\n", Name );
			break;
		case ST_PROXIMITY:
			printf( "Debugdump of Trap Region %s:\n", Name );
			break;
		case ST_TRAVEL:
			printf( "Debugdump of Travel Region %s:\n", Name );
			break;
		default:
			printf( "Debugdump of Unsupported Region %s:\n", Name );
			break;
	}
	printf( "TrapDetected: %d  Trapped: %s\n", TrapDetected, Scripts[0]?"Yes":"No" );
	printf( "Trap detection: %d  Trap removal: %d\n", TrapDetectionDifficulty,
		TrapRemovalDifficulty );
	printf( "Key: %s  Dialog: %s\n", KeyResRef, DialogResRef );
	printf( "Active: %s\n", Active?"Yes":"No");
}

/*******************
 * Container Class *
 *******************/

Container::Container(void)
	: Highlightable( ST_CONTAINER )
{
	Name[0] = 0;
	Type = 0;
	LockDifficulty = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	Trapped = 0;
	TrapDetected = 0;
	inventory.SetInventoryType(INVENTORY_HEAP);
}

Container::~Container()
{
}

void Container::DebugDump()
{
	printf( "Debugdump of Container %s\n", Name );
	printf( "Type: %d   LockDifficulty: %d\n", Type, LockDifficulty );
	printf( "Flags: %d  Trapped: %d\n", Flags, Trapped );
	printf( "Trap detection: %d  Trap removal: %d\n", TrapDetectionDiff,
		TrapRemovalDiff );
}

