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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.15 2003/11/26 16:36:38 balrog994 Exp $
 *
 */

#define INANIMATE		1
#define PC				2
#define FAMILIAR		3
#define ALLY			4
#define CONTROLLED		5
#define CHARMED			6
#define GOODBUTRED		28
#define GOODBUTBLUE		29
#define GOODCUTOFF		30
#define NOTGOOD			31
#define ANYTHING		126
#define NEUTRAL			128
#define NOTEVIL			199
#define EVILCUTOFF		200
#define EVILBUTGREEN	201
#define EVILBUTBLUE		202
#define ENEMY			255

#define STATE_DEAD		0x00000800

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"

extern Interface * core;

Map::Map(void)
{
	tm = NULL;
}

Map::~Map(void)
{
	if(tm)
		delete(tm);
	for(unsigned int i = 0; i < animations.size(); i++) {
		delete(animations[i]);
	}
	for(unsigned int i = 0; i < actors.size(); i++) {
		delete(actors[i].actor);
	}
}

void Map::AddTileMap(TileMap * tm, ImageMgr * lm)
{
	this->tm = tm;
	LightMap = lm;
}
Color green = {0x00, 0xff, 0x00, 0xff};
Color red   = {0xff, 0x00, 0x00, 0xff};
Color yellow= {0xff, 0xff, 0x00, 0xff};
Color cyan  = {0x00, 0xff, 0xff, 0xff};
void Map::DrawMap(Region viewport)
{	
	if(tm)
		tm->DrawOverlay(0, viewport);
	Video * video = core->GetVideoDriver();
	for(unsigned int i = 0; i < animations.size(); i++) {
		//TODO: Clipping Animations off screen
		video->BlitSprite(animations[i]->NextFrame(), animations[i]->x, animations[i]->y);
	}
	Region vp = video->GetViewport();
	for(unsigned int i = 0; i < actors.size(); i++) {
		CharAnimations * ca = actors[i].actor->GetAnims();
		if(!ca)
			continue;
		Animation * anim = ca->GetAnimation(actors[i].AnimID, actors[i].Orientation);
		bool DrawCircle=ca->DrawCircle;
		if(DrawCircle && (ca->CircleSize==0) ) DrawCircle=false;
		else {
			if(actors[i].actor->Modified[IE_NOCIRCLE]) DrawCircle=false;
			else {
				 if(actors[i].actor->Modified[IE_STATE_ID]&STATE_DEAD) DrawCircle=false;
			}
		}
		
		if(DrawCircle) {	
			switch(actors[i].actor->BaseStats[IE_EA])
			{
			case EVILCUTOFF:
			case GOODCUTOFF:
			break;

			case PC:
			case FAMILIAR:
			case ALLY:
			case CONTROLLED:
			case CHARMED:
			case EVILBUTGREEN:
				video->DrawEllipse(actors[i].XPos-vp.x, actors[i].YPos-vp.y, ca->CircleSize*10, ((ca->CircleSize*15)/2), green);
			break;

			case ENEMY:
			case GOODBUTRED:
				video->DrawEllipse(actors[i].XPos-vp.x, actors[i].YPos-vp.y, ca->CircleSize*10, ((ca->CircleSize*15)/2), red);
			break;

			case NEUTRAL:
				video->DrawEllipse(actors[i].XPos-vp.x, actors[i].YPos-vp.y, ca->CircleSize*10, ((ca->CircleSize*15)/2), yellow);
			break;

			default:
				printf("%d\n", actors[i].actor->BaseStats[IE_EA]);
				video->DrawEllipse(actors[i].XPos-vp.x, actors[i].YPos-vp.y, ca->CircleSize*10, ((ca->CircleSize*15)/2), cyan);
			break;
			}
		}
		if(anim) {
			Sprite2D * nextFrame = anim->NextFrame();
			if(actors[i].lastFrame != nextFrame) {
				actors[i].MinX = actors[i].XPos-nextFrame->XPos;
				actors[i].MaxX = actors[i].MinX+nextFrame->Width;
				actors[i].MinY = actors[i].YPos-nextFrame->YPos;
				actors[i].MaxY = actors[i].MinY+nextFrame->Height;
				actors[i].lastFrame = nextFrame;
			}
			if(actors[i].MinX > (vp.x+vp.w))
				continue;
			if(actors[i].MaxX < vp.x)
				continue;
			if(actors[i].MinY > (vp.y+vp.h))
				continue;
			if(actors[i].MaxY < vp.y)
				continue;
			int ax = actors[i].XPos, ay = actors[i].YPos;
			int cx = ax/16;
			int cy = ay/16;
			Color tint = LightMap->GetPixel(cx, cy);
			tint.a = 0xA0;
			//video->BlitSprite(nextFrame, actors[i].XPos, actors[i].YPos);
			video->BlitSpriteTinted(nextFrame, ax, ay, tint);
		}
	}
}

void Map::AddAnimation(Animation * anim)
{
	animations.push_back(anim);
}

void Map::AddActor(ActorBlock actor)
{
	CharAnimations * ca = actor.actor->GetAnims();
	if(ca) {
		Animation * anim = ca->GetAnimation(actor.AnimID, actor.Orientation);
		Sprite2D * nextFrame = anim->NextFrame();
		if(actor.lastFrame != nextFrame) {
			actor.MinX = actor.XPos-nextFrame->XPos;
			actor.MaxX = actor.MinX+nextFrame->Width;
			actor.MinY = actor.YPos-nextFrame->YPos;
			actor.MaxY = actor.MinY+nextFrame->Width;
			actor.lastFrame = nextFrame;
		}
	}
	actors.push_back(actor);
}

ActorBlock * Map::GetActor(int x, int y)
{
	for(int i = 0; i < actors.size(); i++) {
		ActorBlock *actor = &actors.at(i);
		if((actor->MinX > x) || (actor->MinY > y))
			continue;
		if((actor->MaxX < x) || (actor->MaxY < y))
			continue;
		return actor;
	}
	return NULL;
}
