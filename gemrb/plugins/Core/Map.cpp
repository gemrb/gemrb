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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.40 2003/12/12 23:03:38 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"
#ifndef WIN32
#include <sys/time.h>
#endif

extern Interface * core;

#define STEP_TIME 150

Map::Map(void)
{
	tm = NULL;
	queue = NULL;
	Script = NULL;
	Qcount = 0;
	lastActorCount = 0;
	justCreated = true;
}

Map::~Map(void)
{
	if(tm)
		delete(tm);
	for(unsigned int i = 0; i < animations.size(); i++) {
		delete(animations[i]);
	}
	for(unsigned int i = 0; i < actors.size(); i++) {
		delete(actors[i]->actor);
		delete(actors[i]);
	}
	core->FreeInterface(LightMap);
	core->FreeInterface(SearchMap);
	if(queue)
		delete(queue);
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
	if(Script)
		Script->Update();
	int ipCount = 0;
	while(true) {
		ActorBlock ** acts;
		int count;
		InfoPoint * ip = tm->GetInfoPoint(ipCount++);
		if(!ip)
			break;
		if(ip->Type != 0)
			continue;
		Region BBox = ip->outline->BBox;
		if(BBox.x <= 500)
			BBox.x = 0;
		else
			BBox.x -= 500;
		if(BBox.y <= 500)
			BBox.y = 0;
		else
			BBox.y -= 500;
		BBox.h += 1000;
		BBox.w += 1000;
		count = GetActorInRect(acts, BBox);
		for(int x = 0; x < count; x++) {
			if(acts[x]->actor->Modified[IE_EA] == 2) {
				ip->Script->Update();
				break;
			}
		}
	}
	Video * video = core->GetVideoDriver();
	for(unsigned int i = 0; i < animations.size(); i++) {
		//TODO: Clipping Animations off screen
		video->BlitSpriteMode(animations[i]->NextFrame(), animations[i]->x, animations[i]->y, animations[i]->BlitMode, false);
	}
	Region vp = video->GetViewport();
	//for(unsigned int i = 0; i < actors.size(); i++) {
	GenerateQueue();
	while(true) {
		//ActorBlock * actor = &actors.at(i);
		ActorBlock * actor = GetRoot();
		if(!actor)
			break;
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
			} else switch(actor->actor->BaseStats[IE_EA])
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
			if(anim->endReached && anim->autoSwitchOnEnd) {
				actor->AnimID = anim->nextAnimID;
				anim->autoSwitchOnEnd = false;
			}
		}
		if(actor->textDisplaying) {
#ifdef WIN32
			unsigned long time = GetTickCount();
#else
			struct timeval tv;
			gettimeofday(&tv, NULL);
			unsigned long time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
			if((time - actor->timeStartDisplaying) >= 3000) {
				actor->textDisplaying = 0;
			}
			if(actor->textDisplaying == 1) {
				Font * font = core->GetFont(9);
				Region rgn(actor->XPos-100, actor->YPos-100, 200, 400);
				font->Print(rgn, (unsigned char*)actor->overHeadText, NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false);
			}
		}
		for(int i = 0; i < MAX_SCRIPTS; i++) {
			if(actor->Scripts[i])
				actor->Scripts[i]->Update();
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
	ActorBlock * ab = new ActorBlock();
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
	actor.textDisplaying = 0;
	actor.overHeadText = NULL;
	ab->actor = actor.actor;
	ab->AnimID = actor.AnimID;
	ab->lastFrame = actor.lastFrame;
	ab->MaxX = actor.MaxX;
	ab->MaxY = actor.MaxY;
	ab->MinX = actor.MinX;
	ab->MinY = actor.MinY;
	ab->Orientation = actor.Orientation;
	ab->overHeadText = NULL;
	ab->path = NULL;
	ab->step = NULL;
	ab->textDisplaying = 0;
	ab->timeStartDisplaying = 0;
	ab->timeStartStep = 0;
	ab->XDes = actor.XDes;
	ab->YDes = actor.YDes;
	ab->XPos = actor.XPos;
	ab->YPos = actor.YPos;
	ab->Selected = false;
	actors.push_back(ab);
}

