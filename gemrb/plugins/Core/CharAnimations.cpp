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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/CharAnimations.cpp,v 1.48 2005/03/18 18:10:12 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AnimationMgr.h"
#include "CharAnimations.h"
#include "Interface.h"

static int AvatarsCount = 0;
static AvatarStruct *AvatarTable = NULL;
static int SixteenToNine[16]={0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1};
static int SixteenToFive[16]={0,0,1,1,2,2,3,3,4,4,3,3,2,2,1,1};

void CharAnimations::ReleaseMemory()
{
	if (AvatarTable) {
		free(AvatarTable);
		AvatarTable=NULL;
	}
}

int CharAnimations::GetAvatarsCount()
{
	return AvatarsCount;
}

AvatarStruct *CharAnimations::GetAvatarStruct(int RowNum)
{
	return AvatarTable+RowNum;
}

int CharAnimations::GetCircleSize() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].CircleSize;
}
int CharAnimations::NoPalette() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].PaletteType;
}
int CharAnimations::GetAnimType() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].AnimationType;
}

void CharAnimations::SetArmourLevel(int ArmourLevel)
{
	if (AvatarsRowNum==~0u) return;
	strncpy( ResRef, AvatarTable[AvatarsRowNum].Prefixes[ArmourLevel], 8 );
	ResRef[8]=0;
}

void CharAnimations::SetupColors(ieDword *arg)
{
	if(arg) {
		for (int i = 0; i < MAX_ANIMS; i++) {
			for (int j = 0; j < MAX_ORIENT; j++) {
				if (Anims[i][j]) {
					Anims[i][j]->ChangePalette = true;
				}
			}
		}
		Colors = arg;
	}
	if(!Colors || !Palette) {
		return;
	}
        if (GetAnimType() >=IE_ANI_PST_ANIMATION_1) {
                // Avatars in PS:T
                int size = 32;
                int dest = 256-Colors[6]*size;
                for (unsigned int i = 0; i < Colors[6]; i++) {
                        Color* NewPal = core->GetPalette( Colors[i], size );
                        memcpy( &Palette[dest], NewPal, size * sizeof( Color ) );
                        dest +=size;
                        free( NewPal );
                }
                return;
        }

	int PType = NoPalette();
        if( PType) {
		//handling special palettes like MBER_BL (black bear)
		if (PType==1) 
			return;
		char PaletteResRef[8];
		sprintf(PaletteResRef, "%.4s_%-.2s",ResRef, (char *) &PType);
		ImageMgr *bmp = (ImageMgr *) core->GetInterface( IE_BMP_CLASS_ID);
		if (bmp) {
			DataStream* s = core->GetResourceMgr()->GetResource( PaletteResRef, IE_BMP_CLASS_ID );
			bmp->Open( s, true);
			bmp->GetPalette(0, 256, Palette);
			core->FreeInterface( bmp );
		}
                return;
        }
        Color* MetalPal = core->GetPalette( Colors[0], 12 );
        Color* MinorPal = core->GetPalette( Colors[1], 12 );
        Color* MajorPal = core->GetPalette( Colors[2], 12 );
        Color* SkinPal = core->GetPalette( Colors[3], 12 );
        Color* LeatherPal = core->GetPalette( Colors[4], 12 );
        Color* ArmorPal = core->GetPalette( Colors[5], 12 );
        Color* HairPal = core->GetPalette( Colors[6], 12 );
        memcpy( &Palette[0x04], MetalPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x10], MinorPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x1C], MajorPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x28], SkinPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x34], LeatherPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x40], ArmorPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x4C], HairPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x58], &MinorPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x60], &MajorPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x68], &MinorPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x70], &MetalPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x78], &LeatherPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x80], &LeatherPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x88], &MinorPal[1], 8 * sizeof( Color ) );

        int i; //moved here to be compatible with msvc6.0

        for (i = 0x90; i < 0xA8; i += 0x08)
                memcpy( &Palette[i], &LeatherPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0xB0], &SkinPal[1], 8 * sizeof( Color ) );
        for (i = 0xB8; i < 0xFF; i += 0x08)
                memcpy( &Palette[i], &LeatherPal[1], 8 * sizeof( Color ) );
        free( MetalPal );
        free( MinorPal );
        free( MajorPal );
        free( SkinPal );
        free( LeatherPal );
        free( ArmorPal );
        free( HairPal );
}

