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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/CharAnimations.cpp,v 1.16 2003/12/02 19:50:14 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AnimationMgr.h"
#include "CharAnimations.h"
#include "Interface.h"

extern Interface * core;

CharAnimations::CharAnimations(char * BaseResRef, unsigned char OrientCount, unsigned char MirrorType, int RowIndex)
{
	int len = strlen(BaseResRef);
	ResRef = (char*)malloc(len+1);
	memcpy(ResRef, BaseResRef, len+1);
	this->OrientCount = OrientCount;
	this->MirrorType = MirrorType;
	LoadedFlag = 0;
	for(int i = 0; i < 18; i++) {
		for(int j = 0; j < 16; j++) {
			Anims[i][j] = NULL;
		}
	}
	ArmorType = 0;
	RangedType = 0;
	WeaponType = 0;
	this->RowIndex = RowIndex;
	Avatars = core->GetTable(core->LoadTable("avatars"));
	char * val = Avatars->QueryField(RowIndex, 24);
	if(val[0] == '*')
		UsePalette = true;
	else
		UsePalette = false;
	CircleSize = atoi(Avatars->QueryField(RowIndex, 6));
}

CharAnimations::~CharAnimations(void)
{
	free(ResRef);
}
/*This is a simple Idea of how the animation are coded

If the Orientation Count is 9 (i.e. BG2/IWD2 Avatar animations)
the 1-7 frames are mirrored to create the 9-14 frames.

There are 4 Mirroring modes:

IE_ANI_CODE_MIRROR: The code automatically mirrors the needed frames 
(as in the example above)

IE_ANI_ONE_FILE: The whole animation is in one file, no mirroring needed.

IE_ANI_TWO_FILES: The whole animation is in 2 files. The East and West part are in 2 BAM Files.

IE_ANI_FOUR_FILES: The Animation is coded in Four Files. Probably it is an old Two File animation with
additional frames added in a second time.


 WEST PART       |       EAST PART
                 |
        NW  NNW  N  NNE  NE
     NW 006 007 008 009 010 NE
    WNW 005      |      011 ENE
      W 004     xxx     012 E
    WSW 003      |      013 ESE
     SW 002 001 000 015 014 SE
	    SW  SSW  S  SSE  SE
                 |
                 |

*/
Animation * CharAnimations::GetAnimation(unsigned char AnimID, unsigned char Orient)
{
	//TODO: Implement Auto Resource Loading
	if(Anims[AnimID][Orient]) {
		if(Anims[AnimID][Orient]->ChangePalette && UsePalette) {
			Anims[AnimID][Orient]->SetPalette(Palette);
			Anims[AnimID][Orient]->ChangePalette = false;
		}
		return Anims[AnimID][Orient];
	}
	char *ResRef = (char*)malloc(9);
	strcpy(ResRef, this->ResRef);
	unsigned char Cycle;
	GetAnimResRef(AnimID, Orient, ResRef, Cycle);
	DataStream * stream = core->GetResourceMgr()->GetResource(ResRef, IE_BAM_CLASS_ID);
	printf("Loaded %s\n", ResRef);
	free(ResRef);
	AnimationMgr * anim = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	anim->Open(stream, true);
	Animation * a = anim->GetAnimation(Cycle, 0, 0, IE_NORMAL);
	switch(MirrorType) {
		case IE_ANI_CODE_MIRROR:
			{
				switch(OrientCount) {
					case 5:
						{
						if(Orient > 8)
							core->GetVideoDriver()->MirrorAnimation(a);
						if(Orient & 1)
							Orient--;
						Anims[AnimID][Orient] = a;
						Anims[AnimID][Orient+1] = a;
						}
					break;

					case 9:
						{
						if(Orient > 8)
							core->GetVideoDriver()->MirrorAnimation(a);
						Anims[AnimID][Orient] = a;
						}
					break;
				}
			}
		break;

		case IE_ANI_ONE_FILE:
			{
				Anims[AnimID][Orient] = a;
			}
		break;

		case IE_ANI_CODE_MIRROR_2:
			{
				switch(OrientCount) {
					case 9:
						{
						if(Orient > 8)
							core->GetVideoDriver()->MirrorAnimation(a);
						Anims[AnimID][Orient] = a;
						}
					break;
				}
			}
		break;

		case IE_ANI_TWO_FILES:
		case IE_ANI_TWO_FILES_2:
		case IE_ANI_TWO_FILES_3:
			{
				switch(OrientCount) {
					case 5:
						{
						if(Orient & 1)
							Orient--;
						Anims[AnimID][Orient] = a;
						Anims[AnimID][Orient+1] = a;
						}
					break;
				}
			}
		break;
	}
	if(Anims[AnimID][Orient]) {
		if(Anims[AnimID][Orient]->ChangePalette && UsePalette) {
			Anims[AnimID][Orient]->SetPalette(Palette);
			Anims[AnimID][Orient]->ChangePalette = false;
		}
	}
	return Anims[AnimID][Orient];
}

