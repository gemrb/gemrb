#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"

extern Interface * core;

GameControl::GameControl(void)
{
	MapIndex = -1;
	Changed = true;
	lastActor = NULL;
}

GameControl::~GameControl(void)
{
}

/** Draws the Control on the Output Display */
void GameControl::Draw(unsigned short x, unsigned short y)
{
	if(MapIndex == -1)
		return;
	Region vp(x+XPos, y+YPos, Width, Height);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	if(area)
		area->DrawMap(vp);
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
	unsigned short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	ActorBlock * actor = area->GetActor(GameX, GameY);
	if(lastActor)
		lastActor->actor->anims->DrawCircle = false;
	if(!actor) {
		lastActor = NULL;
		return;
	}
	lastActor = actor;
	lastActor->actor->anims->DrawCircle = true;
}
/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	printf("MouseDown\n");
}
/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	unsigned short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	ActorBlock * actor = area->GetActor(GameX, GameY);
	for(int i = 0; i < selected.size(); i++)
		selected[i]->Selected = false;
	selected.clear();
	if(!actor)
		return;
	selected.push_back(actor);
	actor->Selected = true;
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
}