void CharAnimations::InitAvatarsTable()
{
	int tmp= core->LoadTable( "avatars" );
	if (tmp<0) {
		printMessage("CharAnimations", "A critical animation file is missing!\n", LIGHT_RED);
		abort();
	}
	TableMgr *Avatars = core->GetTable( tmp );
	AvatarTable = (AvatarStruct *) calloc ( AvatarsCount = Avatars->GetRowCount(), sizeof(AvatarStruct) );
	int i=AvatarsCount;
	while(i--) {
		AvatarTable[i].AnimID=(unsigned int) strtol(Avatars->GetRowName(i),NULL,0 );
		strncpy(AvatarTable[i].Prefixes[0],Avatars->QueryField(i,AV_PREFIX1),8);
		strncpy(AvatarTable[i].Prefixes[1],Avatars->QueryField(i,AV_PREFIX2),8);
		strncpy(AvatarTable[i].Prefixes[2],Avatars->QueryField(i,AV_PREFIX3),8);
		strncpy(AvatarTable[i].Prefixes[3],Avatars->QueryField(i,AV_PREFIX4),8);
		AvatarTable[i].AnimationType=atoi(Avatars->QueryField(i,AV_ANIMTYPE) );
		AvatarTable[i].CircleSize=atoi(Avatars->QueryField(i,AV_CIRCLESIZE) );
		char *tmp = Avatars->QueryField(i,AV_USE_PALETTE);
		//QueryField will always return a zero terminated string
		//so tmp[0] must exist
		if( isalpha (tmp[0]) ) {
			//this is a hack, we store 2 letters on an integer
			//it was allocated with calloc, so don't bother erasing it
			strncpy( (char *) &AvatarTable[i].PaletteType, tmp, 3);
		}
		else {
			AvatarTable[i].PaletteType=atoi(Avatars->QueryField(i,AV_USE_PALETTE) );
		}
	}
	core->DelTable( tmp );
}

CharAnimations::CharAnimations(unsigned int AnimID, ieDword ArmourLevel)
{
	Palette = NULL;
	Colors = NULL;
	if (!AvatarsCount) {
		InitAvatarsTable();
	}

	for (int i = 0; i < MAX_ANIMS; i++) {
		for (int j = 0; j < MAX_ORIENT; j++) {
			Anims[i][j] = NULL;
		}
	}
	ArmorType = 0;
	RangedType = 0;
	WeaponType = 0;
	AvatarsRowNum=AvatarsCount;
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		AnimID&=0xff;
	}

	while (AvatarsRowNum--) {
		if (AvatarTable[AvatarsRowNum].AnimID==AnimID) {
			SetArmourLevel( ArmourLevel );
			return;
		}
	}
	char tmp[256];
	sprintf(tmp, "Invalid or nonexistent avatar entry:%04X\n", AnimID);
	printMessage("CharAnimations",tmp, LIGHT_RED);
}

