#ifndef ACTORBLOCK_H
#define ACTORBLOCK_H

#include "Sprite2D.h"
#include "Actor.h"
#include "PathFinder.h"
#include "GameScript.h"

typedef struct ActorBlock {
	unsigned short XPos, YPos;
	unsigned short XDes, YDes;
	unsigned short MinX, MaxX, MinY, MaxY;
	unsigned char Orientation;
	unsigned char AnimID;
	Sprite2D * lastFrame;
	Actor * actor;
	bool Selected;
	PathNode * path;
	PathNode * step;
	unsigned long timeStartStep;
	char * overHeadText;
	unsigned char textDisplaying;
	unsigned long timeStartDisplaying;
	GameScript * Scripts[MAX_SCRIPTS];
	bool DeleteMe;
} ActorBlock;

#endif
