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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.34 2003/12/03 18:27:06 doc_wagon Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"
#ifndef WIN32
#include <sys/time.h>
#endif

extern Interface * core;

#define STEP_TIME 100

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
	core->FreeInterface(LightMap);
	core->FreeInterface(SearchMap);
}

void Map::AddTileMap(TileMap * tm, ImageMgr * lm, ImageMgr * sr)
{
	this->tm = tm;
	LightMap = lm;
	SearchMap = sr;
}
static Color green			= {0x00, 0xff, 0x00, 0xff};
static Color red			= {0xff, 0x00, 0x00, 0xff};
static Color yellow		= {0xff, 0xff, 0x00, 0xff};
static Color cyan			= {0x00, 0xff, 0xff, 0xff};
static Color green_dark	= {0x00, 0x80, 0x00, 0xff};
static Color red_dark		= {0x80, 0x00, 0x00, 0xff};
static Color yellow_dark	= {0x80, 0x80, 0x00, 0xff};
static Color cyan_dark		= {0x00, 0x80, 0x80, 0xff};
static Color magenta		= {0xff, 0x00, 0xff, 0xff};

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
		ActorBlock * actor = &actors.at(i);
		if(actor->path) {	
#ifndef WIN32
			struct timeval tv;
			gettimeofday(&tv, NULL);
			unsigned long time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#else
			unsigned long time = GetTickCount();
#endif
			if(!actor->step) {
				actor->step = actor->path;
				actor->timeStartStep = time;
			}
			if((time-actor->timeStartStep) >= STEP_TIME) {
				printf("[New Step] : Orientation = %d\n", actor->step->orient);
				actor->step = actor->step->Next;
				actor->timeStartStep = time;
			}
			actor->Orientation = actor->step->orient;
			actor->AnimID = IE_ANI_WALK;
			actor->XPos = (actor->step->x*16)+8;
			actor->YPos = (actor->step->y*12)+6;
			if(!actor->step->Next) {
				printf("Last Step\n");
				PathNode * nextNode = actor->path->Next;
				PathNode * thisNode = actor->path;
				while(true) {
					delete(thisNode);
					thisNode = nextNode;
					if(!thisNode)
						break;
					nextNode = thisNode->Next;
				}
				actor->path = NULL;
				actor->AnimID = IE_ANI_AWAKE;
			}
			else {
				if(actor->step->Next->x > actor->step->x)
					actor->XPos += ((((actor->step->Next->x*16)+8)-actor->XPos)*(time-actor->timeStartStep))/STEP_TIME;
				else
					actor->XPos -= ((actor->XPos-((actor->step->Next->x*16)+8))*(time-actor->timeStartStep))/STEP_TIME;
				if(actor->step->Next->y > actor->step->y)
					actor->YPos += ((((actor->step->Next->y*12)+6)-actor->YPos)*(time-actor->timeStartStep))/STEP_TIME;
				else
					actor->YPos -= ((actor->YPos-((actor->step->Next->y*12)+6))*(time-actor->timeStartStep))/STEP_TIME;
			}
		}
		CharAnimations * ca = actor->actor->GetAnims();
		if(!ca)
			continue;
		Animation * anim = ca->GetAnimation(actor->AnimID, actor->Orientation);
		bool DrawCircle=ca->DrawCircle;
		if(actor->Selected)
			DrawCircle = true;
		if(DrawCircle && (ca->CircleSize==0) ) DrawCircle=false;
		else {
			if(actor->actor->Modified[IE_NOCIRCLE]) DrawCircle=false;
			else {
				 if(actor->actor->Modified[IE_STATE_ID]&STATE_DEAD) DrawCircle=false;
			}
		}
		if(DrawCircle) {
			Color *color;

			if(actor->actor->BaseStats[IE_UNSELECTABLE]) {
				color=&magenta;
			}
			if(actor->actor->BaseStats[IE_MORALEBREAK]<actor->actor->Modified[IE_MORALEBREAK])
			{
				if(actor->Selected) color=&yellow;
				else color=&yellow_dark;
			} else switch(actors[i].actor->BaseStats[IE_EA])
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
				if(actor->Selected) color=&green;
				else color=&green_dark;
			break;

			case ENEMY:
			case GOODBUTRED:
				if(actor->Selected) color=&red;
				else color=&red_dark;
			break;
			default:
				if(actor->Selected) color=&cyan;
				else color=&cyan_dark;

			break;
			}
			video->DrawEllipse(actor->XPos-vp.x, actor->YPos-vp.y, ca->CircleSize*10, ((ca->CircleSize*15)/2), *color);
		}

		if(anim) {
			Sprite2D * nextFrame = anim->NextFrame();
			if(actor->lastFrame != nextFrame) {
				actor->MinX = actor->XPos-nextFrame->XPos;
				actor->MaxX = actor->MinX+nextFrame->Width;
				actor->MinY = actor->YPos-nextFrame->YPos;
				actor->MaxY = actor->MinY+nextFrame->Height;
				actor->lastFrame = nextFrame;
			}
			if(actor->MinX > (vp.x+vp.w))
				continue;
			if(actor->MaxX < vp.x)
				continue;
			if(actor->MinY > (vp.y+vp.h))
				continue;
			if(actor->MaxY < vp.y)
				continue;
			int ax = actor->XPos, ay = actor->YPos;
			int cx = ax/16;
			int cy = ay/12;
			Color tint = LightMap->GetPixel(cx, cy);
			tint.a = 0xA0;
			//video->BlitSprite(nextFrame, actors[i].XPos, actors[i].YPos);
			video->BlitSpriteTinted(nextFrame, ax, ay, tint);
		}
	}
	//TODO: Check if here is a door
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
	actor.Selected = false;
	actor.path = NULL;
	actor.step = NULL;
	actors.push_back(actor);
}

