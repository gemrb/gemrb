#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"

#define IE_CHEST_CURSOR	32

extern Interface * core;

GameControl::GameControl(void)
{
	MapIndex = -1;
	Changed = true;
	lastActor = NULL;
	MouseIsDown = false;
	DrawSelectionRect = false;
	overDoor = NULL;
	overContainer = NULL;
	overInfoPoint = NULL;
	Color white = {0xff, 0xff, 0xff, 0xff}, black = {0x00, 0x00, 0x00, 0xff};
	InfoTextPalette = core->GetVideoDriver()->CreatePalette(white, black);
	lastCursor = 0;
	moveX = moveY = 0;
}

GameControl::~GameControl(void)
{
	free(InfoTextPalette);
}

/** Draws the Control on the Output Display */
void GameControl::Draw(unsigned short x, unsigned short y)
{
	if(MapIndex == -1)
		return;
	Video * video = core->GetVideoDriver();
	Region viewport = core->GetVideoDriver()->GetViewport();
	viewport.x += moveX;
	viewport.y += moveY;
	video->SetViewport(viewport.x, viewport.y);
	Region vp(x+XPos, y+YPos, Width, Height);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	if(area) {
		area->DrawMap(vp);
		if(DrawSelectionRect) {
			short GameX = lastMouseX, GameY = lastMouseY;
			video->ConvertToGame(GameX, GameY);
			CalculateSelection(GameX, GameY);

			Color green = {0x00, 0xff, 0x00, 0xff};
			
			short xs[4] = {SelectionRect.x, SelectionRect.x+SelectionRect.w, SelectionRect.x+SelectionRect.w, SelectionRect.x};
			short ys[4] = {SelectionRect.y, SelectionRect.y, SelectionRect.y+SelectionRect.h, SelectionRect.y+SelectionRect.h};
			Point points[4] = {
				{SelectionRect.x, SelectionRect.y},
				{SelectionRect.x+SelectionRect.w, SelectionRect.y},
				{SelectionRect.x+SelectionRect.w, SelectionRect.y+SelectionRect.h},
				{SelectionRect.x, SelectionRect.y+SelectionRect.h}
			};
			Gem_Polygon poly(points, 4);
			video->DrawPolyline(&poly, green, false);
		}
		if(overDoor) {
			Color cyan = {0x00, 0xff, 0xff, 0xff};
			if(overDoor->DoorClosed)
				video->DrawPolyline(overDoor->closed, cyan, true);	
			else 
				video->DrawPolyline(overDoor->open, cyan, true);	
		}
		if(overContainer) {
			Color cyan = {0x00, 0xff, 0xff, 0xff};
			Color red  = {0xff, 0x00, 0x00, 0xff};
			if(overContainer->TrapDetected && overContainer->Trapped) {
				video->DrawPolyline(overContainer->outline, red, true);
				core->GetVideoDriver()->SetCursor(core->Cursors[39]->GetFrame(0),core->Cursors[40]->GetFrame(0));
			}
			else {
				video->DrawPolyline(overContainer->outline, cyan, true);
				core->GetVideoDriver()->SetCursor(core->Cursors[2]->GetFrame(0),core->Cursors[3]->GetFrame(0));
			}
		}
		for(int i = 0; i < infoPoints.size(); i++) {
#ifdef WIN32
			unsigned long time = GetTickCount();
#else
			struct timeval tv;
			gettimeofday(&tv, NULL);
			unsigned long time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
			if((time - infoPoints[i]->startDisplayTime) >= 10000) {
				infoPoints[i]->textDisplaying = 0;
				std::vector<InfoPoint*>::iterator m;
				m = infoPoints.begin()+i;
				infoPoints.erase(m);
				i--;
				continue;
			}
			if(infoPoints[i]->textDisplaying == 1) {
				Font * font = core->GetFont(9);
				Region rgn(infoPoints[i]->outline->BBox.x+(infoPoints[i]->outline->BBox.w/2)-100, infoPoints[i]->outline->BBox.y, 200, 400);
				font->Print(rgn, (unsigned char*)infoPoints[i]->String, InfoTextPalette, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false);
			}
		}
	}
	else {
		Color Blue;
		Blue.b = 0xff;
		Blue.r = 0;
		Blue.g = 0;
		Blue.a = 128;
		core->GetVideoDriver()->DrawRect(vp, Blue);
	}
}
/** Sets the Text of the current control */
int GameControl::SetText(const char * string, int pos)
{
	return 0;
}
/** Key Press Event */
void GameControl::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	printf("KeyPress\n");
}
/** Key Release Event */
void GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	printf("KeyRelease\n");
}
/** Mouse Over Event */
void GameControl::OnMouseOver(unsigned short x, unsigned short y)
{
	lastMouseX = x;
	lastMouseY = y;
	if(x < 20)
		moveX = -5;
	else if(x > (core->Width-20))
		moveX = 5;
	else
		moveX = 0;
	if(y < 20)
		moveY = -5;
	else if(y > (core->Height-20))
		moveY = 5;
	else
		moveY = 0;
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	if(MouseIsDown && (!DrawSelectionRect)) {
		if((abs(GameX-StartX) > 5) || (abs(GameY-StartY) > 5))
			DrawSelectionRect = true;
	}
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	
	if(!DrawSelectionRect) {
		ActorBlock * actor = area->GetActor(GameX, GameY);
		if(lastActor)
			lastActor->actor->anims->DrawCircle = false;
		if(!actor) {
			lastActor = NULL;
		}
		else {
			lastActor = actor;
			lastActor->actor->anims->DrawCircle = true;
		}
	}
	//CalculateSelection(GameX, GameY);

	Door * door = area->tm->GetDoor(GameX, GameY);
	if(door) {
		if(door->Cursor != lastCursor) {
			core->GetVideoDriver()->SetCursor(core->Cursors[door->Cursor]->GetFrame(0), core->Cursors[door->Cursor+1]->GetFrame(0));
			lastCursor = door->Cursor;
		}
		overDoor = door;
	}
	else {
		if(lastCursor != 0) {
			core->GetVideoDriver()->SetCursor(core->Cursors[0]->GetFrame(0), core->Cursors[1]->GetFrame(0));
			lastCursor = 0;
		}
		overDoor = NULL;
	}
	overContainer = area->tm->GetContainer(GameX, GameY);
	if(overContainer) {
		if(lastCursor != IE_CHEST_CURSOR) {
			core->GetVideoDriver()->SetCursor(core->Cursors[IE_CHEST_CURSOR]->GetFrame(0), core->Cursors[IE_CHEST_CURSOR+1]->GetFrame(0));
			lastCursor = IE_CHEST_CURSOR;
		}
	}
	overInfoPoint = area->tm->GetInfoPoint(GameX, GameY);
	if(overInfoPoint) {
		if(overInfoPoint->Cursor != lastCursor) {
			core->GetVideoDriver()->SetCursor(core->Cursors[overInfoPoint->Cursor]->GetFrame(0), core->Cursors[overInfoPoint->Cursor+1]->GetFrame(0));
			lastCursor = overInfoPoint->Cursor;
		}
	}
}
/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	MouseIsDown = true;
	SelectionRect.x = GameX;
	SelectionRect.y = GameY;
	StartX = GameX;
	StartY = GameY;
	SelectionRect.w = 0;
	SelectionRect.h = 0;
}
/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	MouseIsDown = false;
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	Door * door = area->tm->GetDoor(GameX, GameY);
	if(door) {
		area->tm->ToogleDoor(door);
	}
	if(!DrawSelectionRect) {
		ActorBlock * actor = area->GetActor(GameX, GameY);
		for(int i = 0; i < selected.size(); i++)
			selected[i]->Selected = false;
		selected.clear();
		if(actor) {
			selected.push_back(actor);
			actor->Selected = true;
		}
		if(overInfoPoint) {
			if(overInfoPoint->Type == 1) {
				if(overInfoPoint->textDisplaying != 1) {
					overInfoPoint->textDisplaying = 1;
					infoPoints.push_back(overInfoPoint);
#ifdef WIN32
					overInfoPoint->startDisplayTime = GetTickCount();
#else
					struct timeval tv;
					gettimeofday(&tv, NULL);
					overInfoPoint->startDisplayTime = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
				}
			}
		}
	}
	else {
		ActorBlock ** ab;
		int count = area->GetActorInRect(ab, SelectionRect);
		for(int i = 0; i < highlighted.size(); i++)
			highlighted[i]->actor->anims->DrawCircle = false;
		highlighted.clear();
		for(int i = 0; i < selected.size(); i++) {
			selected[i]->Selected = false;
			selected[i]->actor->anims->DrawCircle = false;
		}
		selected.clear();
		if(count != 0) {
			for(int i = 0; i < count; i++) {
				ab[i]->Selected = true;
				selected.push_back(ab[i]);
			}
		}
		free(ab);
	}
	DrawSelectionRect = false;
}
/** Special Key Press */
void GameControl::OnSpecialKeyPress(unsigned char Key)
{	
	Region Viewport = core->GetVideoDriver()->GetViewport();
	switch(Key) {
		case GEM_RIGHT:
			Viewport.x += 64;
		break;

		case GEM_LEFT:
			Viewport.x -= 64;
		break;

		case GEM_UP:
			Viewport.y -= 64;
		break;

		case GEM_DOWN:
			Viewport.y += 64;
		break;

		case GEM_MOUSEOUT:
			moveX = 0;
			moveY = 0;
		break;
	}
	core->GetVideoDriver()->SetViewport(Viewport.x, Viewport.y);
}
void GameControl::SetCurrentArea(int Index)
{
	MapIndex = Index;
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	ActorBlock ab = game->GetPC(0);
	if(ab.actor)
		area->AddActor(ab);
	//night or day?
	area->PlayAreaSong(0);
}

