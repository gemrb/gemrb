#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "Actor.h"
#include "Interface.h"

extern Interface * core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

Actor::Actor(unsigned short AnimationID)
{
	int i;

	for(i = 0; i < MAX_STATS; i++) {
		BaseStats[i] = 0;
		Modified[i] = 0;
	}
	for(i = 0; i < MAX_SCRIPTS; i++) {
		Scripts[i][0]=0;
	}
	Dialog[0] = 0;
	ScriptName[0] = 0;
	Icon[0] = 0;

	char tmp[7];
	sprintf(tmp, "0x%04X", AnimationID);
	int AvatarTable = core->LoadTable("avatars");
	TableMgr * at = core->GetTable(AvatarTable);
	int RowIndex = at->GetRowIndex(tmp);
	if(RowIndex < 0) {
		printMessage("Actor", "Avatar Animation not supported!\n", YELLOW);
		anims = NULL;
	}
	else {
		char * BaseResRef = at->QueryField(RowIndex, 0);
		char * Mirror = at->QueryField(RowIndex, 1);
		char * Orient = at->QueryField(RowIndex, 2);
		anims = new CharAnimations(BaseResRef, atoi(Orient), atoi(Mirror));
	}
	LongName = NULL;
	ShortName = NULL;
}

Actor::~Actor(void)
{
	if(anims)
		delete(anims);
	if(LongName)
		free(LongName);
	if(ShortName)
		free(ShortName);
}

CharAnimations *Actor::GetAnims()
{
  return anims;
}

/** Returns a Stat value (Base Value + Mod) */
long Actor::GetStat(unsigned int StatIndex)
{
        if(StatIndex >= MAX_STATS)
                return 0xdadadadada;
        return Modified[StatIndex];
}
bool  Actor::SetStat(unsigned int StatIndex, long Value)
{
        if(StatIndex >= MAX_STATS)
                return false;
        Modified[StatIndex] = Value;
        return true;
}
long Actor::GetMod(unsigned int StatIndex)
{
        if(StatIndex >= MAX_STATS)
                return 0xdadadadada;
        return Modified[StatIndex]-BaseStats[StatIndex];
}
/** Returns a Stat Base Value */
long Actor::GetBase(unsigned int StatIndex)
{
        if(StatIndex >= MAX_STATS)
                return 0xffff;
        return BaseStats[StatIndex];
}

/** Sets a Stat Base Value */
bool  Actor::SetBase(unsigned int StatIndex, long Value)
{
        if(StatIndex >= MAX_STATS)
                return false;
        BaseStats[StatIndex] = Value;
        return true;
}
/** call this after load, before applying effects */
void Actor::Init()
{
        memcpy(Modified,BaseStats,sizeof(Modified) );
}
/** implements a generic opcode function, modify modifier
    returns the change
*/
int Actor::NewStat(unsigned int StatIndex, long ModifierValue, long ModifierType)
{
        int oldmod=Modified[StatIndex];

        switch(ModifierType)
        {
        case 0:  //flat point modifier
                Modified[StatIndex]+=ModifierValue;
                break;
        case 1:  //straight stat change
                Modified[StatIndex]=ModifierValue;
                break;
        case 2:  //percentile
                Modified[StatIndex]=Modified[StatIndex]*100/ModifierValue;
                break;
        }
        return Modified[StatIndex]-oldmod;
}

