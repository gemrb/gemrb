#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"

extern Interface * core;

GameControl::GameControl(void)
{
	MapIndex = -1;
	Changed = true;
	lastActor = NULL;
	MouseIsDown = false;
	DrawSelectionRect = false;
	overDoor = NULL;
	lastCursor = 0;
	moveX = moveY = 0;
}

GameControl::~GameControl(void)
{
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
		if(!actor)
			return;
		selected.push_back(actor);
		actor->Selected = true;
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