void CharAnimations::GetAnimResRef(unsigned char AnimID, unsigned char Orient, char * ResRef, unsigned char & Cycle)
{
	switch(MirrorType) 
		{
		case IE_ANI_CODE_MIRROR:
			{
				if(OrientCount == 9) {
					AddVHRSuffix(ResRef, AnimID, Cycle, Orient);
					ResRef[8] = 0;
				}
				else {
					
				}
			}
		break;

		case IE_ANI_ONE_FILE:
			{
				if(OrientCount == 16) {
					char * val = Avatars->QueryField(RowIndex, AnimID+7);
					if(val[0] == '*') {
						ResRef[0] = 0;
						return;
					}
					Cycle = atoi(val) + Orient;
				}
			}
		break;

		case IE_ANI_TWO_FILES_2:
			{
				if(OrientCount == 5) {
					AddMHRSuffix(ResRef, AnimID, Cycle, Orient);
					ResRef[8] = 0;
				}
				else {
					
				}
			}
		break;

		case IE_ANI_TWO_FILES_3:
			{
				if(OrientCount == 5) {
					AddMMRSuffix(ResRef, AnimID, Cycle, Orient);
					ResRef[8] = 0;
				}
			}
		break;

		case IE_ANI_TWO_FILES:
			{
				if(OrientCount == 5) {
					char * val = Avatars->QueryField(RowIndex, AnimID+7);
					if(val[0] == '*') {
						ResRef[0] = 0;
						return;
					}
					Cycle = atoi(val) + (Orient/2);
					switch(AnimID) {
						case IE_ANI_ATTACK:
						case IE_ANI_ATTACK_BACKSLASH:
						case IE_ANI_ATTACK_SLASH:
						case IE_ANI_ATTACK_JAB:
						case IE_ANI_CAST:
						case IE_ANI_CONJURE:
						case IE_ANI_SHOOT:
							strcat(ResRef, "G2");
						break;

						default:
							strcat(ResRef, "G1");
						break;
					}
					if(Orient > 9) {
						strcat(ResRef, "E");
					}
				}
			}
		break;

		case IE_ANI_CODE_MIRROR_2:
			{
				if(OrientCount == 9) {
					char * val = Avatars->QueryField(RowIndex, AnimID+7);
					if(val[0] == '*') {
						ResRef[0] = 0;
						return;
					}
					if(Orient > 8)
						Cycle = 7 - (Orient % 9);
					else
						Cycle = (Orient % 9);
					Cycle += atoi(val);
					switch(AnimID) {
						case IE_ANI_ATTACK_BACKSLASH:
							strcat(ResRef, "G21");
						break;

						case IE_ANI_ATTACK_SLASH:
							strcat(ResRef, "G2");
						break;

						case IE_ANI_ATTACK_JAB:
							strcat(ResRef, "G22");
						break;

						case IE_ANI_AWAKE:
							strcat(ResRef, "G12");
						break;

						case IE_ANI_DIE:
						case IE_ANI_DAMAGE:
							strcat(ResRef, "G14");
						break;

						case IE_ANI_READY:
							strcat(ResRef, "G1");
						break;

						case IE_ANI_WALK:
							strcat(ResRef, "G11");
						break;
					}
				}
			}
		break;
		}
}

void CharAnimations::AddVHRSuffix(char * ResRef, unsigned char AnimID, unsigned char &Cycle, unsigned char Orient)
	{
		switch(AnimID)
			{
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
			case IE_ANI_ATTACK:
			case IE_ANI_ATTACK_SLASH:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A1");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A4");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A7");
					break;
					}
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_ATTACK_BACKSLASH:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A2");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A5");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A8");
					break;
					}
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_ATTACK_JAB:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A3");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A6");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A9");
					break;
					}
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_AWAKE:
				{
				strcat(ResRef, "G1");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=9;
				}
			break;

			case IE_ANI_CAST:
				{
				strcat(ResRef, "CA");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=9;
				}
			break;

			case IE_ANI_CONJURE:
				{
				strcat(ResRef, "CA");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_DAMAGE:
				{
				strcat(ResRef, "G14");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=36;
				}
			break;

			case IE_ANI_DIE:
				{
				strcat(ResRef, "G15");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=45;
				}
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
			case IE_ANI_EMERGE:
				{
				strcat(ResRef, "G15");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=45;
				}
			break;

			case IE_ANI_HEAD_TURN:
				{
				strcat(ResRef, "G12");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=18;
				}
			break;

			//Unknown... maybe only a transparency effect apply
			case IE_ANI_HIDE:
				{

				}
			break;

			case IE_ANI_READY:
				{
				strcat(ResRef, "G13");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=27;
				}
			break;

			//This depends on the ranged weapon equipped
			case IE_ANI_SHOOT:
				{
				switch(RangedType)
					{
					case IE_ANI_RANGED_BOW:
						{
						strcat(ResRef, "SA");
						}
					break;

					case IE_ANI_RANGED_XBOW:
						{
						strcat(ResRef, "SX");
						}
					break;
					
					case IE_ANI_RANGED_THROW:
						{
						strcat(ResRef, "SS");
						}
					break;
					}
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_SLEEP:
				{
				strcat(ResRef, "G16");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=54;
				}
			break;

			case IE_ANI_TWITCH:
				{
				strcat(ResRef, "G17");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				Cycle+=72;
				}
			break;

			case IE_ANI_WALK:
				{
				strcat(ResRef, "G11");
				if(Orient > 8)
					Cycle = 7 - (Orient % 9);
				else
					Cycle = (Orient % 9);
				}
			break;
			}
	}

