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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.23 2004/01/29 21:40:21 avenger_teambg Exp $
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

static Color green			= {0x00, 0xff, 0x00, 0xff};
static Color red			= {0xff, 0x00, 0x00, 0xff};
static Color yellow			= {0xff, 0xff, 0x00, 0xff};
static Color cyan			= {0x00, 0xff, 0xff, 0xff};
static Color green_dark		= {0x00, 0x80, 0x00, 0xff};
static Color red_dark		= {0x80, 0x00, 0x00, 0xff};
static Color yellow_dark	= {0x80, 0x80, 0x00, 0xff};
static Color cyan_dark		= {0x00, 0x80, 0x80, 0xff};
static Color magenta		= {0xff, 0x00, 0xff, 0xff};

Actor::Actor() : Moveble(ST_ACTOR)
{
	int i;

	//memset(BaseStats, 0, MAX_STATS*sizeof(*BaseStats));
	//memset(Modified, 0, MAX_STATS*sizeof(*Modified));
	for(i = 0; i < MAX_STATS; i++) {
		BaseStats[i] = 0;
		Modified[i] = 0;
	}
	Dialog[0] = 0;
	SmallPortrait[0] = 0;
	LargePortrait[0] = 0;

	anims = NULL;

	LongName = NULL;
	ShortName = NULL;

	DeleteMe = false;
	FromGame = false;
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
	SetCircleSize();
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

void Actor::SetCircleSize()
{
	Color *color;
	if(Modified[IE_UNSELECTABLE]) {
		color=&magenta;
	}
	if(BaseStats[IE_MORALEBREAK]>=Modified[IE_MORALEBREAK]) {
		color=&yellow;
	} else {			
		switch(BaseStats[IE_EA])
			{
			case EVILCUTOFF:
			case GOODCUTOFF:
			break;

			case PC:
			case FAMILIAR:
			case ALLY:
			case CONTROLLED:
			case CHARMED:
			case EVILBUTGREEN:
				color=&green;
			break;

			case ENEMY:
			case GOODBUTRED:
				color=&red;
			break;
			default:
				color=&cyan;
			break;
			}
	}
	SetCircle(anims->CircleSize, *color);
}

bool  Actor::SetStat(unsigned int StatIndex, long Value)
{
    if(StatIndex >= MAX_STATS)
            return false;
    Modified[StatIndex] = Value;
	switch(StatIndex) {
		case IE_EA:
		case IE_UNSELECTABLE:
		case IE_MORALEBREAK:
			SetCircleSize();
		break;
	}
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
	switch(StatIndex) {
		case IE_EA:
		case IE_UNSELECTABLE:
		case IE_MORALEBREAK:
			SetCircleSize();
		break;
	}
    return true;
}
/** call this after load, before applying effects */
void Actor::Init()
{
    memcpy(Modified, BaseStats, MAX_STATS*sizeof(*Modified) );
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

void Actor::DebugDump()
{
	printf("Debugdump of Actor %s:\n", LongName);
	for(int i=0;i<MAX_SCRIPTS;i++) {
		const char *poi="<none>";
		if(Scripts[i] && Scripts[i]->script) {
			poi=Scripts[i]->script->GetName();
		}
		printf("Script %d: %s\n",i,poi);
	}
	printf("Dialog: %s\n",Dialog);
	printf("Scripting name: %s\n",scriptName);
}
