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
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"

#define IE_CHEST_CURSOR	32

extern Interface * core;

static Color cyan = {0x00, 0xff, 0xff, 0xff};
static Color red  = {0xff, 0x00, 0x00, 0xff};
static Color green = {0x00, 0xff, 0x00, 0xff};
static Color white = {0xff, 0xff, 0xff, 0xff};
static Color black = {0x00, 0x00, 0x00, 0xff};
static Color blue = {0x00, 0x00,0xff,0x80};

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
	drawPath = NULL;
	pfsX = 0;
	pfsY = 0;
	InfoTextPalette = core->GetVideoDriver()->CreatePalette(white, black);
	lastCursor = 0;
	moveX = moveY = 0;
	DebugFlags = 0;
	AIUpdateCounter = 1;
	DisableMouse = false;
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
#ifdef WIN32
		unsigned long time = GetTickCount();
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		unsigned long time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
		if((time-AIUpdateCounter) >= 66) {
			core->GSUpdate();
			AIUpdateCounter = time;
		}
		area->DrawMap(vp);
		if(DisableMouse)
			return;
		short GameX = lastMouseX, GameY = lastMouseY;
		video->ConvertToGame(GameX, GameY);
		if(DrawSelectionRect) {	
			CalculateSelection(GameX, GameY);
			
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
			if(overDoor->DoorClosed)
				video->DrawPolyline(overDoor->closed,cyan,true);
			else {
				video->DrawPolyline(overDoor->open,cyan,true);
			}
		}
		if(overContainer) {
			if(overContainer->TrapDetected && overContainer->Trapped) {
				video->DrawPolyline(overContainer->outline, red, true);
			}
			else {
				video->DrawPolyline(overContainer->outline, cyan, true);
			}
		}
		if(DebugFlags&1) {
			//draw traps with blue overlay
		}
		for(size_t i = 0; i < infoPoints.size(); i++) {
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
				font->Print(rgn, (unsigned char*)infoPoints[i]->String, InfoTextPalette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_TOP, false);
			}
		}

		if(drawPath) {
			PathNode * node = drawPath;
			while(true) {
				short GameX = (node->x*16)+8, GameY = (node->y*12)+6;
				if(!node->Parent) {
					video->DrawCircle(GameX, GameY, 2, green);
				}
				else {
					short oldX = (node->Parent->x*16)+8, oldY = (node->Parent->y*12)+6;
					video->DrawLine(oldX, oldY, GameX, GameY, green);
				}
				if(!node->Next) {
					video->DrawCircle(GameX, GameY, 2, green);
					break;
				}
				node = node->Next;
			}
		}

		if(DebugFlags&2) {
			Sprite2D * spr = area->SearchMap->GetImage();
			video->BlitSprite(spr, 0, 0, true);
			video->FreeSprite(spr);
			Region point(GameX/16, GameY/12, 1, 1);
			video->DrawRect(point, red);
		}
	}
	else {
		core->GetVideoDriver()->DrawRect(vp, blue);
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
}
/** Key Release Event */
void GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	if(!core->CheatEnabled() ) return;
	if(Mod&64 ) //ctrl
	{
	switch(Key) {
		case 'a':  //'a'
			{
				if(overContainer) {
					if(overContainer->Trapped && !(overContainer->TrapDetected)) {
						overContainer->TrapDetected = 1;
						core->GetVideoDriver()->FreeSprite(overContainer->outline->fill);
						overContainer->outline->fill = NULL;
					}
				}
			}
		break;

		case 'p': //'p'
			{
				short GameX = lastMouseX, GameY = lastMouseY;
				core->GetVideoDriver()->ConvertToGame(GameX, GameY);
				if(drawPath) {
					PathNode * nextNode = drawPath->Next;
					PathNode * thisNode = drawPath;
					while(true) {
						delete(thisNode);
						thisNode = nextNode;
						if(!thisNode)
							break;
						nextNode = thisNode->Next;
					}

				}
				drawPath = core->GetPathFinder()->FindPath(pfsX, pfsY, GameX, GameY);
			}
		break;

		case 's': // s
			{
				pfsX = lastMouseX; 
				pfsY = lastMouseY;
				core->GetVideoDriver()->ConvertToGame(pfsX, pfsY);
			}
		break;
		case 'x': // 'x'
			{
				Game * game = core->GetGame();
				Map * area = game->GetMap(MapIndex);
				//get the area resref, store it first!
				short cX = lastMouseX; 
				short cY = lastMouseY;
				core->GetVideoDriver()->ConvertToGame(cX, cY);
				printf("[%d.%d]\n",cX, cY);
			}
		break;
		case '4': // '4'  //show all traps
				DebugFlags^=1;
		break;
		case '5': // '5' //show the searchmap
				DebugFlags^=2;
		break;
		default:
			printf("KeyRelease:%d  %d\n",Key, Mod);
		break;
	}
	}
}
/** Mouse Over Event */
void GameControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if(DisableMouse)
		return;
	int nextCursor=0;

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

	switch(area->GetBlocked(GameX, GameY)) {
		case 0:
			nextCursor = 6;
		break;

		case 1:
			nextCursor = 4;
		break;

		case 2:
			nextCursor = 34;
		break;
	}

	overInfoPoint = area->tm->GetInfoPoint(GameX, GameY);
	if(overInfoPoint) {
		if(overInfoPoint->Type == 1)
			nextCursor = overInfoPoint->Cursor;
	}

	overDoor = area->tm->GetDoor(GameX, GameY);
	if(overDoor)
		nextCursor = overDoor->Cursor;

	overContainer = area->tm->GetContainer(GameX, GameY);
	if(overContainer) {
		if(overContainer->TrapDetected && overContainer->Trapped)
				nextCursor=38;
		else
				nextCursor=2;
	}

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
			if((lastActor->actor->Modified[IE_STATE_ID]&STATE_DEAD) == 0) {
				switch(lastActor->actor->Modified[IE_EA]) {
					case EVILCUTOFF:
					case GOODCUTOFF:
					break;

					case PC:
					case FAMILIAR:
					case ALLY:
					case CONTROLLED:
					case CHARMED:
					case EVILBUTGREEN:
						nextCursor = 0;
					break;

					case ENEMY:
					case GOODBUTRED:
						nextCursor = 12;
					break;
					default:
						nextCursor = 18;
					break;
				}
			}
		}
	}

	if(lastCursor != nextCursor) {
		core->GetVideoDriver()->SetCursor(core->Cursors[nextCursor]->GetFrame(0), core->Cursors[nextCursor+1]->GetFrame(0));
		lastCursor = nextCursor;
	}
}
/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	if(DisableMouse)
		return;
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
	if(DisableMouse)
		return;
	MouseIsDown = false;
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame(GameX, GameY);
	Game * game = core->GetGame();
	Map * area = game->GetMap(MapIndex);
	Door * door = area->tm->GetDoor(GameX, GameY);
	if(door) {
		area->tm->ToggleDoor(door);
		return;
	}
	if(!DrawSelectionRect) {
		ActorBlock * actor = area->GetActor(GameX, GameY);
		if(!actor && (selected.size() == 1)) {
			actor = selected.at(0);
			actor->XDes = GameX;
			actor->YDes = GameY;
			actor->path = core->GetPathFinder()->FindPath(actor->XPos, actor->YPos, GameX, GameY);
			actor->step = NULL;
		}
		for(size_t i = 0; i < selected.size(); i++)
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
		for(size_t i = 0; i < highlighted.size(); i++)
			highlighted[i]->actor->anims->DrawCircle = false;
		highlighted.clear();
		for(size_t i = 0; i < selected.size(); i++) {
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
	core->GetPathFinder()->SetMap(area->SearchMap, area->tm->XCellCount*4, (area->tm->YCellCount*64)/12);
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
			for(size_t i = 0; i < highlighted.size(); i++)
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

void GameControl::SetCutSceneMode(bool active)
{
	DisableMouse = active;
	if(active) {
		core->GetVideoDriver()->SetCursor(NULL, NULL);
	}
	else {
		core->GetVideoDriver()->SetCursor(core->Cursors[lastCursor]->GetFrame(0), core->Cursors[lastCursor+1]->GetFrame(0));
	}
}
