#include "../../includes/win32def.h"
#include "Game.h"

Game::Game(void)
{
}

Game::~Game(void)
{
	for(int i = 0; i < PCs.size(); i++) {
		delete(PCs[i].actor);
	}
	for(int i = 0; i < Maps.size(); i++) {
		delete(Maps[i]);
	}
}

ActorBlock Game::GetPC(int slot)
{
	ActorBlock ab;
	ab.actor = NULL;
	if(slot >= PCs.size())
		return ab;
	return PCs[slot];
}
int Game::SetPC(ActorBlock pc)
{
	PCs.push_back(pc);
	return PCs.size()-1;
}
int Game::DelPC(int slot, bool autoFree)
{
	if(slot >= PCs.size())
		return -1;
	if(!PCs[slot].actor)
		return -1;
	if(autoFree)
		delete(PCs[slot].actor);
	PCs[slot].actor = NULL;
	return 0;
}
Map * Game::GetMap(int index)
{
	if(index >= Maps.size())
		return NULL;
	return Maps[index];
}
int Game::AddMap(Map* map)
{
	for(int i = 0; i < Maps.size(); i++) {
		if(!Maps[i]) {
			Maps[i] = map;
			return i;
		}
	}
	Maps.push_back(map);
	return Maps.size()-1;
}
int Game::DelMap(int index, bool autoFree)
{
	if(index >= Maps.size())
		return -1;
	if(!Maps[index])
		return -1;
	if(autoFree)
		delete(Maps[index]);
	Maps[index] = NULL;
	return 0;
}
