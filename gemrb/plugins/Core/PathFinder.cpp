#include "../../includes/win32def.h"
#include "PathFinder.h"
#include <stdlib.h>

#define NormalCost	10
#define AdditionalCost	4

PathFinder::PathFinder(void)
{
	MapSet = NULL;
	sMap = NULL;
}

PathFinder::~PathFinder(void)
{
	if(MapSet)
		delete [] MapSet;
}

void PathFinder::SetMap(ImageMgr * sMap, unsigned int Width, unsigned int Height)
{
	if(MapSet)
		delete [] MapSet;
	this->Width = Width;
	this->Height = Height;
	//Filling Matrices
	MapSet = new unsigned short[Width*Height];
	this->sMap = sMap;
}

void PathFinder::Leveldown(unsigned int px, unsigned int py, unsigned int &level, unsigned int &nx, unsigned int &ny, unsigned int &diff)
{
  int pos;
  unsigned int nlevel;

  if((px>=Width) || (py>=Height)) return; //walked off the map
  pos=py*Width+px;
  nlevel=MapSet[pos];
  if(!nlevel) return; //not even considered
  if(level<=nlevel) return;
  unsigned int ndiff=level-nlevel;
  if(ndiff>diff)
  {
    level=nlevel;
    diff=ndiff;
    nx=px;
    ny=py;
  }
}

static int Passable[16]={0,1,1,1,1,1,1,1,0,1,0,0,0,0,1,1};

void PathFinder::SetupNode(unsigned int x,unsigned int y, int Cost)
{
	unsigned int pos;

        if((x>=Width) || (y>=Height))
                return;
	pos=y*Width+x;
	if(MapSet[pos])
		return;
        if(!Passable[sMap->GetPixelIndex(x,y)])
	{
		MapSet[pos]=65535;
                return;
	}
        MapSet[pos] = Cost;
	InternalStack.push((x<<16) |y);
}

PathNode * PathFinder::FindPath(short sX, short sY, short dX, short dY)
{
	unsigned int startX = sX/16, startY = sY/12, goalX = dX/16, goalY = dY/12;
	memset(MapSet, 0, Width*Height*sizeof(unsigned short) );
	while(InternalStack.size())
		InternalStack.pop();

	unsigned int pos=(goalX<<16)|goalY;
	unsigned int pos2=(startX<<16)|startY;
	InternalStack.push(pos);
	MapSet[goalY*Width+goalX] = 1;
	
	while(InternalStack.size()) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;

		if(pos==pos2) {//We've found _a_ path
			printf("GOAL!!!\n");
			break;
		}
		int Cost = MapSet[y*Width+x]+NormalCost;
		if(Cost>65500) {
			printf("Path not found!\n");
			break;
		}
		SetupNode(x-1, y-1, Cost);
		SetupNode(x+1, y-1, Cost);
		SetupNode(x+1, y+1, Cost);
		SetupNode(x-1, y+1, Cost);

		Cost += AdditionalCost;
		SetupNode(x, y-1, Cost);
		SetupNode(x+1, y, Cost);
		SetupNode(x, y+1, Cost);
		SetupNode(x-1, y, Cost);
	}
	PathNode *StartNode = new PathNode;
	PathNode *Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
	StartNode->x=startX;
	StartNode->y=startY;
	StartNode->orient=GetOrient(goalX, goalY, startX, startY);
	if(pos!=pos2)
		return Return;
	unsigned int px = startX;
	unsigned int py = startY;
	pos2 = goalY * Width + goalX;
	while( (pos=py*Width+px) !=pos2) {
		StartNode->Next = new PathNode;
		StartNode->Next->Parent = StartNode;
		StartNode=StartNode->Next;
    		StartNode->Next=NULL;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		unsigned int nx, ny;
		Leveldown(px,py+1,level,nx,ny, diff);
		Leveldown(px+1,py,level,nx,ny, diff);
		Leveldown(px-1,py,level,nx,ny, diff);
		Leveldown(px,py-1,level,nx,ny, diff);
		Leveldown(px-1,py+1,level,nx,ny, diff);
		Leveldown(px+1,py+1,level,nx,ny, diff);
		Leveldown(px+1,py-1,level,nx,ny, diff);
		Leveldown(px-1,py-1,level,nx,ny, diff);
		if(!diff) abort();
    		StartNode->x=nx;
		StartNode->y=ny;
		StartNode->orient=GetOrient(nx,ny,px,py);
		px=nx;
		py=ny;
	}
	return Return;
}

unsigned char PathFinder::GetOrient(short sX, short sY, short dX, short dY)
{
	short deltaX = (dX-sX), deltaY = (dY-sY);
	if(deltaX > 0) {
		if(deltaY > 0) {
			return 6;
		} else if(deltaY == 0) {
			return 4;
		} else {
			return 2;
		}
	}
	else if(deltaX == 0) {
		if(deltaY > 0) {
			return 8;
		} else {
			return 0;
		}
	}
	else {
		if(deltaY > 0) {
			return 10;
		} else if(deltaY == 0) {
			return 12;
		} else {
			return 14;
		}
	}
	return 0;
}