void GameControl::CalculateSelection(unsigned short x, unsigned short y)
{
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	if(DrawSelectionRect) {
		if(x < StartX) {
			SelectionRect.w = StartX-x;
			SelectionRect.x = x;
		}
		else {
			SelectionRect.x = StartX;
			SelectionRect.w = x-StartX;
		}
		if(y < StartY) {
			SelectionRect.h = StartY-y;
			SelectionRect.y = y;
		}
		else {
			SelectionRect.y = StartY;
			SelectionRect.h = y-StartY;
		}
		ActorBlock ** ab;
		int count = area->GetActorInRect(ab, SelectionRect);
		if(count != 0) {
			for(int i = 0; i < highlighted.size(); i++)
				highlighted[i]->actor->anims->DrawCircle = false;
			highlighted.clear();
			for(int i = 0; i < count; i++) {
				ab[i]->actor->anims->DrawCircle = true;
				highlighted.push_back(ab[i]);
			}
		}
		free(ab);
	}
	else {
		ActorBlock * actor = area->GetActor(x, y);
		if(lastActor)
			lastActor->actor->anims->DrawCircle = false;
		if(!actor) {
			lastActor = NULL;
		}
		else {
			lastActor = actor;
			lastActor->actor->anims->DrawCircle = true;
		}
	}
}
