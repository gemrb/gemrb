#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "Actor.h"
#include "Interface.h"

extern Interface * core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

Actor::Actor()
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
	SmallPortrait[0] = 0;
	LargePortrait[0] = 0;

	anims = NULL;

	/*char tmp[7];
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
	}*/
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

void Actor::SetAnimationID(unsigned short AnimID)
{
	char tmp[7];
	sprintf(tmp, "0x%04X", AnimID);
	int AvatarTable = core->LoadTable("avatars");
	TableMgr * at = core->GetTable(AvatarTable);
	int RowIndex = at->GetRowIndex(tmp);
	if(RowIndex < 0) {
		anims = NULL;
		return;
	}
	char * BaseResRef = at->QueryField(RowIndex, 0);
	char * Mirror = at->QueryField(RowIndex, 1);
	char * Orient = at->QueryField(RowIndex, 2);
	anims = new CharAnimations(BaseResRef, atoi(Orient), atoi(Mirror));

	Color Pal[256];
	Pal[0].r = 0x00; Pal[0].g = 0xff; Pal[0].b = 0x00; Pal[0].a = 0x00;
	Pal[1].r = 0x00; Pal[1].g = 0x00; Pal[1].b = 0x00; Pal[1].a = 0x00;
	Pal[2].r = 0xff; Pal[2].g = 0x80; Pal[2].b = 0x80; Pal[2].a = 0x00;
	Pal[3].r = 0xff; Pal[3].g = 0x80; Pal[3].b = 0x80; Pal[3].a = 0x00;
	Color * MetalPal   = core->GetPalette(BaseStats[IE_METAL_COLOR], 12);
	Color * MinorPal   = core->GetPalette(BaseStats[IE_MINOR_COLOR], 12);
	Color * MajorPal   = core->GetPalette(BaseStats[IE_MAJOR_COLOR], 12);
	Color * SkinPal    = core->GetPalette(BaseStats[IE_SKIN_COLOR], 12);
	Color * LeatherPal = core->GetPalette(BaseStats[IE_LEATHER_COLOR], 12);
	Color * ArmorPal   = core->GetPalette(BaseStats[IE_ARMOR_COLOR], 12);
	Color * HairPal    = core->GetPalette(BaseStats[IE_HAIR_COLOR], 12);
	memcpy(&Pal[0x04], MetalPal,   12*sizeof(Color));
	memcpy(&Pal[0x10], MinorPal,   12*sizeof(Color));
	memcpy(&Pal[0x1C], MajorPal,   12*sizeof(Color));
	memcpy(&Pal[0x28], SkinPal,    12*sizeof(Color));
	memcpy(&Pal[0x34], LeatherPal, 12*sizeof(Color));
	memcpy(&Pal[0x40], ArmorPal,   12*sizeof(Color));
	memcpy(&Pal[0x4C], HairPal,    12*sizeof(Color));
	free(MetalPal);
	free(MinorPal);
	free(MajorPal);
	free(SkinPal);
	free(LeatherPal);
	free(ArmorPal);
	free(HairPal);

	/*FILE * ftmp = fopen("tmp.tmp", "wb");
	fwrite(Pal, 256, 4, ftmp);
	fclose(ftmp);*/
	
	anims->SetNewPalette(Pal);
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