//freeing the bitmaps only once, but using an intelligent algorithm
CharAnimations::~CharAnimations(void)
{
	void *tmppoi;

	if (Palette) {
		core->GetVideoDriver()->FreePalette(Palette);
	}
	for (int StanceID = 0; StanceID < MAX_ANIMS; StanceID++) {
		for (int i = 0; i < MAX_ORIENT; i++) {
			if (Anims[StanceID][i]) {
				tmppoi = Anims[StanceID][i];
				delete( Anims[StanceID][i] );
				for (int IDb=StanceID;IDb < MAX_ANIMS; IDb++) {
					for (int j = i; j<MAX_ORIENT; j++) {
						if (Anims[IDb][j]==tmppoi) {
							Anims[IDb][j]=NULL;
						}
					}
				}
			}
		}
	}
}
/*This is a simple Idea of how the animation are coded

There are the following animation types:

IE_ANI_CODE_MIRROR: The code automatically mirrors the needed frames 
			(as in the example above)

			These Animations are stores using the following template:
			[NAME][ARMORTYPE][ACTIONCODE]

			Each BAM File contains only 9 Orientations, the missing 7 Animations
			are created by Horizontally Mirroring the 1-7 Orientations.

IE_ANI_ONE_FILE:	The whole animation is in one file, no mirroring needed.
			Each animation group is 16 Cycles.

IE_ANI_TWO_FILES:	The whole animation is in 2 files. The East and West part are in 2 BAM Files.
			Example:
			ACHKG1
			ACHKG1E

			Each BAM File contains many animation groups, each animation group
			stores 5 Orientations, the missing 3 are stored in East BAM Files.


IE_ANI_FOUR_FILES:	The Animation is coded in Four Files. Probably it is an old Two File animation with
			additional frames added in a second time.

IE_ANI_TWENTYTWO:	This Animation Type stores the Animation in the following format
			[NAME][ACTIONCODE][/E]
			ACTIONCODE=A1-6, CA, SX, SA (sling is A1)
			The G1 file contains several animation states. See MHR
			Probably it could use A7-9 files too, bringing the file numbers to 28.

IE_ANI_SIX_FILES:	The layout for these files is:
			[NAME][G1-3][/E]
			Each state contains 16 Orientations, but the last 6 are stored in the East file.
			G1 contains only the walking animation.
			G2 contains stand, ready, get hit, die and twitch.
			G3 contains 3 attacks.

IE_ANI_TWO_FILES_3:	Animations using this type was stored using the following template:
			[NAME][ACTIONTYPE][/E]

			Example:
			MBFI*
			MBFI*E

			Each BAM File contains one animation group, each animation group
			stores 5 Orientations though the files contain all 8 Cycles, the missing 3 are stored in East BAM Files in Cycle: Stance*8+ (5,6,7).
			This is the standard IWD animation, but BG2 also has it.
			See MMR

IE_ANI_PST_ANIMATION_1:
IE_ANI_PST_ANIMATION_2: Planescape: Torment Animations are stored in a different
			way than the other games. This format uses the following template:
			[C/D][ACTIONTYPE][NAME][B]

			Example:
			CAT1MRTB

			Each Animation stores 5 Orientations, which are automatically mirrored
			to form an 8 Orientation Animation. PST Animations have a different Palette
			format. This Animation Type handles the PST Palette format too.

			NOTE: Walking/Running animations store 9 Orientations.
			The second variation is missing the resting stance (STD) and the transitions.
			These creatures are always in combat stance (don't rest).

IE_ANI_PST_STAND:	This is a modified PST animation, it contains only a
			Standing image for every orientations, it follows the
			[C/D]STD[NAME][B] standard.

IE_ANI_PST_GHOST:	This is a special animation with no standard.


	 WEST PART  	 |  	 EAST PART
			 |
		NW  NNW  N  NNE  NE
	 NW 006 007 008 009 010 NE
	WNW 005 	 |  	011 ENE
	  W 004 	xxx 	012 E
	WSW 003 	 |  	013 ESE
	 SW 002 001 000 015 014 SE
		SW  SSW  S  SSE  SE
			 |
			 |

*/
Animation* CharAnimations::GetAnimation(unsigned char StanceID, unsigned char Orient)
{
	if (StanceID>=MAX_ANIMS) {
		printf("Illegal stance ID\n");
		abort();
	}

	int AnimType = GetAnimType();

	//alter stance here if it is missing and you know a substitute
	//probably we should feed this result back to the actor?
	switch (AnimType) {
		case IE_ANI_PST_STAND:
		case IE_ANI_PST_GHOST:
			StanceID=IE_ANI_AWAKE;
			break;
		case IE_ANI_PST_ANIMATION_2: //std->stc
			if (StanceID==IE_ANI_AWAKE) {
				StanceID=IE_ANI_READY;
				break;
			}
			//fallthrough
		case IE_ANI_PST_ANIMATION_1:
			//pst animations don't twitch on death
			if (StanceID==IE_ANI_DIE) {
				StanceID=IE_ANI_SLEEP;
			}
			break;
	}

	//TODO: Implement Auto Resource Loading
	if (Anims[StanceID][Orient]) {
		if (Anims[StanceID][Orient]->ChangePalette) {
			Anims[StanceID][Orient]->SetPalette( Palette );
			Anims[StanceID][Orient]->ChangePalette = false;
		}
		return Anims[StanceID][Orient];
	}
	//newresref is based on the prefix (ResRef) and various other things
	char NewResRef[12]; //this is longer than expected so it won't overflow
	strncpy( NewResRef, ResRef, 8 ); //we need this long for special anims
	unsigned char Cycle;
	GetAnimResRef( StanceID, Orient, NewResRef, Cycle );
	NewResRef[8]=0; //cutting right to size
	DataStream* stream = core->GetResourceMgr()->GetResource( NewResRef,
		IE_BAM_CLASS_ID );
	AnimationMgr* animgr = ( AnimationMgr* )
		core->GetInterface( IE_BAM_CLASS_ID );
	animgr->Open( stream, true );
	Animation* a = animgr->GetAnimation( Cycle, 0, 0, IE_NORMAL );
	core->FreeInterface( animgr );
	if (!a) {
		return NULL;
	}
	if (Palette) {
		core->GetVideoDriver()->FreePalette(Palette);
	}
	Palette = core->GetVideoDriver()->GetPalette( a->GetFrame( 0 ) );
	a->pos = 0;
	a->endReached = false;

	//setting up the sequencing of animation cycles
	switch (StanceID) {
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			a->playOnce = true;
			break;
		case IE_ANI_DIE:
printf("Die switch to twitch\n");
			a->nextStanceID = IE_ANI_TWITCH;
			a->autoSwitchOnEnd = true;
			break;
		case IE_ANI_WALK:
		case IE_ANI_RUN:
		case IE_ANI_CAST: //IE_ANI_CONJURE is the ending casting anim
		case IE_ANI_READY:
			break;
		case IE_ANI_AWAKE:
			break;
		default:
			a->nextStanceID = IE_ANI_AWAKE;
			a->autoSwitchOnEnd = true;
			break;
	}
	switch (GetAnimType()) {
		case IE_ANI_CODE_MIRROR_3: //bird animations
			if (Orient > 8) {
				a->MirrorAnimation( );
			}
			Anims[StanceID][Orient] = a;
			break;

		case IE_ANI_CODE_MIRROR:
			if (Orient > 8) {
				a->MirrorAnimation( );
			}
			Anims[StanceID][Orient] = a;
			if ((StanceID == IE_ANI_EMERGE) ||
				(StanceID == IE_ANI_GET_UP)) {
				a->playReversed = true;
			}
			break;

		case IE_ANI_SIX_FILES: //16 anims some are stored elsewhere
		case IE_ANI_ONE_FILE: //16 orientations
			Anims[StanceID][Orient] = a;
			break;

		case IE_ANI_CODE_MIRROR_2: //9 orientations
			if (Orient > 8) {
				a->MirrorAnimation( );
			}
			Anims[StanceID][Orient] = a;
			break;

		case IE_ANI_TWO_FILES:
		case IE_ANI_TWENTYTWO:
		case IE_ANI_TWO_FILES_3:
		case IE_ANI_FOUR_FILES:
			Orient&=~1;
			Anims[StanceID][Orient] = a;
			Anims[StanceID][Orient + 1] = a;
			break;

		case IE_ANI_PST_ANIMATION_2:  //no std just stc
		case IE_ANI_PST_ANIMATION_1:
			if (Orient > 8) {
				a->MirrorAnimation( );
			}
			switch (StanceID) {
				case IE_ANI_WALK:
				case IE_ANI_RUN:
				case IE_ANI_PST_START:
					Anims[StanceID][Orient] = a;
					break;
				default:
					Orient &=~1;
					Anims[StanceID][Orient] = a;
					Anims[StanceID][Orient + 1] = a;
					break;
			}
			break;

		case IE_ANI_PST_STAND:
		case IE_ANI_PST_GHOST:
			Orient &=~1;
			Anims[StanceID][Orient] = a;
			Anims[StanceID][Orient+1] = a;
			break;
		default:
			printMessage("CharAnimations","Unknown animation type\n",LIGHT_RED);
			abort();
	}

	if (Anims[StanceID][Orient]) {
		if (Anims[StanceID][Orient]->ChangePalette) {
			SetupColors(NULL); //we should have got the colors already
			Anims[StanceID][Orient]->SetPalette( Palette );
			Anims[StanceID][Orient]->ChangePalette = false;
		}
	}
	return Anims[StanceID][Orient];
}