void CharAnimations::AddMHRSuffix(char * ResRef, unsigned char AnimID, unsigned char &Cycle, unsigned char Orient)
	{
		switch(AnimID)
			{
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
			case IE_ANI_ATTACK:
			case IE_ANI_ATTACK_SLASH:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A1");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A4");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A7");
					break;
					}
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_ATTACK_BACKSLASH:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A2");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A5");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A8");
					break;
					}
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_ATTACK_JAB:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A3");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A6");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A9");
					break;
					}
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_AWAKE:
				{
				strcat(ResRef, "G1");
				Cycle = 8 + (Orient/2);
				}
			break;

			case IE_ANI_CAST:
				{
				strcat(ResRef, "CA");
				Cycle = 9 + (Orient/2);
				}
			break;

			case IE_ANI_CONJURE:
				{
				strcat(ResRef, "CA");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_DAMAGE:
				{
				strcat(ResRef, "G14");
				Cycle = 40 + (Orient/2);
				}
			break;

			case IE_ANI_DIE:
				{
				strcat(ResRef, "G1");
				Cycle = 48 + (Orient/2);
				}
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
			case IE_ANI_EMERGE:
				{
				strcat(ResRef, "G1");
				Cycle = 48 + (Orient/2);
				}
			break;

			case IE_ANI_HEAD_TURN:
				{
				strcat(ResRef, "G1");
				Cycle = 16 + (Orient/2);
				}
			break;

			//Unknown... maybe only a transparency effect apply
			case IE_ANI_HIDE:
				{

				}
			break;

			case IE_ANI_READY:
				{
				strcat(ResRef, "G1");
				Cycle = 24 + (Orient/2);
				}
			break;

			//This depends on the ranged weapon equipped
			case IE_ANI_SHOOT:
				{
				switch(RangedType)
					{
					case IE_ANI_RANGED_BOW:
						{
						strcat(ResRef, "SA");
						Cycle = (Orient/2);
						}
					break;

					case IE_ANI_RANGED_XBOW:
						{
						strcat(ResRef, "SX");
						Cycle = (Orient/2);
						}
					break;
					
					case IE_ANI_RANGED_THROW:
						{
						strcat(ResRef, "A1");
						Cycle = (Orient/2);
						}
					break;
					}
				}
			break;

			case IE_ANI_SLEEP:
				{
				strcat(ResRef, "G1");
				Cycle = 64 + (Orient/2);
				}
			break;

			case IE_ANI_TWITCH:
				{
				strcat(ResRef, "G1");
				Cycle = 56 + (Orient/2);
				}
			break;

			case IE_ANI_WALK:
				{
				strcat(ResRef, "G1");
				Cycle = (Orient/2);
				}
			break;
			}
		if(Orient > 9)
			strcat(ResRef, "E");
	}

void CharAnimations::AddMMRSuffix(char * ResRef, unsigned char AnimID, unsigned char &Cycle, unsigned char Orient)
	{
		switch(AnimID)
			{
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
			case IE_ANI_ATTACK:
			case IE_ANI_ATTACK_SLASH:
			case IE_ANI_ATTACK_BACKSLASH:
				{
                strcat(ResRef, "A1");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_SHOOT:
				{
				strcat(ResRef, "A4");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_ATTACK_JAB:
				{
                strcat(ResRef, "A2");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_AWAKE:
			case IE_ANI_HEAD_TURN:
			case IE_ANI_READY:
				{
				strcat(ResRef, "SD");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_CAST:
			case IE_ANI_CONJURE:
				{
				strcat(ResRef, "SC");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_DAMAGE:
				{
				strcat(ResRef, "GH");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_DIE:
				{
				strcat(ResRef, "DE");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_EMERGE:
				{
				strcat(ResRef, "GU");
				Cycle = (Orient/2);
				}
			break;

			//Unknown... maybe only a transparency effect apply
			case IE_ANI_HIDE:
				{

				}
			break;

			case IE_ANI_SLEEP:
				{
				strcat(ResRef, "TW");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_TWITCH:
				{
				strcat(ResRef, "TW");
				Cycle = (Orient/2);
				}
			break;

			case IE_ANI_WALK:
				{
				strcat(ResRef, "WK");
				Cycle = (Orient/2);
				}
			break;
			}
		if(Orient > 9)
			strcat(ResRef, "E");
	}

