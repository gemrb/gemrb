#include "../../includes/win32def.h"
#include "Character.h"

Character::Character(void)
{
	int i;

	for(i = 0; i < MAX_STATS; i++) {
		Stats[i] = 0;
		Modified[i] = 0;
	}
	for(i = 0; i < MAX_SCRIPTS; i++) {
		Scripts[i][0]=0;
	}
	Dialog[0] = 0;
	Name[0] = 0;
	Icon[0] = 0;
}

Character::~Character(void)
{
}

/** Returns a Stat value (Base Value + Mod) */
long Character::GetStat(unsigned char StatIndex)
{
	if(StatIndex >= MAX_STATS)
		return 0xffff;
	return Modified[StatIndex];
}
long Character::GetMod(unsigned char StatIndex)
{
	if(StatIndex >= MAX_STATS)
		return 0xffff;
	return Modified[StatIndex]-BaseStats[StatIndex];
}
/** Returns a Stat Base Value */
long Character::GetBase(unsigned char StatIndex)
{
	if(StatIndex >= MAX_STATS)
		return 0xffff;
	return Stats[StatIndex];
}
/** Sets a Stat Base Value */
bool  Character::SetBase(unsigned char StatIndex, long Value)
{
	if(StatIndex >= MAX_STATS)
		return false;
	Stats[StatIndex] = Value;
	return true;
}
/** Sets a Stat Modifier Value */
bool  Character::SetMod(unsigned char StatIndex, long Mod)
{
	if(StatIndex >= MAX_STATS)
		return false;
	Modified[StatIndex] = Mod;
	return true;
}
/** call this after load, before applying effects */
int Character::Init()
{
	memcpy(Modified,Stats,sizeof(Modified) );
}
*/
/** implements a generic opcode function, modify modifier 
    returns the change
*/
int Character::NewMod(unsigned char StatIndex, long ModifierValue, long ModifierType)
{
	int oldmod=Modified[StatIndex];

	switch(ModifierType)
	{
	case 0:  //flat point modifier
		Modified[StatIndex]+=ModifierValue;
		break;
	case 1:	 //straight stat change
		Modified[StatIndex]=ModifierValue;
		break;
	case 2:  //percentile
		Modified[StatIndex]=Modified[StatIndex]*100/ModifierValue;
		break;
	}
	return Modified[StatIndex]-oldmod;
}