void CharAnimations::GetAnimResRef(unsigned char StanceID, unsigned char Orient,
	char* ResRef, unsigned char& Cycle)
{
	char tmp[256];

	Orient &= 15;
	switch (GetAnimType()) {
		case IE_ANI_CODE_MIRROR:
			AddVHRSuffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_CODE_MIRROR_3:
			Cycle = StanceID * 9 + SixteenToNine[Orient];
			break;

		case IE_ANI_ONE_FILE:
			Cycle = StanceID * 16 + Orient;
			break;
		case IE_ANI_SIX_FILES:
			AddSixSuffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWENTYTWO:  //5+3 animations
			AddMHRSuffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES_3: //IWD style anims
			AddMMRSuffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES: 
			//we have to fix this
			Cycle = StanceID * 8 + Orient / 2;
			strcat( ResRef, "G1" );
			if (Orient > 9) {
				strcat( ResRef, "E" );
			}
			break;

		case IE_ANI_FOUR_FILES:
			AddLRSuffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_CODE_MIRROR_2: //9 orientations
			AddVHR2Suffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_PST_ANIMATION_1:
		case IE_ANI_PST_ANIMATION_2:
			AddPSTSuffix( ResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_PST_STAND:
			sprintf(ResRef,"%cSTD%4s",this->ResRef[0], this->ResRef+1);
			Cycle = SixteenToFive[Orient];
			break;
		case IE_ANI_PST_GHOST: // pst static animations
			Cycle = SixteenToFive[Orient];
			break;
		default:
			sprintf (tmp,"Unknown animation type in avatars.2da row: %d\n", AvatarsRowNum);
			printMessage ("CharAnimations",tmp, LIGHT_RED);
			abort();
	}
}

void CharAnimations::AddPSTSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	char *Prefix;

	if ((StanceID==IE_ANI_WALK) || (StanceID==IE_ANI_RUN)) {
		Cycle=SixteenToNine[Orient];
	} else if (StanceID==IE_ANI_PST_START) Cycle=0;
	else {
		Cycle=SixteenToFive[Orient];
	}
	switch (StanceID) {
		case IE_ANI_ATTACK:
			Prefix="AT1"; break;
		case IE_ANI_DAMAGE:
			Prefix="HIT"; break;
		case IE_ANI_GET_UP:
			Prefix="GUP"; break;
		case IE_ANI_AWAKE:
			Prefix="STD"; break;
		case IE_ANI_READY:
			Prefix="STC"; break;
		case IE_ANI_DIE:
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Prefix="DFB"; break;
		case IE_ANI_RUN:
			Prefix="RUN"; break;
		case IE_ANI_WALK:
			Prefix="WLK"; break;
		case IE_ANI_PST_START:
			Prefix="MS1"; break;
		default: //just in case
			Prefix="STC"; break;
	}
	sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
}

