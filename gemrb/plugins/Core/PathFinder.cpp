#include "../../includes/win32def.h"
#include "PathFinder.h"
#include <stdlib.h>

#ifndef WIN32

int min(int value1, int value2)
{
	return ((value1 < value2) ? value1 : value2);
}

#endif

PathFinder::PathFinder(void)
{
	MapSet = NULL;
	ClosedSet = NULL;
}

PathFinder::~PathFinder(void)
{
	if(MapSet)
		FreeMatrices();
}

void PathFinder::SetMap(ImageMgr * sMap, int Width, int Height)
{
	if(MapSet)
		FreeMatrices();
	this->Width = Width;
	this->Height = Height;
	//Filling Matrices
	MapSet = new PathNode**[Width];
	ClosedSet = new bool*[Width];
	for(int x = 0; x < Width; x++) {
		MapSet[x] = new PathNode*[Height];
		ClosedSet[x] = new bool[Height];
		memset(ClosedSet[x], 0, Height*sizeof(bool));
		for(int y = 0; y < Height; y++) {
			PathNode * node = new PathNode();
			node->Next = NULL;
			node->Parent = NULL;
			node->x = x;
			node->y = y;
			unsigned long idx = sMap->GetPixelIndex(x,y);
			switch(idx) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 9:
				case 14:
				case 15:
					node->passable = 1;
				break;

				default:
					node->passable = 0;
			}
			MapSet[x][y] = node;
		}
	}
	this->sMap = sMap;
}

PathNode * PathFinder::FindPath(short sX, short sY, short dX, short dY)
{
	short startX = sX/16, startY = sY/12, goalX = dX/16, goalY = dY/12;
	
	OpenStack.clear();
	FinalStack.clear();
	for(int x = 0; x < Width; x++) {
		memset(ClosedSet[x], 0, Height*sizeof(bool));
	}

	OpenStack.push_back(MapSet[startX][startY]);
	MapSet[startX][startY]->Parent = NULL;
	ClosedSet[startX][startY] = true;
	while(true) {
		if(OpenStack.size() == 0) {
			printf("Cannot find a Path\n");
			return NULL;
		}
		PathNode * topNode = OpenStack.front();
		std::vector<PathNode*>::iterator m = OpenStack.begin();
		OpenStack.erase(m);
		//printf("[this=%08X, x=%d, y=%d, Parent=%08X, Next=%08X]\n", topNode, topNode->x, topNode->y, topNode->Parent, topNode->Next);
		if((topNode->x == goalX) && (topNode->y == goalY)) {//We've found _a_ path
			printf("GOAL!!!\n");
			FinalStack.push_back(topNode);
			topNode->Next = NULL;
			break;
		}
		FinalStack.push_back(topNode);
		short minX = (topNode->x == 0 ? 0 : (topNode->x-1)), maxX = ((topNode->x == (Width-1)) ? Width-1 : topNode->x+1);
		short minY = (topNode->y == 0 ? 0 : (topNode->y-1)), maxY = ((topNode->y == (Height-1)) ? Height-1 : topNode->y+1);
		
		for(int y = minY; y <= maxY; y++) {
			for(int x = minX; x <= maxX; x++) {
				if((x != topNode->x) || (y != topNode->y)) {
					if(MapSet[x][y]->passable && (!ClosedSet[x][y])) {
						MapSet[x][y]->Parent = topNode;
						MapSet[x][y]->orient = GetOrient(topNode->x, topNode->y, x, y);
						OpenStack.push_back(MapSet[x][y]);
						ClosedSet[x][y] = true;
					}
				}
			}
		}
		//Sorting the Open Stack
		std::vector<PathNode*> tmp;
		for(int i = 0; i < OpenStack.size(); i++) {
			PathNode * node = OpenStack.at(i);
			node->distance = GetDistance(node->x, node->y, goalX, goalY);
			if(tmp.size() == 0)
				tmp.push_back(node);
			else {
				bool insert = true;
				std::vector<PathNode*>::iterator m;
				for(m = tmp.begin(); m != tmp.end(); ++m) {
					if((*m)->distance > node->distance) {
						tmp.insert(m, node);
						insert = false;
						break;
					}
				}
				if(insert)
					tmp.push_back(node);
			}
		}
		//Copying tmp to OpenStack
		OpenStack.clear();
		int maxsize = min(15, tmp.size());
		for(int i = 0; i < maxsize; i++) {
			OpenStack.push_back(tmp.at(i));
		}
	}
	PathNode * ret = FinalStack.back();
	PathNode * newNode = NULL;
	while(ret && (ret->Parent != NULL)) {
		ret->Parent->Next = ret;
		PathNode * node = new PathNode();
		node->x = ret->x;
		node->y = ret->y;
		node->orient = ret->orient;
		node->passable = ret->passable;
		node->Next = newNode;
		node->Parent = NULL;
		if(newNode)
			newNode->Parent = node;
		newNode = node;
		ret = ret->Parent;
	}
	ret->orient = GetOrient(ret->x, ret->y, ret->Next->x, ret->Next->y);
	PathNode * node = new PathNode();
	node->x = ret->x;
	node->y = ret->y;
	node->orient = ret->orient;
	node->passable = ret->passable;
	node->Next = newNode;
	node->Parent = NULL;
	return node;
}

unsigned long PathFinder::GetDistance(short sX, short sY, short dX, short dY)
{
	int hd = min(abs(sX-dX), abs(sY-dY));
	int hs = (abs(sX-dX) + abs(sY-dY));
	return (10*hd)+((14*hs)-(2*hd));
}

void PathFinder::FreeMatrices()
{
	for(int x = 0; x < Width; x++) {
		for(int y = 0; y < Height; y++) {
			delete(MapSet[x][y]);
		}
		delete(MapSet[x]);
		delete(ClosedSet[x]);
	}
	delete(MapSet);
	delete(ClosedSet);
}

unsigned char PathFinder::GetOrient(short sX, short sY, short dX, short dY)
{
	short deltaX = (sX-dX), deltaY = (sY-dY);
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
