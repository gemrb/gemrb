#include "../../includes/win32def.h"
#include "PathFinder.h"
#include <stdlib.h>


#define DiagonalCost	14
#define NormalCost		10

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
	OpenStack = new PathNode*[Width*Height];
	OpenStackCount = 0;
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
	
	OpenStackCount=0;
	FinalStack.clear();
	for(int x = 0; x < Width; x++) {
		memset(ClosedSet[x], 0, Height*sizeof(bool));
	}

	HeapAdd(MapSet[startX][startY]);
	MapSet[startX][startY]->Parent = NULL;
	MapSet[startX][startY]->distance = GetDistance(startX, startY, goalX, goalY);
	MapSet[startX][startY]->cost = 0;
	ClosedSet[startX][startY] = true;
	while(true) {
		PathNode * topNode = HeapRemove();
		if(!topNode) {
			printf("Cannot find a Path\n");
			return NULL;
		}
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
						MapSet[x][y]->cost = topNode->cost;
						if(x == topNode->x) {
							MapSet[x][y]->cost += NormalCost;
						}
						else if(x > topNode->x) {
							if(y == topNode->y)
								MapSet[x][y]->cost += NormalCost;
							else
								MapSet[x][y]->cost += DiagonalCost;
						}
						else {
							if(y == topNode->y)
								MapSet[x][y]->cost += NormalCost;
							else
								MapSet[x][y]->cost += DiagonalCost;
						}
						MapSet[x][y]->Parent = topNode;
						MapSet[x][y]->orient = GetOrient(topNode->x, topNode->y, x, y);
						MapSet[x][y]->distance = MapSet[x][y]->cost+GetDistance(x, y, goalX, goalY);
						HeapAdd(MapSet[x][y]);
						ClosedSet[x][y] = true;
					}
				}
			}
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
	return (NormalCost*hd)+((DiagonalCost*hs)-(2*hd));
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
	delete(OpenStack);
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

PathNode * PathFinder::HeapRemove()
{
	if(OpenStackCount==0)
		return NULL;
	PathNode * ret = OpenStack[0];
	PathNode * node = OpenStack[0] = OpenStack[OpenStackCount-1];
	OpenStackCount--;
	int lastPos = 1;
	while(true) {
		int leftChildPos = (lastPos*2)-1;
		int rightChildPos = lastPos*2;
		if(leftChildPos >= OpenStackCount)
			break;
		PathNode * child  = OpenStack[leftChildPos];
		int childPos = leftChildPos;
		if(rightChildPos < OpenStackCount) { //If both Child Exist
			PathNode * rightChild = OpenStack[lastPos*2];
			if(rightChild->distance < child->distance) {
				childPos = rightChildPos;
				child = rightChild;
			}
			
		}
		if(node->distance > child->distance) {
			OpenStack[lastPos-1] = child;
			OpenStack[childPos] = node;
			lastPos = childPos+1;
		}
		else
			break;
	}
	return ret;
}

void PathFinder::HeapAdd(PathNode * node)
{
	OpenStackCount++;
	OpenStack[OpenStackCount-1] = node;
	int lastPos = OpenStackCount;
	while(lastPos != 1) {
		int parentPos = (lastPos/2)-1;
		PathNode * parent = OpenStack[parentPos];
		if(node->distance < parent->distance) {
			OpenStack[parentPos] = node;
			OpenStack[lastPos-1] = parent;
			lastPos = parentPos+1;
		}
		else
			break;
	}
}