void CharAnimations::AddVHR2Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "G21" );
			break;

		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "G2" );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "G26" );
			Cycle+=45;
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "G12" );
			Cycle+=18;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "G14" );
			Cycle+=45;
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "G14" );
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "G14" );
			Cycle+=27;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "G1" );
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "G11" );
			break;
	}
}

void CharAnimations::AddVHRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Cycle = SixteenToNine[Orient];
	switch (StanceID) {
		//Attack is a special case... it takes cycles randomly
		//based on the weapon type (TODO)
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			switch (WeaponType) {
				case IE_ANI_WEAPON_1H:
					strcat( ResRef, "A1" );
					break;

				case IE_ANI_WEAPON_2H:
					strcat( ResRef, "A4" );
					break;

				case IE_ANI_WEAPON_2W:
					strcat( ResRef, "A7" );
					break;
			}
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			switch (WeaponType) {
				case IE_ANI_WEAPON_1H:
					strcat( ResRef, "A2" );
					break;

				case IE_ANI_WEAPON_2H:
					strcat( ResRef, "A5" );
					break;

				case IE_ANI_WEAPON_2W:
					strcat( ResRef, "A8" );
					break;
			}
			break;

		case IE_ANI_ATTACK_JAB:
			switch (WeaponType) {
				case IE_ANI_WEAPON_1H:
					strcat( ResRef, "A3" );
					break;

				case IE_ANI_WEAPON_2H:
					strcat( ResRef, "A6" );
					break;

				case IE_ANI_WEAPON_2W:
					strcat( ResRef, "A9" );
					break;
			}
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "G1" );
			Cycle += 9;
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "CA" );
			Cycle += 9;
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "CA" );
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "G14" );
			Cycle += 36;
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "G15" );
			Cycle += 45;
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "G19" );
			Cycle += 81;
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "G12" );
			Cycle += 18;
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_READY:
			strcat( ResRef, "G13" );
			Cycle += 27;
			break;

			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			switch (RangedType) {
				case IE_ANI_RANGED_BOW:
					strcat( ResRef, "SA" );
					break;

				case IE_ANI_RANGED_XBOW:
					strcat( ResRef, "SX" );
					break;

				case IE_ANI_RANGED_THROW:
					strcat( ResRef, "SS" );
					break;
			}
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "G16" );
			Cycle += 54;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "G17" );
			Cycle += 72;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "G11" );
			break;
	}
}


