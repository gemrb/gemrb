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
		for(int j = 0; j < 16; j++)
			Anims[i][j] = NULL;
	}
	ArmorType = 0;
	RangedType = 0;
	WeaponType = 0;
	this->RowIndex = RowIndex;
	Avatars = core->GetTable(core->LoadTable("avatars"));
	char * val = Avatars->QueryField(RowIndex, 20);
	if(val[0] == '*')
		UsePalette = true;
	else
		UsePalette = false;
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
	if(Anims[AnimID][Orient]->ChangePalette && UsePalette) {
		Anims[AnimID][Orient]->SetPalette(Palette);
		Anims[AnimID][Orient]->ChangePalette = false;
	}
	return Anims[AnimID][Orient];
}

void CharAnimations::AddVHRSuffix(char * ResRef, unsigned char AnimID, unsigned char &Cycle, unsigned char Orient)
	{
		switch(ArmorType)
			{
			case 0: // No Armor
				{
				if(stricmp(core->GameType, "bg2") == 0) {
					if(ResRef[3] != 'W')
						ResRef[3] = 'B';
				}
				else {
					if((ResRef[3] != 'T') && (ResRef[3] != 'W'))
						ResRef[3] = 'B';
				}
				strcat(ResRef, "1");
				}
			break;

			case 1: // Light Armor
				{
				if((ResRef[3] != 'T') && (ResRef[3] != 'W'))
					ResRef[3] = 'B';
				strcat(ResRef, "2");
				}
			break;

			case 2: // Medium Armor
				{
				if((ResRef[3] != 'T') && (ResRef[3] != 'W'))
					ResRef[3] = 'B';
				strcat(ResRef, "3");
				}
			break;

			case 3: // Heavy Armor
				{
				strcat(ResRef, "4");
				}
			break;
			}
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
						strcat(ResRef, "A3");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A7");
					break;
					}
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
						strcat(ResRef, "A4");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A8");
					break;
					}
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
						strcat(ResRef, "A5");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A9");
					break;
					}
				Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_AWAKE:
				{
				strcat(ResRef, "G1");
				Cycle = 9 + (Orient % 9);
				}
			break;

			case IE_ANI_CAST:
				{
				strcat(ResRef, "CA");
				Cycle = 9 + (Orient % 9);
				}
			break;

			case IE_ANI_CONJURE:
				{
				strcat(ResRef, "CA");
				Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_DAMAGE:
				{
				strcat(ResRef, "G14");
				Cycle = 36 + (Orient % 9);
				}
			break;

			case IE_ANI_DIE:
				{
				strcat(ResRef, "G15");
				Cycle = 45 + (Orient % 9);
				}
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
			case IE_ANI_EMERGE:
				{
				strcat(ResRef, "G15");
				Cycle = 45 + (Orient % 9);
				}
			break;

			case IE_ANI_HEAD_TURN:
				{
				strcat(ResRef, "G12");
				Cycle = 18 + (Orient % 9);
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
				Cycle = 27 + (Orient % 9);
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
						Cycle = (Orient % 9);
						}
					break;

					case IE_ANI_RANGED_XBOW:
						{
						strcat(ResRef, "SX");
						Cycle = (Orient % 9);
						}
					break;
					
					case IE_ANI_RANGED_THROW:
						{
						strcat(ResRef, "SS");
						Cycle = (Orient % 9);
						}
					break;
					}
				}
			break;

			case IE_ANI_SLEEP:
				{
				strcat(ResRef, "G16");
				Cycle = 54 + (Orient % 9);
				}
			break;

			case IE_ANI_TWITCH:
				{
				strcat(ResRef, "G17");
				Cycle = 72 + (Orient % 9);
				}
			break;

			case IE_ANI_WALK:
				{
				strcat(ResRef, "G11");
				Cycle = (Orient % 9);
				}
			break;
			}
	}
