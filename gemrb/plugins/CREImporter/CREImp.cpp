/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CREImporter/CREImp.cpp,v 1.9 2003/11/25 19:27:42 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "CREImp.h"
#include "../Core/Interface.h"
#include "../Core/HCAnimationSeq.h"
#include "../../includes/ie_stats.h"

CREImp::CREImp(void)
{
	str = NULL;
	autoFree = false;
}

CREImp::~CREImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool CREImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "CRE V1.0", 8) != 0) {
		if(strncmp(Signature, "CRE V1.2", 8) != 0) {
			if(strncmp(Signature, "CRE V2.2", 8) != 0) {
				if(strncmp(Signature, "CRE V9.0", 8) != 0) {
					printf("[CREImporter]: Not a CRE File or File Version not supported: %8s\n", Signature);
				}
				else
					CREVersion = IE_CRE_V9_0;
				return true;
			}
			else
				CREVersion = IE_CRE_V2_2;
			return true;
		}
		else
			CREVersion = IE_CRE_V1_2;
		return true;
	}
	CREVersion = IE_CRE_V1_0;
	return true;
}

Actor * CREImp::GetActor()
{
	int RandColor = core->LoadTable("RANDCOLR");
	TableMgr * rndcol = core->GetTable(RandColor);
	Actor * act = new Actor();
	unsigned long strref;
	str->Read(&strref, 4);
	act->LongName = core->GetString(strref);
	str->Read(&strref, 4);
	act->ShortName = core->GetString(strref);
	str->Read(&act->BaseStats[IE_MC_FLAGS], 2);
	str->Seek(2, GEM_CURRENT_POS);
	str->Read(&act->BaseStats[IE_XPVALUE], 4);
	str->Read(&act->BaseStats[IE_XP], 4);
	str->Read(&act->BaseStats[IE_GOLD], 4);
	str->Read(&act->BaseStats[IE_STATE_ID], 4);
	str->Read(&act->BaseStats[IE_HITPOINTS], 2);
	str->Read(&act->BaseStats[IE_MAXHITPOINTS], 2);
	str->Read(&act->BaseStats[IE_ANIMATION_ID], 2);
	str->Seek(2, GEM_CURRENT_POS);
	str->Read(&act->BaseStats[IE_METAL_COLOR], 1);
	str->Read(&act->BaseStats[IE_MINOR_COLOR], 1);
	str->Read(&act->BaseStats[IE_MAJOR_COLOR], 1);
	str->Read(&act->BaseStats[IE_SKIN_COLOR], 1);
	str->Read(&act->BaseStats[IE_LEATHER_COLOR], 1);
	str->Read(&act->BaseStats[IE_ARMOR_COLOR], 1);
	str->Read(&act->BaseStats[IE_HAIR_COLOR], 1);
	if(act->BaseStats[IE_METAL_COLOR] >= 200) {
		act->BaseStats[IE_METAL_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_METAL_COLOR]-200));
	}
	if(act->BaseStats[IE_MINOR_COLOR] >= 200) {
		act->BaseStats[IE_MINOR_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_MINOR_COLOR]-200));
	}
	if(act->BaseStats[IE_MAJOR_COLOR] >= 200) {
		act->BaseStats[IE_MAJOR_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_MAJOR_COLOR]-200));
	}
	if(act->BaseStats[IE_SKIN_COLOR] >= 200) {
		act->BaseStats[IE_SKIN_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_SKIN_COLOR]-200));
	}
	if(act->BaseStats[IE_LEATHER_COLOR] >= 200) {
		act->BaseStats[IE_LEATHER_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_LEATHER_COLOR]-200));
	}
	if(act->BaseStats[IE_ARMOR_COLOR] >= 200) {
		act->BaseStats[IE_ARMOR_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_ARMOR_COLOR]-200));
	}
	if(act->BaseStats[IE_HAIR_COLOR] >= 200) {
		act->BaseStats[IE_HAIR_COLOR] = atoi(rndcol->QueryField((rand()%10)+1, act->BaseStats[IE_HAIR_COLOR]-200));
	}

	/*Color Pal[256];
	Pal[0].r = 0x00; Pal[0].g = 0xff; Pal[0].b = 0x00; Pal[0].a = 0x00;
	Pal[1].r = 0x00; Pal[1].g = 0x00; Pal[1].b = 0x00; Pal[1].a = 0x00;
	Pal[2].r = 0xff; Pal[2].g = 0x80; Pal[2].b = 0x80; Pal[2].a = 0x00;
	Pal[3].r = 0xff; Pal[3].g = 0x80; Pal[3].b = 0x80; Pal[3].a = 0x00;
	Color * MetalPal   = core->GetPalette(act->BaseStats[IE_METAL_COLOR], 12);
	Color * MinorPal   = core->GetPalette(act->BaseStats[IE_MINOR_COLOR], 12);
	Color * MajorPal   = core->GetPalette(act->BaseStats[IE_MAJOR_COLOR], 12);
	Color * SkinPal    = core->GetPalette(act->BaseStats[IE_SKIN_COLOR], 12);
	Color * LeatherPal = core->GetPalette(act->BaseStats[IE_LEATHER_COLOR], 12);
	Color * ArmorPal   = core->GetPalette(act->BaseStats[IE_ARMOR_COLOR], 12);
	Color * HairPal    = core->GetPalette(act->BaseStats[IE_HAIR_COLOR], 12);
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

	if(act->anims)
		act->anims->SetNewPalette(Pal);*/

	unsigned char TotSCEFF;
	str->Read(&TotSCEFF, 1);
	str->Read(act->SmallPortrait, 8);
	act->SmallPortrait[8] = 0;
	str->Read(act->LargePortrait, 8);
	act->LargePortrait[8] = 0;
	str->Read(&act->BaseStats[IE_REPUTATION], 1);
	str->Read(&act->BaseStats[IE_HIDEINSHADOWS], 1);
	str->Read(&act->BaseStats[IE_ARMORCLASS], 2);
	str->Read(&act->Modified[IE_ARMORCLASS], 2);
	str->Read(&act->BaseStats[IE_ACCRUSHINGMOD], 2);
	str->Read(&act->BaseStats[IE_ACMISSILEMOD], 2);
	str->Read(&act->BaseStats[IE_ACPIERCINGMOD], 2);
	str->Read(&act->BaseStats[IE_ACSLASHINGMOD], 2);
	str->Read(&act->BaseStats[IE_THAC0], 1);			//Unknown in CRE V2.2
	str->Read(&act->BaseStats[IE_NUMBEROFATTACKS], 1);	//Unknown in CRE V2.2
	str->Read(&act->BaseStats[IE_SAVEVSDEATH], 1);		//Fortitude Save in V2.2
	str->Read(&act->BaseStats[IE_SAVEVSWANDS], 1);		//Reflex Save in V2.2
	str->Read(&act->BaseStats[IE_SAVEVSPOLY], 1);		//Will Save in V2.2
	str->Read(&act->BaseStats[IE_SAVEVSBREATH], 1);		//Unused in V2.2
	str->Read(&act->BaseStats[IE_SAVEVSSPELL], 1);		//Unused in V2.2
	str->Read(&act->BaseStats[IE_RESISTFIRE], 1);		
	str->Read(&act->BaseStats[IE_RESISTCOLD], 1);
	str->Read(&act->BaseStats[IE_RESISTELECTRICITY], 1);
	str->Read(&act->BaseStats[IE_RESISTACID], 1);
	str->Read(&act->BaseStats[IE_RESISTMAGIC], 1);
	str->Read(&act->BaseStats[IE_RESISTMAGICFIRE], 1);
	str->Read(&act->BaseStats[IE_RESISTMAGICCOLD], 1);
	str->Read(&act->BaseStats[IE_RESISTSLASHING], 1);
	str->Read(&act->BaseStats[IE_RESISTCRUSHING], 1);
	str->Read(&act->BaseStats[IE_RESISTPIERCING], 1);
	str->Read(&act->BaseStats[IE_RESISTMISSILE], 1);
	str->Read(&act->BaseStats[IE_DETECTILLUSIONS], 1);
	str->Read(&act->BaseStats[IE_SETTRAPS], 1);
	str->Read(&act->BaseStats[IE_LORE], 1);
	str->Read(&act->BaseStats[IE_LOCKPICKING], 1);
	str->Read(&act->BaseStats[IE_STEALTH], 1);
	str->Read(&act->BaseStats[IE_TRAPS], 1);
	str->Read(&act->BaseStats[IE_PICKPOCKET], 1);
	str->Read(&act->BaseStats[IE_FATIGUE], 1);
	str->Read(&act->BaseStats[IE_INTOXICATION], 1);
	str->Read(&act->BaseStats[IE_LUCK], 1);
	switch(CREVersion) {
		case IE_CRE_V1_0:
			{
			str->Read(&act->BaseStats[IE_PROFICIENCYLONGSWORD], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYSHORTSWORD], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYLONGBOW], 1);	//Generic Bow
			str->Read(&act->BaseStats[IE_PROFICIENCYSPEAR], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYMACE], 1);		//Blunt Weapons
			str->Read(&act->BaseStats[IE_PROFICIENCYFLAILMORNINGSTAR], 1);	//Spiked Weapons
			str->Read(&act->BaseStats[IE_PROFICIENCYAXE], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYDART], 1);		//Missile Weapons
			str->Seek(13, GEM_CURRENT_POS);
			}
		break;

		case IE_CRE_V1_2:
			{
			str->Read(&act->BaseStats[IE_PROFICIENCYMARTIALARTS], 1);	//Fist
			str->Read(&act->BaseStats[IE_PROFICIENCYLONGSWORD], 1);		//Edged
			str->Read(&act->BaseStats[IE_PROFICIENCYWARHAMMER], 1);		//Hammer
			str->Read(&act->BaseStats[IE_PROFICIENCYAXE], 1);			//Axe
			str->Read(&act->BaseStats[IE_PROFICIENCYQUARTERSTAFF], 1);	//Club
			str->Read(&act->BaseStats[IE_PROFICIENCYLONGBOW], 1);		//Bow
			str->Seek(15, GEM_CURRENT_POS);
			}
		break;

		case IE_CRE_V2_2:
			{
			
			}
		break;

		case IE_CRE_V9_0:
			{
			str->Read(&act->BaseStats[IE_PROFICIENCYLONGSWORD], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYSHORTSWORD], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYLONGBOW], 1);		//Bows
			str->Read(&act->BaseStats[IE_PROFICIENCYSPEAR], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYAXE], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYDART], 1);			//Missile
			str->Read(&act->BaseStats[IE_PROFICIENCYBASTARDSWORD], 1);  //Great Sword
			str->Read(&act->BaseStats[IE_PROFICIENCYDAGGER], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYHALBERD], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYMACE], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYFLAILMORNINGSTAR], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYWARHAMMER], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYQUARTERSTAFF], 1);
			str->Read(&act->BaseStats[IE_PROFICIENCYCROSSBOW], 1);
			str->Seek(6, GEM_CURRENT_POS);
			}
		break;

		str->Seek(0x1ED, GEM_CURRENT_POS);
		str->Read(&act->BaseStats[IE_EA], 1);
	}
	act->SetAnimationID(act->BaseStats[IE_ANIMATION_ID]);
	if(act->anims)
		act->anims->DrawCircle = false;

	return act;
}
