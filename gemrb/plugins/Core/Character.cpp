#include "../../includes/win32def.h"
#include "Character.h"

Character::Character(void)
{
	for(int i = 0; i < MAX_STATS; i++) {
		Stats[i] = 0;
		Mods[i] = 0;
	}
	Dialog[0] = 0;
	Name[0] = 0;
	Icon[0] = 0;
}

Character::~Character(void)
{
}

/** Returns a Stat value (Base Value + Mod) */
short Character::GetStat(unsigned char StatIndex)
{
	if(StatIndex >= MAX_STATS)
		return 0xffff;
	return Stats[StatIndex] + Mods[StatIndex];
}
/** Returns a Stat Modifier */
short Character::GetMod(unsigned char StatIndex)
{
	if(StatIndex >= MAX_STATS)
		return 0xffff;
	return Mods[StatIndex];
}
/** Returns a Stat Base Value */
short Character::GetBase(unsigned char StatIndex)
{
	if(StatIndex >= MAX_STATS)
		return 0xffff;
	return Stats[StatIndex];
}
/** Sets a Stat Base Value */
bool  Character::SetBase(unsigned char StatIndex, short Value)
{
	if(StatIndex >= MAX_STATS)
		return false;
	Stats[StatIndex] = Value;
	return true;
}
/** Sets a Stat Modifier Value */
bool  Character::SetMod(unsigned char StatIndex, short Mod)
{
	if(StatIndex >= MAX_STATS)
		return false;
	Mods[StatIndex] = Mod;
	return true;
}