void CharAnimations::AddSixSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_WALK:
			strcat( ResRef, "G1" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "G3" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "G3" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "G3" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "G2" );
			Cycle = 0 + Orient;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "G2" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "G2" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
			strcat( ResRef, "G2" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "G2" );
			Cycle = 64 + Orient;
			break;

	}
	if (Orient>9) {
		strcat( ResRef, "E" );
	}
}

void CharAnimations::AddMHRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Orient /= 2;

	switch (StanceID) {
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			switch (WeaponType) {
				case IE_ANI_WEAPON_1H:
					strcat( ResRef, "A1" );
					break;

				case IE_ANI_WEAPON_2H:
					strcat( ResRef, "A4" );
					break;

				case IE_ANI_WEAPON_2W:
					strcat( ResRef, "A7" );
					break;
			}
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			switch (WeaponType) {
				case IE_ANI_WEAPON_1H:
					strcat( ResRef, "A2" );
					break;

				case IE_ANI_WEAPON_2H:
					strcat( ResRef, "A5" );
					break;

				case IE_ANI_WEAPON_2W:
					strcat( ResRef, "A8" );
					break;
			}
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			switch (WeaponType) {
				case IE_ANI_WEAPON_1H:
					strcat( ResRef, "A3" );
					break;

				case IE_ANI_WEAPON_2H:
					strcat( ResRef, "A6" );
					break;

				case IE_ANI_WEAPON_2W:
					strcat( ResRef, "A9" );
					break;
			}
			Cycle = Orient;
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "G1" );
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "CA" );
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "CA" );
			Cycle = Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "G1" );
			Cycle = 40 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
			strcat( ResRef, "G1" );
			Cycle = 48 + Orient;
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_EMERGE:
			strcat( ResRef, "G1" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "G1" );
			Cycle = 16 + Orient;
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_READY:
			strcat( ResRef, "G1" );
			Cycle = 24 + Orient;
			break;

			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			switch (RangedType) {
				case IE_ANI_RANGED_BOW:
					strcat( ResRef, "SA" );
					Cycle = Orient;
					break;

				case IE_ANI_RANGED_XBOW:
					strcat( ResRef, "SX" );
					Cycle = Orient;
					break;

				case IE_ANI_RANGED_THROW:
					strcat( ResRef, "A1" );
					Cycle = Orient;
					break;
			}
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "G1" );
			Cycle = 64 + Orient;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "G1" );
			Cycle = 56 + Orient;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "G1" );
			Cycle = Orient;
			break;
	}
	if (Orient>=5) {
		strcat( ResRef, "E" );
	}
}

void CharAnimations::AddLRSuffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "G2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "G2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "G2" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			//these animations are missing
			strcat( ResRef, "G2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "G1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_READY:
			strcat( ResRef, "G1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_AWAKE:
			strcat( ResRef, "G1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			strcat( ResRef, "G1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_DIE:
			strcat( ResRef, "G1" );
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
			strcat( ResRef, "G1" );
			Cycle = 40 + Orient / 2;
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "E" );
	}
}

void CharAnimations::AddMMRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "A1" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_SHOOT:
			strcat( ResRef, "A4" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "A2" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_READY:
			strcat( ResRef, "SD" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			strcat( ResRef, "SC" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "GH" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "DE" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "GU" );
			Cycle = ( Orient / 2 );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "SL" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "TW" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "WK" );
			Cycle = ( Orient / 2 );
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "E" );
	}
}

