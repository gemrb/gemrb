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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.48 2003/12/21 09:47:35 avenger_teambg Exp $
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

/*static Color green			= {0x00, 0xff, 0x00, 0xff};
static Color red			= {0xff, 0x00, 0x00, 0xff};
static Color yellow		= {0xff, 0xff, 0x00, 0xff};
static Color cyan			= {0x00, 0xff, 0xff, 0xff};
static Color green_dark	= {0x00, 0x80, 0x00, 0xff};
static Color red_dark		= {0x80, 0x00, 0x00, 0xff};
static Color yellow_dark	= {0x80, 0x80, 0x00, 0xff};
static Color cyan_dark		= {0x00, 0x80, 0x80, 0xff};
static Color magenta		= {0xff, 0x00, 0xff, 0xff};*/

void Map::DrawMap(Region viewport)
{	
	if(tm)
		tm->DrawOverlay(0, viewport);
	if(Script)
		Script->Update();
	int ipCount = 0;
	while(true) {
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
		int i = 0;
		while(true) {
			Actor * actor = core->GetGame()->GetPC(i++);
			if(!actor)
				break;
			if(!actor->InParty)
				break;
			if(BBox.PointInside(actor->XPos, actor->YPos)) {
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
	GenerateQueue();
	while(true) {
		Actor * actor = GetRoot();
		if(!actor)
			break;
		actor->DoStep(LightMap);
		CharAnimations * ca = actor->GetAnims();
		if(!ca)
			continue;
		Animation * anim = ca->GetAnimation(actor->AnimID, actor->Orientation);
		if((!actor->Modified[IE_NOCIRCLE]) && (!(actor->Modified[IE_STATE_ID]&STATE_DEAD)))
			actor->DrawCircle();
		if(anim) {
			Sprite2D * nextFrame = anim->NextFrame();
			if(actor->lastFrame != nextFrame) {
				Region newBBox;
				newBBox.x = actor->XPos-nextFrame->XPos;
				newBBox.w = nextFrame->Width;
				newBBox.y = actor->YPos-nextFrame->YPos;
				newBBox.h = nextFrame->Height;
				actor->lastFrame = nextFrame;
				actor->SetBBox(newBBox);
			}
			if(!actor->BBox.InsideRegion(vp))
				continue;
			int ax = actor->XPos, ay = actor->YPos;
			int cx = ax/16;
			int cy = ay/12;
			Color tint = LightMap->GetPixel(cx, cy);
			tint.a = 0xA0;
			video->BlitSpriteTinted(nextFrame, ax, ay, tint);
			if(anim->endReached && anim->autoSwitchOnEnd) {
				actor->AnimID = anim->nextAnimID;
				anim->autoSwitchOnEnd = false;
			}
		}
		if(actor->textDisplaying) {
			unsigned long time;
			GetTime(time);
			if((time - actor->timeStartDisplaying) >= 6000) {
				actor->textDisplaying = 0;
			}
			if(actor->textDisplaying == 1) {
				Font * font = core->GetFont(9);
				Region rgn(actor->XPos-100, actor->YPos-100, 200, 400);
				font->Print(rgn, (unsigned char*)actor->overHeadText, NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false);
			}
		}
		for(int i = 0; i < 5; i++) {
			if(actor->Scripts[i])
				actor->Scripts[i]->Update();
		}
		if(actor->DeleteMe)
			DeleteActor(actor);
	}
	for(unsigned int i = 0; i < vvcCells.size(); i++) {
		ScriptedAnimation * vvc = vvcCells.at(i);
		if(!vvc)
			continue;
		if(vvc->anims[0]->endReached) {
			vvcCells[i] = NULL;
			delete(vvc);
			continue;
		}
		if(vvc->justCreated) {
			vvc->justCreated = false;
			if(vvc->Sounds[0][0] != 0) {
				core->GetSoundMgr()->Play(vvc->Sounds[0]);
			}
		}
		Sprite2D * frame = vvc->anims[0]->NextFrame();
		if(vvc->Transparency & IE_VVC_BRIGHTEST) {
			video->BlitSpriteMode(frame, vvc->XPos, vvc->YPos, 1, false);
		}
		else {
			video->BlitSprite(frame, vvc->XPos, vvc->YPos, false);
		}
	}
}

void Map::AddAnimation(Animation * anim)
{
	animations.push_back(anim);
}

void Map::AddActor(Actor *actor)
{
	CharAnimations * ca = actor->GetAnims();
	if(ca) {
		Animation * anim = ca->GetAnimation(actor->AnimID, actor->Orientation);
		Sprite2D * nextFrame = anim->NextFrame();
		if(actor->lastFrame != nextFrame) {
			Region newBBox;
			newBBox.x = actor->XPos-nextFrame->XPos;
			newBBox.w = nextFrame->Width;
			newBBox.y = actor->YPos-nextFrame->YPos;
			newBBox.h = nextFrame->Height;
			actor->lastFrame = nextFrame;
			actor->SetBBox(newBBox);
		}
	}
	actors.push_back(actor);
}

void Map::DeleteActor(Actor * actor)
{
	std::vector<Actor*>::iterator m;
	for(m = actors.begin(); m != actors.end(); ++m) {
		if((*m) == actor) {
			actors.erase(m);
			delete(actor);
			return;
		}
	}
}

Actor * Map::GetActor(int x, int y)
{
	for(size_t i = 0; i < actors.size(); i++) {
		Actor *actor = actors.at(i);
		if(actor->BaseStats[IE_UNSELECTABLE] || (actor->BaseStats[IE_STATE_ID]&STATE_DEAD))
			continue;
		if(actor->IsOver((unsigned short)x, (unsigned short)y))
			return actor;
	}
	return NULL;
}

Actor * Map::GetActor(const char * Name)
{
	for(size_t i = 0; i < actors.size(); i++) {
		Actor *actor = actors.at(i);
		if(stricmp(actor->scriptName, Name) == 0) {
			printf("Returning Actor %d: %s\n", i, actor->scriptName);
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorInRect(Actor ** & actors, Region &rgn)
{
	actors = (Actor**)malloc(this->actors.size()*sizeof(Actor*));
	int count = 0;
	for(size_t i = 0; i < this->actors.size(); i++) {
		Actor *actor = this->actors.at(i);
		if(actor->BaseStats[IE_UNSELECTABLE] || (actor->BaseStats[IE_STATE_ID]&STATE_DEAD))
			continue;
		if((actor->BBox.x > (rgn.x+rgn.w)) || (actor->BBox.y > (rgn.y+rgn.h)))
			continue;
		if(((actor->BBox.x+actor->BBox.w) < rgn.x) || ((actor->BBox.y+actor->BBox.h) < rgn.y))
			continue;
		actors[count++] = actor;
	}
	actors = (Actor**)realloc(actors, count*sizeof(Actor*));
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
        if(songlist<0)
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
		queue = new Actor*[actors.size()];
		lastActorCount = (int)actors.size();
	}
	Qcount = 0;
	for(unsigned int i = 0; i < actors.size(); i++) {
		Actor * actor = actors.at(i);
		Qcount++;
		queue[Qcount-1] = actor;
		int lastPos = Qcount;
		while(lastPos != 1) {
			int parentPos = (lastPos/2)-1;
			Actor * parent = queue[parentPos];
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

Actor * Map::GetRoot()
{
	if(Qcount==0)
		return NULL;
	Actor * ret = queue[0];
	Actor * node = queue[0] = queue[Qcount-1];
	Qcount--;
	int lastPos = 1;
	while(true) {
		int leftChildPos = (lastPos*2)-1;
		int rightChildPos = lastPos*2;
		if(leftChildPos >= Qcount)
			break;
		Actor * child  = queue[leftChildPos];
		int childPos = leftChildPos;
		if(rightChildPos < Qcount) { //If both Child Exist
			Actor * rightChild = queue[lastPos*2];
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

void Map::AddVVCCell(ScriptedAnimation * vvc)
{
	for(unsigned int i = 0; i < vvcCells.size(); i++) {
		if(vvcCells[i] == NULL) {
			vvcCells[i] = vvc;
			return;
		}
	}
	vvcCells.push_back(vvc);
}