ActorBlock * Map::GetActor(int x, int y)
{
	for(size_t i = 0; i < actors.size(); i++) {
		ActorBlock *actor = &actors.at(i);
		if((actor->MinX > x) || (actor->MinY > y))
			continue;
		if((actor->MaxX < x) || (actor->MaxY < y))
			continue;
		return actor;
	}
	return NULL;
}
int Map::GetActorInRect(ActorBlock ** & actors, Region &rgn)
{
	actors = (ActorBlock**)malloc(this->actors.size()*sizeof(ActorBlock*));
	int count = 0;
	for(size_t i = 0; i < this->actors.size(); i++) {
		ActorBlock *actor = &this->actors.at(i);
		if((actor->MinX > (rgn.x+rgn.w)) || (actor->MinY > (rgn.y+rgn.h)))
			continue;
		if((actor->MaxX < rgn.x) || (actor->MaxY < rgn.y))
			continue;
		actors[count++] = actor;
	}
	actors = (ActorBlock**)realloc(actors, count*sizeof(ActorBlock*));
	return count;
}

void Map::PlayAreaSong(int SongType)
{
//you can speed this up by loading the songlist once at startup
        int column;
        const char *tablename;

        if(core->HasFeature(GF_HAS_SONGLIST)) {
                 column=1;
                 tablename="songlist";
        }
        else {
/*since bg1 and pst has no .2da for songlist, we must supply one in
  the gemrb/override folder.
  It should be: music.2da, first column is a .mus filename
*/
                column=0;
                tablename="music";
        }
        int songlist = core->LoadTable(tablename);
        if(!songlist)
                return;
        TableMgr * tm = core->GetTable(songlist);
        if(!tm) {
			core->DelTable(songlist);
                return;
		}
        char *poi=tm->QueryField(SongHeader.SongList[SongType],column);
        core->GetMusicMgr()->SwitchPlayList(poi, true);
}

int Map::GetBlocked(int cx, int cy) {
	int block = SearchMap->GetPixelIndex(cx/16, cy/12);
	switch(block) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 9:
		case 15:
			return 1;
		break;

		case 14:
			return 2;
		break;
	}
	return 0;
}
