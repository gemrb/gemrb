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

void Map::AddTileMap(TileMap *tm)
{
	this->tm = tm;
}

void Map::DrawMap(void)
{	
	if(tm)
		tm->DrawOverlay(0);
	Video * video = core->GetVideoDriver();
	Region vp = video->GetViewport();
	for(unsigned int i = 0; i < animations.size(); i++) {
		//TODO: Clipping Animations off screen
		video->BlitSprite(animations[i]->NextFrame(), animations[i]->x, animations[i]->y);
	}
	for(unsigned int i = 0; i < actors.size(); i++) {
		CharAnimations * ca = actors[i].actor->GetAnims();
		Animation * anim = ca->GetAnimation(actors[i].AnimID, actors[i].Orientation);
		if(anim)
			video->BlitSprite(anim->NextFrame(), actors[i].XPos, actors[i].YPos);
	}
}

void Map::AddAnimation(Animation * anim)
{
	animations.push_back(anim);
}

void Map::AddActor(ActorBlock actor)
{
	actors.push_back(actor);
}
