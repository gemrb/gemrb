/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.16 2003/12/09 19:02:42 balrog994 Exp $
 *
 */

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
	char * BaseResRef = at->QueryField(RowIndex, BaseStats[IE_ARMOR_TYPE]);
	char * Mirror = at->QueryField(RowIndex, 4);
	char * Orient = at->QueryField(RowIndex, 5);
	if(anims)
		delete(anims);	
	anims = new CharAnimations(BaseResRef, atoi(Orient), atoi(Mirror), RowIndex);

	int palType = atoi(at->QueryField(RowIndex, 4));
	
	Color Pal[256];
	memcpy(Pal, anims->Palette, 256*sizeof(Color));
	if(palType != 10) {
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
		//for(int i = 0x58; i < 0xFF; i+=0x08)
		//	memcpy(&Pal[i], &MinorPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x58], &MinorPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x60], &MajorPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x68], &MinorPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x70], &MetalPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x78], &LeatherPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x80], &LeatherPal[1], 8*sizeof(Color));
		memcpy(&Pal[0x88], &MinorPal[1], 8*sizeof(Color));
		for(int i = 0x90; i < 0xA8; i+=0x08)
			memcpy(&Pal[i], &LeatherPal[1], 8*sizeof(Color));
		memcpy(&Pal[0xB0], &SkinPal[1], 8*sizeof(Color));
		for(int i = 0xB8; i < 0xFF; i+=0x08)
			memcpy(&Pal[i], &LeatherPal[1], 8*sizeof(Color));
		free(MetalPal);
		free(MinorPal);
		free(MajorPal);
		free(SkinPal);
		free(LeatherPal);
		free(ArmorPal);
		free(HairPal);
	}
	else {
		Color * MetalPal   = core->GetPalette(7+BaseStats[IE_METAL_COLOR], 32);
		Color * MinorPal   = core->GetPalette(7+BaseStats[IE_MINOR_COLOR], 32);
		Color * MajorPal   = core->GetPalette(7+BaseStats[IE_MAJOR_COLOR], 32);
		Color * SkinPal    = core->GetPalette(7+BaseStats[IE_SKIN_COLOR], 32);
		Color * LeatherPal = core->GetPalette(7+BaseStats[IE_LEATHER_COLOR], 32);
		Color * ArmorPal   = core->GetPalette(7+BaseStats[IE_ARMOR_COLOR], 32);
		Color * HairPal    = core->GetPalette(7+BaseStats[IE_HAIR_COLOR], 32);
		for(int i = 0x10; i <= 0xF0; i+=0x10) {
			if((Pal[i].r == 0xff) && (Pal[i].g == 0x00) && (Pal[i].b == 0x00)) {
				memcpy(&Pal[i], HairPal, 32*sizeof(Color));
				i+=0x10;
			} else if((Pal[i].r == 0x00) && (Pal[i].g == 0xff) && (Pal[i].b == 0xff)) {
				memcpy(&Pal[i], MajorPal, 32*sizeof(Color));
				i+=0x10;
			} else if((Pal[i].r == 0xff) && (Pal[i].g == 0xff) && (Pal[i].b == 0x00)) {
				memcpy(&Pal[i], SkinPal, 32*sizeof(Color));
				i+=0x10;
			} else if((Pal[i].r == 0x00) && (Pal[i].g == 0x00) && (Pal[i].b == 0xff)) {
				memcpy(&Pal[i], MinorPal, 32*sizeof(Color));
				i+=0x10;
			}
		}
		/*memcpy(&Pal[0x10], &MetalPal[16],   16*sizeof(Color));
		memcpy(&Pal[0xE0], MinorPal,   32*sizeof(Color));
		memcpy(&Pal[0xA0], MajorPal,   32*sizeof(Color));
		memcpy(&Pal[0xC0], SkinPal,    32*sizeof(Color));
		memcpy(&Pal[0x80], LeatherPal, 32*sizeof(Color));
		memcpy(&Pal[0x40], LeatherPal, 16*sizeof(Color));
		memcpy(&Pal[0x40], HairPal,    16*sizeof(Color));
		*/
		free(MetalPal);
		free(MinorPal);
		free(MajorPal);
		free(SkinPal);
		free(LeatherPal);
		free(ArmorPal);
		free(HairPal);
	}

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
                return 0xdadadada;
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
                return 0xdadadada;
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

