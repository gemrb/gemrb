#include "../../includes/win32def.h"
#include "ActorBlock.h"
#include "Interface.h"

extern Interface * core;

/***********************
 *	Scriptable Class   *
 ***********************/
Scriptable::Scriptable(ScriptableType type)
{
	Type = type;
	MySelf = NULL;
	CutSceneId = NULL;
	for(int i = 0; i < MAX_SCRIPTS; i++) {
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
	interval = (1000/AI_UPDATE_TIME);
	WaitCounter = 0;
	resetAction = false;
	neverExecuted = true;

	locals = new Variables();
	locals->SetType(GEM_VARIABLES_INT);
}

Scriptable::~Scriptable(void)
{
	for(int i = 0; i < MAX_SCRIPTS; i++) {
		if(Scripts[i])
			delete(Scripts[i]);
	}
	if(overHeadText)
		free(overHeadText);
	if(locals)
		delete(locals);
}

void Scriptable::SetPosition(unsigned short XPos, unsigned short YPos)
{
	this->XPos = XPos;
	this->YPos = YPos;
}

void Scriptable::SetMySelf(Scriptable * MySelf)
{
	this->MySelf = MySelf;
}

void Scriptable::SetScript(int index, GameScript * script)
{
	if(index >= MAX_SCRIPTS)
		return;
	Scripts[index] = script;
}

void Scriptable::DisplayHeadText(char * text)
{
	if(overHeadText)
		free(overHeadText);
	overHeadText = text;
	GetTime(timeStartDisplaying);
	textDisplaying = 1;
}

void Scriptable::SetScriptName(char * text) 
{
	strncpy(scriptName, text, 32);
	scriptName[32] = 0;
}

void Scriptable::ExecuteScript(GameScript * Script)
{
	Script->Update();
}

void Scriptable::AddAction(Action * aC)
{
	actionQueue.push_back(aC);
}

Action * Scriptable::GetNextAction()
{
	if(actionQueue.size() == 0)
		return NULL;
	return actionQueue.front();
}

Action * Scriptable::PopNextAction()
{
	if(actionQueue.size() == 0)
		return NULL;
	Action * aC = actionQueue.front();
	actionQueue.pop_front();
	return aC;
}

void Scriptable::ClearActions()
{
	actionQueue.clear();
}

void Scriptable::ProcessActions()
{
	unsigned long thisTime;
	GetTime(thisTime);
	if((thisTime-startTime) < interval)
		return;
	startTime = thisTime;
	if(WaitCounter) {
		WaitCounter--;
		if(!WaitCounter)
			CurrentAction = NULL;
		return;
	}
	if(resetAction) {
		CurrentAction = NULL;
		resetAction = false;
	}
	while(!CurrentAction) {
		CurrentAction = PopNextAction();
		if(!CurrentAction) {
			if(!neverExecuted) {
				switch(Type) {
					case ST_PROXIMITY:
						{
						if(!(EndAction & SEA_RESET))
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
			if(CutSceneId)
				CutSceneId = NULL;
			
			break;
		}
		GameScript::ExecuteAction(this, CurrentAction);
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

Selectable::Selectable(ScriptableType type) : Scriptable(type) 
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
	if(!size)
		return;
	Color *col = NULL;
	if(Selected) {
		col = &selectedColor;
	} else if(Over) {
		col = &overColor;
	} else
		return;
	Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->DrawEllipse(XPos-vp.x, YPos-vp.y, size*10, ((size*15)/2), *col);
}

bool Selectable::IsOver(unsigned short XPos, unsigned short YPos)
{
	return BBox.PointInside(XPos, YPos);
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
    overColor.r = color.r>>1;
	overColor.g = color.g>>1;
	overColor.b = color.b>>1;
}

/***********************
 * Highlightable Class *
 ***********************/

Highlightable::Highlightable(ScriptableType type) : Scriptable(type) 
{ 
	outline = NULL; 
}

Highlightable::~Highlightable(void)
{

}

bool Highlightable::IsOver(unsigned short XPos, unsigned short YPos)
{
	if(!outline)
		return false;
	if(outline->BBox.PointInside(XPos, YPos))
		return outline->PointIn(XPos, YPos);
	return false;
}

void Highlightable::DrawOutline()
{
	if(!outline)
		return;
	core->GetVideoDriver()->DrawPolyline(outline, outlineColor, true);
}

/*****************
 * Moveble Class *
 *****************/

Moveble::Moveble(ScriptableType type) : Selectable(type) 
{
	XDes = XPos;
	YDes = YPos;
	Orientation = 0;
	AnimID = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	lastFrame = NULL;
}

Moveble::~Moveble(void)
{

}

void Moveble::DoStep(ImageMgr * LightMap)
{	
	if(!path)
		return;
	unsigned long time;
	GetTime(time);
	if(!step) {
		step = path;
		timeStartStep = time;
	}
	if((time-timeStartStep) >= STEP_TIME) {
		//printf("[New Step] : Orientation = %d\n", step->orient);
		step = step->Next;
		timeStartStep = time;
	}
	Orientation = step->orient;
	AnimID = IE_ANI_WALK;
	XPos = (step->x*16)+8;
	YPos = (step->y*12)+6;
	if(!step->Next) {
		//printf("Last Step\n");
		PathNode * nextNode = path->Next;
		PathNode * thisNode = path;
		while(true) {
			delete(thisNode);
			thisNode = nextNode;
			if(!thisNode)
				break;
			nextNode = thisNode->Next;
		}
		path = NULL;
		AnimID = IE_ANI_AWAKE;
		CurrentAction = NULL;
	}
	else {
		if(step->Next->x > step->x)
			XPos += (unsigned short)(((((step->Next->x*16)+8)-XPos)*(time-timeStartStep))/STEP_TIME);
		else
			XPos -= (unsigned short)(((XPos-((step->Next->x*16)+8))*(time-timeStartStep))/STEP_TIME);
		if(step->Next->y > step->y)
			YPos += (unsigned short)(((((step->Next->y*12)+6)-YPos)*(time-timeStartStep))/STEP_TIME);
		else
			YPos -= (unsigned short)(((YPos-((step->Next->y*12)+6))*(time-timeStartStep))/STEP_TIME);
	}
}
void Moveble::WalkTo(unsigned short XDes, unsigned short YDes)
{
	this->XDes = XDes;
	this->YDes = YDes;
	if(path) {
		PathNode * nextNode = path->Next;
		PathNode * thisNode = path;
		while(true) {
			delete(thisNode);
			thisNode = nextNode;
			if(!thisNode)
				break;
			nextNode = thisNode->Next;
		}
	}
	path = core->GetPathFinder()->FindPath(XPos, YPos, XDes, YDes);
	step = NULL;
}
void Moveble::MoveTo(unsigned short XDes, unsigned short YDes)
{
	XPos = XDes;
	YPos = YDes;
	this->XDes = XDes;
	this->YDes = YDes;
}

/**************
 * Door Class *
 **************/

Door::Door(TileOverlay * Overlay) : Highlightable(ST_DOOR) 
{
	Name[0] = 0;
	tiles = NULL;
	count = 0;
	DoorClosed = false;
	open = NULL;
	closed = NULL;
	Cursor = 0;
	OpenSound[0] = 0;
	CloseSound[0] = 0;
	overlay = Overlay;
}

Door::~Door(void)
{
	if(open)
		delete(open);
	if(closed)
		delete(closed);
	if(tiles)
		free(tiles);
}

void Door::ToggleTiles(bool playsound) 
{
	unsigned char state = 0;
	if(DoorClosed) {
		state = 1;
		if(playsound && (CloseSound[0] != '\0'))
			core->GetSoundMgr()->Play(CloseSound);
	}
	else {
		if(playsound && (OpenSound[0] != '\0'))
			core->GetSoundMgr()->Play(OpenSound);
	}
	for(int i = 0; i < count; i++) {
		overlay->tiles[tiles[i]]->tileIndex = state;
	}
}

void Door::SetName(char * Name)
{
	strncpy(this->Name, Name, 8);
	this->Name[8] = 0;
}

void Door::SetTiles(unsigned short * Tiles, int count)
{
	if(tiles)
		free(tiles);
	tiles = Tiles;
	this->count = count;
}

void Door::SetDoorClosed(bool Closed, bool playsound)
{
	if(Closed) {
		outline = closed;
	}
	else {
		outline = open;
	}
	if(DoorClosed == Closed)
		return;
	DoorClosed = Closed;
	ToggleTiles(playsound);
}

void Door::ToggleDoorState()
{
	DoorClosed = !DoorClosed;
	ToggleTiles(true);
	if(DoorClosed)
		outline = closed;
	else
		outline = open;
}

void Door::SetPolygon(bool Open, Gem_Polygon * poly)
{
	if(Open) {
		if(open)
			delete(open);
		open = poly;
	}
	else {
		if(closed)
			delete(closed);
		closed = poly;
	}
	if(DoorClosed)
		outline = closed;
	else
		outline = open;
}

void Door::SetCursor(unsigned char CursorIndex)
{
	Cursor = CursorIndex;
}

/**************
 * InfoPoint Class *
 **************/

InfoPoint::InfoPoint(void) : Highlightable(ST_TRIGGER) 
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

