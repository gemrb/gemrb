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
	Clicker = NULL;
	LastEntered = NULL;
	Active = true;
	EndAction = 2;
	CurrentAction = NULL;
	startTime = 0;
	interval = ( 1000 / AI_UPDATE_TIME );
	WaitCounter = 0;
	playDeadCounter = 0;
	resetAction = false;
	neverExecuted = true;
	OnCreation = true;

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

void Scriptable::SetPosition(unsigned short XPos, unsigned short YPos)
{
	this->XPos = XPos;
	this->YPos = YPos;
}

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

void Scriptable::DisplayHeadText(char* text)
{
	if (overHeadText) {
		free( overHeadText );
	}
	overHeadText = text;
	GetTime( timeStartDisplaying );
	textDisplaying = 1;
}

void Scriptable::SetScriptName(char* text)
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
		printf( "[IEScript]: NULL action encountered!\n" );
		return;
	}
	actionQueue.push_back( aC );
	aC->IncRef();
}

void Scriptable::AddActionInFront(Action* aC)
{
	if (!aC) {
		printf( "[IEScript]: NULL action encountered!\n" );
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
	for (int i = 0; i < actionQueue.size(); i++) {
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
			mov->AnimID = IE_ANI_GET_UP;
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
			if (!neverExecuted) {
				switch (Type) {
					case ST_PROXIMITY:
						 {
							if (!( EndAction & SEA_RESET ))
								Active = false;
						}
						break;

					case ST_TRIGGER:
						 {
							Clicker = NULL;
							neverExecuted = true;
						}
						break;
				}
			}
			if (CutSceneId)
				CutSceneId = NULL;

			break;
		}
		if (Type == ST_ACTOR) {
			Moveble* actor = ( Moveble* )this;
			if (actor->AnimID == IE_ANI_DIE)
				actor->AnimID = IE_ANI_GET_UP;
		}
		printf( "Executing Action: %s\n", this->scriptName );
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

void Selectable::SetBBox(Region newBBox)
{
	BBox = newBBox;
}

void Selectable::DrawCircle()
{
	if (!size) {
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
	core->GetVideoDriver()->DrawEllipse( XPos - vp.x, YPos - vp.y, size * 10,
								( ( size * 15 ) / 2 ), *col );
}

bool Selectable::IsOver(unsigned short XPos, unsigned short YPos)
{
	return BBox.PointInside( XPos, YPos );
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

bool Highlightable::IsOver(unsigned short XPos, unsigned short YPos)
{
	if (!outline) {
		return false;
	}
	if (outline->BBox.PointInside( XPos, YPos )) {
		return outline->PointIn( XPos, YPos );
	}
	return false;
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
	XDes = XPos;
	YDes = YPos;
	Orientation = 0;
	AnimID = 0;
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

void Moveble::DoStep(ImageMgr* LightMap)
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
	AnimID = IE_ANI_WALK;
	XPos = ( step->x * 16 ) + 8;
	YPos = ( step->y * 12 ) + 6;
	if (!step->Next) {
		//printf("Last Step\n");
		ClearPath();
	} else {
		if (step->Next->x > step->x)
			XPos += ( unsigned short )
				( ( ( ( ( step->Next->x * 16 ) + 8 ) - XPos ) * ( time -
				timeStartStep ) ) /
				STEP_TIME );
		else
			XPos -= ( unsigned short )
				( ( ( XPos - ( ( step->Next->x * 16 ) + 8 ) ) * ( time -
				timeStartStep ) ) /
				STEP_TIME );
		if (step->Next->y > step->y)
			YPos += ( unsigned short )
				( ( ( ( ( step->Next->y * 12 ) + 6 ) - YPos ) * ( time -
				timeStartStep ) ) /
				STEP_TIME );
		else
			YPos -= ( unsigned short )
				( ( ( YPos - ( ( step->Next->y * 12 ) + 6 ) ) * ( time -
				timeStartStep ) ) /
				STEP_TIME );
	}
}

void Moveble::AddWayPoint(unsigned short XDes, unsigned short YDes)
{
	if(!path) {
		WalkTo(XDes, YDes);
		return;
	}
	PathNode *endNode=path;
	while(endNode->Next) {
		endNode=endNode->Next;
	}
	PathNode *path2 = core->GetPathFinder()->FindPath( endNode->x, endNode->y, XDes, YDes );
	endNode->Next=path2;
}

void Moveble::WalkTo(unsigned short XDes, unsigned short YDes)
{
	this->XDes = XDes;
	this->YDes = YDes;
/*
	if (path) {
		PathNode* nextNode = path->Next;
		PathNode* thisNode = path;
		while (true) {
			delete( thisNode );
			thisNode = nextNode;
			if (!thisNode)
				break;
			nextNode = thisNode->Next;
		}
	}
*/
	ClearPath();
	path = core->GetPathFinder()->FindPath( XPos, YPos, XDes, YDes );
//	step = NULL;
}

void Moveble::RunAwayFrom(unsigned short XDes, unsigned short YDes, int PathLength, int Backing)
{
	ClearPath();
	path = core->GetPathFinder()->RunAway( XPos, YPos, XDes, YDes, PathLength, Backing );
}

void Moveble::MoveTo(unsigned short XDes, unsigned short YDes)
{
	XPos = XDes;
	YPos = YDes;
	this->XDes = XDes;
	this->YDes = YDes;
}

void Moveble::ClearPath()
{
	if (!path) {
		return;
	}
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
	AnimID = IE_ANI_AWAKE;
	CurrentAction = NULL;
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

void Door::ToggleTiles(bool playsound)
{
	unsigned char state = ( closedIndex == 1 ) ? 0 : 1;
	if (Flags&1) {
		if (playsound && ( OpenSound[0] != '\0' ))
			core->GetSoundMgr()->Play( OpenSound );
		XPos = open->BBox.x + ( open->BBox.w / 2 );
		YPos = open->BBox.y + ( open->BBox.h / 2 );
	} else {
		if (playsound && ( CloseSound[0] != '\0' ))
			core->GetSoundMgr()->Play( CloseSound );
		XPos = closed->BBox.x + ( closed->BBox.w / 2 );
		YPos = closed->BBox.y + ( closed->BBox.h / 2 );
	}
	Flags ^=1;
	for (int i = 0; i < count; i++) {
		overlay->tiles[tiles[i]]->tileIndex = state;
	}
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
	if((Flags&1)!=Closed) {
		ToggleTiles( playsound );
	}
	if (Closed) {
		outline = closed;
	} else {
		outline = open;
	}
	if (Closed) {
		XPos = closed->BBox.x + ( closed->BBox.w / 2 );
		YPos = closed->BBox.y + ( closed->BBox.h / 2 );
	} else {
		XPos = open->BBox.x + ( open->BBox.w / 2 );
		YPos = open->BBox.y + ( open->BBox.h / 2 );
	}
}

void Door::ToggleDoorState()
{
	ToggleTiles( true );
	if (Flags&1) {
		outline = closed;
	} else {
		outline = open;
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
	printf( "DoorClosed: %s\n", Flags&1 ? "Yes":"No");
	printf( "DoorLocked: %s\n", Flags&2 ? "Yes":"No");
	printf( "DoorTrapped: %s\n", Flags&4 ? "Yes":"No");
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
	Trapped = 0;
	TrapDetected = 0;
	TrapLaunchX = 0; 
	TrapLaunchY = 0;
	KeyResRef[0] = 0;
	DialogResRef[0] = 0;
}

InfoPoint::~InfoPoint(void)
{
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
	printf( "TrapDetected: %d  Trapped: %d\n", TrapDetected, Trapped );
	printf( "Trap detection: %d  Trap removal: %d\n", TrapDetectionDifficulty,
		TrapRemovalDifficulty );
	printf( "Key: %s  Dialog: %s\n", KeyResRef, DialogResRef );
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
	Locked = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	Trapped = 0;
	TrapDetected = 0;
}

Container::~Container()
{
}

void Container::DebugDump()
{
	printf( "Debugdump of Container %s\n", Name );
	printf( "Type: %d   LockDifficulty: %d\n", Type, LockDifficulty );
	printf( "Locked: %d  Trapped: %d\n", Locked, Trapped );
	printf( "Trap detection: %d  Trap removal: %d\n", TrapDetectionDiff,
		TrapRemovalDiff );
}