void Map::AddActor(ActorBlock *actor)
{
	CharAnimations * ca = actor->actor->GetAnims();
	if(ca) {
		Animation * anim = ca->GetAnimation(actor->AnimID, actor->Orientation);
		Sprite2D * nextFrame = anim->NextFrame();
		if(actor->lastFrame != nextFrame) {
			actor->MinX = actor->XPos-nextFrame->XPos;
			actor->MaxX = actor->MinX+nextFrame->Width;
			actor->MinY = actor->YPos-nextFrame->YPos;
			actor->MaxY = actor->MinY+nextFrame->Width;
			actor->lastFrame = nextFrame;
		}
	}
	actor->Selected = false;
	actor->path = NULL;
	actor->step = NULL;
	actor->textDisplaying = 0;
	actor->overHeadText = NULL;
	actors.push_back(actor);
}

ActorBlock * Map::GetActor(int x, int y)
{
	for(size_t i = 0; i < actors.size(); i++) {
		ActorBlock *actor = actors.at(i);
		if((actor->MinX > x) || (actor->MinY > y))
			continue;
		if((actor->MaxX < x) || (actor->MaxY < y))
			continue;
		return actor;
	}
	return NULL;
}

ActorBlock * Map::GetActor(const char * Name)
{
	for(size_t i = 0; i < actors.size(); i++) {
		ActorBlock *actor = actors.at(i);
		if(stricmp(actor->actor->ScriptName, Name) == 0) {
			printf("Returning Actor %d: %s\n", i, actor->actor->ScriptName);
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorInRect(ActorBlock ** & actors, Region &rgn)
{
	actors = (ActorBlock**)malloc(this->actors.size()*sizeof(ActorBlock*));
	int count = 0;
	for(size_t i = 0; i < this->actors.size(); i++) {
		ActorBlock *actor = this->actors.at(i);
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

void Map::AddWallGroup(WallGroup * wg)
{
	wallGroups.push_back(wg);
}

void Map::GenerateQueue()
{
	if(lastActorCount != actors.size()) {
		if(queue)
			delete(queue);
		queue = new ActorBlock*[actors.size()];
		lastActorCount = actors.size();
	}
	Qcount = 0;
	for(int i = 0; i < actors.size(); i++) {
		ActorBlock * actor = actors.at(i);
		Qcount++;
		queue[Qcount-1] = actor;
		int lastPos = Qcount;
		while(lastPos != 1) {
			int parentPos = (lastPos/2)-1;
			ActorBlock * parent = queue[parentPos];
			if(actor->YPos < parent->YPos) {
				queue[parentPos] = actor;
				queue[lastPos-1] = parent;
				lastPos = parentPos+1;
			}
			else
				break;
		}
	}
}

ActorBlock * Map::GetRoot()
{
	if(Qcount==0)
		return NULL;
	ActorBlock * ret = queue[0];
	ActorBlock * node = queue[0] = queue[Qcount-1];
	Qcount--;
	int lastPos = 1;
	while(true) {
		int leftChildPos = (lastPos*2)-1;
		int rightChildPos = lastPos*2;
		if(leftChildPos >= Qcount)
			break;
		ActorBlock * child  = queue[leftChildPos];
		int childPos = leftChildPos;
		if(rightChildPos < Qcount) { //If both Child Exist
			ActorBlock * rightChild = queue[lastPos*2];
			if(rightChild->YPos < child->YPos) {
				childPos = rightChildPos;
				child = rightChild;
			}
			
		}
		if(node->YPos > child->YPos) {
			queue[lastPos-1] = child;
			queue[childPos] = node;
			lastPos = childPos+1;
		}
		else
			break;
	}
	return ret;
}
