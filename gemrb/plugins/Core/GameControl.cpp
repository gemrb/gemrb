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
	Region vp(x+XPos, y+YPos, Width, Height);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	if(area) {
		area->DrawMap(vp);
		if(DrawSelectionRect) {
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
				video->DrawPolyline(overDoor->closed, cyan, false);	
			else 
				video->DrawPolyline(overDoor->open, cyan, false);	
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
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	if(MouseIsDown && (!DrawSelectionRect)) {
	if((abs(GameX-StartX) > 5) || (abs(GameY-StartY) > 5))
		DrawSelectionRect = true;
	}
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	if(DrawSelectionRect) {
		if(GameX < SelectionRect.x) {
			SelectionRect.w = StartX-GameX;
			SelectionRect.x = GameX;
		}
		else {
			SelectionRect.x = StartX;
			SelectionRect.w = GameX-SelectionRect.x;
		}
		if(GameY < SelectionRect.y) {
			SelectionRect.h = StartY-GameY;
			SelectionRect.y = GameY;
		}
		else {
			SelectionRect.y = StartY;
			SelectionRect.h = GameY-SelectionRect.y;
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
	Door * door = area->tm->GetDoor(GameX, GameY);
	if(door)
		overDoor = door;
	else
		overDoor = NULL;
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
	printf("SpecialKeyPress\n");
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
