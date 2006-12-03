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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/CharAnimations.cpp,v 1.94 2006/12/03 16:47:24 wjpalenstijn Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AnimationMgr.h"
#include "CharAnimations.h"
#include "Interface.h"
#include "Video.h"
#include "Palette.h"
#include "ResourceMgr.h"
#include "ImageMgr.h"
#include "Map.h"

static int AvatarsCount = 0;
static AvatarStruct *AvatarTable = NULL;
static ieByte SixteenToNine[16]={0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1};
static ieByte SixteenToFive[16]={0,0,1,1,2,2,3,3,4,4,3,3,2,2,1,1};

struct EquipResRefData {
	char Suffix[9];
	unsigned char Cycle;
};


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

unsigned int CharAnimations::GetAnimationID() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].AnimID;
}

int CharAnimations::GetCircleSize() const
{
	if (AvatarsRowNum==~0u) return -1;
	return AvatarTable[AvatarsRowNum].CircleSize;
}
int CharAnimations::NoPalette() const
{
	if (AvatarsRowNum==~0u) return -1;
	return AvatarTable[AvatarsRowNum].PaletteType;
}

int CharAnimations::GetAnimType() const
{
	if (AvatarsRowNum==~0u) return -1;
	return AvatarTable[AvatarsRowNum].AnimationType;
}

char CharAnimations::GetSize() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].Size;
}

int CharAnimations::GetActorPartCount() const
{
	if (AvatarsRowNum==~0u) return -1;
	switch (AvatarTable[AvatarsRowNum].AnimationType) {
	case IE_ANI_NINE_FRAMES: //dragon animations
		return 9;
	case IE_ANI_FOUR_FRAMES: //wyvern animations
		return 4;
	case IE_ANI_PST_GHOST:   //special pst anims
		if (AvatarTable[AvatarsRowNum].Prefixes[1][0]=='*') {
			return 1;
		}
		if (AvatarTable[AvatarsRowNum].Prefixes[2][0]=='*') {
			return 2;
		}
		if (AvatarTable[AvatarsRowNum].Prefixes[3][0]=='*') {
			return 3;
		}
		return 4;
	default:
		return 1;
	}
}

int CharAnimations::GetTotalPartCount() const
{
	if (AvatarsRowNum==~0u) return -1;
	switch (AvatarTable[AvatarsRowNum].AnimationType) {
	case IE_ANI_CODE_MIRROR:
		return GetActorPartCount() + 3; // equipment
	default:
		return GetActorPartCount();
	}
}

void CharAnimations::SetArmourLevel(int ArmourLevel)
{
	if (AvatarsRowNum==~0u) return;
	//ignore ArmourLevel for the static pst anims (all sprites are displayed)
	if (AvatarTable[AvatarsRowNum].AnimationType == IE_ANI_PST_GHOST) {
		ArmourLevel = 0;
	}
	strncpy( ResRef, AvatarTable[AvatarsRowNum].Prefixes[ArmourLevel], 8 );
	ResRef[8]=0;
	DropAnims();
}

void CharAnimations::SetWeaponType(int wt)
{
	if (wt != WeaponType) {
		WeaponType = wt;
		DropAnims();
	}
}

void CharAnimations::SetHelmetRef(const char* ref)
{
	if (HelmetRef[0] != ref[0] || HelmetRef[1] != ref[1]) {
		HelmetRef[0] = ref[0];
		HelmetRef[1] = ref[1];

		// TODO: Only drop helmet anims
		DropAnims();
	}
}

void CharAnimations::SetWeaponRef(const char* ref)
{
	if (WeaponRef[0] != ref[0] || WeaponRef[1] != ref[1]) {
		WeaponRef[0] = ref[0];
		WeaponRef[1] = ref[1];

		// TODO: Only drop weapon anims
		DropAnims();
	}
}

void CharAnimations::SetOffhandRef(const char* ref)
{
	if (OffhandRef[0] != ref[0] || OffhandRef[1] != ref[1]) {
		OffhandRef[0] = ref[0];
		OffhandRef[1] = ref[1];

		// TODO: Only drop shield/offhand anims
		DropAnims();
	}
}

void CharAnimations::SetColors(ieDword *arg)
{
	Colors = arg;
	SetupColors();
}

void CharAnimations::SetupColors()
{
	if (!palette) {
		return;
	}

	if (!Colors) {
		return;
	}

	if (lockPalette) {
		return;
	}

	if (GetAnimType() >= IE_ANI_PST_ANIMATION_1) {
		// Avatars in PS:T
		int size = 32;
		int dest = 256-Colors[6]*size;
		if (Colors[6] == 0) {
			core->FreePalette(palette, PaletteResRef);
			PaletteResRef[0]=0;
			return;
		}
		for (unsigned int i = 0; i < Colors[6]; i++) {
			core->GetPalette( Colors[i]&255, size, &palette->col[dest]);
			//Color* NewPal = core->GetPalette( Colors[i]&255, size );
			//memcpy( &palette->col[dest], NewPal, size*sizeof( Color ) );
			dest +=size;
			//free( NewPal );
		}
		return;
	}

	int PType = NoPalette();
	if ( PType) {
		core->FreePalette(palette, PaletteResRef);
		PaletteResRef[0]=0;
		//handling special palettes like MBER_BL (black bear)
		if (PType==1) {
			return;
		}
		snprintf(PaletteResRef,8,"%.4s_%-.2s",ResRef, (char *) &PType);
		strlwr(PaletteResRef);
		palette = core->GetPalette(PaletteResRef);
		//invalid palette, rolling back
		if (!palette) {
			PaletteResRef[0]=0;
		}
		return;
	}

	//metal
	core->GetPalette( Colors[0]&255, 12, &palette->col[0x04]);
	//minor
	core->GetPalette( Colors[1]&255, 12, &palette->col[0x10]);
	//major
	core->GetPalette( Colors[2]&255, 12, &palette->col[0x1c]);
	//skin
	core->GetPalette( Colors[3]&255, 12, &palette->col[0x28]);
	//leather
	core->GetPalette( Colors[4]&255, 12, &palette->col[0x34]);
	//armor
	core->GetPalette( Colors[5]&255, 12, &palette->col[0x40]);
	//hair
	core->GetPalette( Colors[6]&255, 12, &palette->col[0x4c]);

	//minor
	memcpy( &palette->col[0x58], &palette->col[0x11], 8 * sizeof( Color ) );
	//major
	memcpy( &palette->col[0x60], &palette->col[0x1d], 8 * sizeof( Color ) );
	//minor
	memcpy( &palette->col[0x68], &palette->col[0x11], 8 * sizeof( Color ) );
	//metal
	memcpy( &palette->col[0x70], &palette->col[0x05], 8 * sizeof( Color ) );
	//leather
	memcpy( &palette->col[0x78], &palette->col[0x35], 8 * sizeof( Color ) );
	//leather
	memcpy( &palette->col[0x80], &palette->col[0x35], 8 * sizeof( Color ) );
	//minor
	memcpy( &palette->col[0x88], &palette->col[0x11], 8 * sizeof( Color ) );

	int i; //moved here to be compatible with msvc6.0

	for (i = 0x90; i < 0xA8; i += 0x08)
		//leather
		memcpy( &palette->col[i], &palette->col[0x35], 8 * sizeof( Color ) );
	//skin
	memcpy( &palette->col[0xB0], &palette->col[0x29], 8 * sizeof( Color ) );
	for (i = 0xB8; i < 0xFF; i += 0x08)
		//leather
		memcpy( &palette->col[i], &palette->col[0x35], 8 * sizeof( Color ) );
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
		strnlwrcpy(AvatarTable[i].Prefixes[0],Avatars->QueryField(i,AV_PREFIX1),8);
		strnlwrcpy(AvatarTable[i].Prefixes[1],Avatars->QueryField(i,AV_PREFIX2),8);
		strnlwrcpy(AvatarTable[i].Prefixes[2],Avatars->QueryField(i,AV_PREFIX3),8);
		strnlwrcpy(AvatarTable[i].Prefixes[3],Avatars->QueryField(i,AV_PREFIX4),8);
		AvatarTable[i].AnimationType=(ieByte) atoi(Avatars->QueryField(i,AV_ANIMTYPE) );
		AvatarTable[i].CircleSize=(ieByte) atoi(Avatars->QueryField(i,AV_CIRCLESIZE) );
		const char *tmp = Avatars->QueryField(i,AV_USE_PALETTE);
		//QueryField will always return a zero terminated string
		//so tmp[0] must exist
		if ( isalpha (tmp[0]) ) {
			//this is a hack, we store 2 letters on an integer
			//it was allocated with calloc, so don't bother erasing it
			strncpy( (char *) &AvatarTable[i].PaletteType, tmp, 3);
		}
		else {
			AvatarTable[i].PaletteType=atoi(Avatars->QueryField(i,AV_USE_PALETTE) );
		}
		AvatarTable[i].Size=Avatars->QueryField(i,AV_SIZE)[0];
	}
	core->DelTable( tmp );
}

CharAnimations::CharAnimations(unsigned int AnimID, ieDword ArmourLevel)
{
	Colors = NULL;
	palette = NULL;
	nextStanceID = 0;
	autoSwitchOnEnd = false;
	lockPalette = false;
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
	PaletteResRef[0] = 0;
	WeaponRef[0] = 0;
	HelmetRef[0] = 0;
	OffhandRef[0] = 0;

	AvatarsRowNum=AvatarsCount;
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		ieDword tmp = AnimID&0xf000;
		if (tmp==0x6000 || tmp==0xe000) {
			AnimID&=0xff;
		}
	}

	while (AvatarsRowNum--) {
		if (AvatarTable[AvatarsRowNum].AnimID==AnimID) {
			SetArmourLevel( ArmourLevel );
			return;
		}
	}
	ResRef[0]=0;
	printMessage("CharAnimations", " ", LIGHT_RED);
	printf("Invalid or nonexistent avatar entry:%04X\n", AnimID);
}

//we have to drop them when armourlevel changes
void CharAnimations::DropAnims()
{
	Animation** tmppoi;
	int partCount = GetTotalPartCount();
	for (int StanceID = 0; StanceID < MAX_ANIMS; StanceID++) {
		for (int i = 0; i < MAX_ORIENT; i++) {
			if (Anims[StanceID][i]) {
				tmppoi = Anims[StanceID][i];
				for (int j = 0; j < partCount; j++)
					delete Anims[StanceID][i][j];
				delete[] tmppoi;

				// anims can only be duplicated at the Animation** level
				for (int IDb=StanceID;IDb < MAX_ANIMS; IDb++) {
					for (int i2 = 0; i2<MAX_ORIENT; i2++) {
						if (Anims[IDb][i2] == tmppoi) {
							Anims[IDb][i2] = 0;
						}
					}
				}
			}
		}
	}
}

//freeing the bitmaps only once, but using an intelligent algorithm
CharAnimations::~CharAnimations(void)
{
	DropAnims();
	core->FreePalette(palette, PaletteResRef);
}
/*
This is a simple Idea of how the animation are coded

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
			ACHKg1
			ACHKg1E

			Each BAM File contains many animation groups, each animation group
			stores 5 Orientations, the missing 3 are stored in East BAM Files.


IE_ANI_FOUR_FILES:	The Animation is coded in Four Files. Probably it is an old Two File animation with
			additional frames added in a second time.

IE_ANI_TWENTYTWO:	This Animation Type stores the Animation in the following format
			[NAME][ACTIONCODE][/E]
			ACTIONCODE=A1-6, CA, SX, SA (sling is A1)
			The g1 file contains several animation states. See MHR
			Probably it could use A7-9 files too, bringing the file numbers to 28.
			This is the original bg1 format.

IE_ANI_SIX_FILES:	The layout for these files is:
			[NAME][g1-3][/E]
			Each state contains 16 Orientations, but the last 6 are stored in the East file.
			g1 contains only the walking animation.
			G2 contains stand, ready, get hit, die and twitch.
			g3 contains 3 attacks.

IE_ANI_SIX_FILES_2:     Similar to SIX_FILES, but the orientation numbers are reduced like in FOUR_FILES. Only one animation uses it: MOGR 

IE_ANI_TWO_FILES_2:	Animations using this type are stored using the following template:
			[NAME]g1[/E]
			Each state contains 8 Orientations, but the second 4 are stored in the East file.
			From the standard animations, only AHRS and ACOW belong to this type.

IE_ANI_TWO_FILES_3:	Animations using this type are stored using the following template:
			[NAME][ACTIONTYPE][/E]

			Example:
			MBFI*
			MBFI*E

			Each BAM File contains one animation group, each animation group
			stores 5 Orientations though the files contain all 8 Cycles, the missing 3 are stored in East BAM Files in Cycle: Stance*8+ (5,6,7).
			This is the standard IWD animation, but BG2 also has it.
			See MMR

IE_ANI_FOUR_FRAMES:	These animations are large, four bams make a frame.


IE_ANI_NINE_FRAMES:     These animations are huge, nine bams make a frame.


IE_ANI_PST_ANIMATION_1:
IE_ANI_PST_ANIMATION_2:
IE_ANI_PST_ANIMATION_3:
			Planescape: Torment Animations are stored in a different
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
			Animation_3 is without STC  (combat stance), they are always standing

IE_ANI_PST_STAND:	This is a modified PST animation, it contains only a
			Standing image for every orientations, it follows the
			[C/D]STD[NAME][B] standard.

IE_ANI_PST_GHOST:	This is a special static animation with no standard
			All armourlevels are drawn simultaneously. There is no orientation or stance.


  WEST PART  |  EAST PART
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

Animation** CharAnimations::GetAnimation(unsigned char StanceID, unsigned char Orient)
{
	if (StanceID>=MAX_ANIMS) {
		printf("Illegal stance ID\n");
		abort();
	}

	int AnimType = GetAnimType();

	//alter stance here if it is missing and you know a substitute
	//probably we should feed this result back to the actor?
	switch (AnimType) {
		case -1: //invalid animation
			return NULL;

		case IE_ANI_PST_STAND:
			StanceID=IE_ANI_AWAKE;
			break;
		case IE_ANI_PST_GHOST:
			StanceID=IE_ANI_AWAKE;
			Orient=0;
			break;
		case IE_ANI_PST_ANIMATION_3: //stc->std
			if (StanceID==IE_ANI_READY) {
				StanceID=IE_ANI_AWAKE;
			}
			break;
		case IE_ANI_PST_ANIMATION_2: //std->stc
			if (StanceID==IE_ANI_AWAKE) {
				StanceID=IE_ANI_READY;
			}
			break;
	}
	//pst animations don't have separate animation for sleep/die
	if (AnimType >= IE_ANI_PST_ANIMATION_1) {
		if (StanceID==IE_ANI_DIE) {
			StanceID=IE_ANI_TWITCH;
		}
	}

	//TODO: Implement Auto Resource Loading
	//setting up the sequencing of animation cycles
	autoSwitchOnEnd = false;
	switch (StanceID) {
		case IE_ANI_SLEEP: //going to sleep
			nextStanceID = IE_ANI_TWITCH;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_TWITCH: //dead, sleeping
			autoSwitchOnEnd = false;
			break;
		case IE_ANI_DIE: //going to die
			nextStanceID = IE_ANI_TWITCH;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_WALK:
		case IE_ANI_RUN:
		case IE_ANI_CAST: //IE_ANI_CONJURE is the ending casting anim
		case IE_ANI_READY:
			break;
		case IE_ANI_AWAKE:
			break;
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_PST_START:
			nextStanceID = IE_ANI_AWAKE;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
		case IE_ANI_ATTACK:
			nextStanceID = IE_ANI_READY;
			autoSwitchOnEnd = true;
			break;
		default:
			printf ("Invalid Stance: %d\n", StanceID);
			break;
	}
	Animation** anims = Anims[StanceID][Orient];

	if (anims) {
		return anims;
	}

	int partCount = GetTotalPartCount();
	int actorPartCount = GetActorPartCount();
	if (partCount < 0) return 0;
	anims = new Animation*[partCount];

	EquipResRefData* equipdat = 0;
	for (int part = 0; part < partCount; ++part)
	{
		anims[part] = 0;

		//newresref is based on the prefix (ResRef) and various
		// other things.
		//this is longer than expected so it won't overflow
		char NewResRef[12];
		unsigned char Cycle;
		if (part < actorPartCount) {
			// Character animation parts

			if (equipdat) delete equipdat;

			//we need this long for special anims
			strncpy( NewResRef, ResRef, 8 );
			GetAnimResRef( StanceID, Orient, NewResRef, Cycle, part, equipdat);
		} else {
			// Equipment animation parts

			anims[part] = 0;
			if (GetSize() == '*' || GetSize() == 0) continue;

			if (part == actorPartCount) {
				if (HelmetRef[0] == 0) continue;
				// helmet
				GetEquipmentResRef(HelmetRef,false,NewResRef,Cycle,equipdat);
				printf("Using helmet ref %s\n", NewResRef);
			} else if (part == actorPartCount+1) {
				if (WeaponRef[0] == 0) continue;
				// weapon
				GetEquipmentResRef(WeaponRef,false,NewResRef,Cycle,equipdat);
				printf("Using weapon ref %s\n", NewResRef);
			} else if (part == actorPartCount+2) {
				if (OffhandRef[0] == 0) continue;
				if (WeaponType == IE_ANI_WEAPON_2H) continue;
				// off-hand
				if (WeaponType == IE_ANI_WEAPON_1H) {
					GetEquipmentResRef(OffhandRef,false,NewResRef,Cycle,
									   equipdat);
				} else { // IE_ANI_WEAPON_2W
					GetEquipmentResRef(OffhandRef,true,NewResRef,Cycle,
									   equipdat);
				}
				printf("Using offhand ref %s\n", NewResRef);
			}
		}
		NewResRef[8]=0; //cutting right to size

		AnimationFactory* af = ( AnimationFactory* )
			core->GetResourceMgr()->GetFactoryResource( NewResRef,
														IE_BAM_CLASS_ID,
														IE_NORMAL );

		if (!af) {
			if (part < actorPartCount) {
				char warnbuf[200];
				snprintf(warnbuf, 200,
						 "Couldn't create animationfactory: %s\n", NewResRef);
				printMessage("CharAnimations",warnbuf,LIGHT_RED);
				for (int i = 0; i < part; ++i)
					delete anims[i];
				delete[] anims;
				delete equipdat;
				return 0;
			} else {
				// not fatal if animation for equipment is missing
				continue;
			}
		}

		Animation* a = af->GetCycle( Cycle );
		anims[part] = a;

		if (!a) {
			if (part < actorPartCount) {
				char warnbuf[200];
				snprintf(warnbuf, 200,
						 "Couldn't load animation: %s, cycle %d\n",
						 NewResRef, Cycle);
				printMessage("CharAnimations",warnbuf,LIGHT_RED);
				for (int i = 0; i < part; ++i)
					delete anims[i];
				delete[] anims;
				delete equipdat;
				return 0;
			} else {
				// not fatal if animation for equipment is missing
				continue;
			}
		}

		if (!palette && (NoPalette()!=1) ) {
			// This is the first time we're loading an Animation.
			// We copy the palette of its first frame into our own palette
			palette=core->GetVideoDriver()->GetPalette(a->GetFrame(0))->Copy();

			// ...and setup the colours properly
			SetupColors();
		}
	
		//animation is affected by game flags
		a->gameAnimation = true;
		a->SetPos( 0 );

		//setting up the sequencing of animation cycles
		switch (StanceID) {
			case IE_ANI_SLEEP:
			case IE_ANI_TWITCH:
			case IE_ANI_DIE:
			case IE_ANI_PST_START:
			case IE_ANI_HEAD_TURN:
				a->Flags |= A_ANI_PLAYONCE;
				break;
			case IE_ANI_EMERGE:
			case IE_ANI_GET_UP:
				a->playReversed = true;
				a->Flags |= A_ANI_PLAYONCE;
				break;
		}
		switch (GetAnimType()) {
			case IE_ANI_NINE_FRAMES: //dragon animations
			case IE_ANI_FOUR_FRAMES: //wyvern animations
			case IE_ANI_BIRD:
			case IE_ANI_CODE_MIRROR:
			case IE_ANI_CODE_MIRROR_2: //9 orientations
			case IE_ANI_PST_ANIMATION_3:  //no stc just std
			case IE_ANI_PST_ANIMATION_2:  //no std just stc
			case IE_ANI_PST_ANIMATION_1:
				if (Orient > 8) {
					a->MirrorAnimation( );
				}
				break;
			default:
				break;
		}

		// make animarea of part 0 encompass the animarea of the other parts
		if (part > 0)
			anims[0]->AddAnimArea(a);

	}

	switch (GetAnimType()) {
		case IE_ANI_NINE_FRAMES: //dragon animations
			Anims[StanceID][Orient] = anims;
			break;
		case IE_ANI_FOUR_FRAMES: //wyvern animations
			Anims[StanceID][Orient] = anims;
			break;
		case IE_ANI_BIRD:
			Anims[StanceID][Orient] = anims;
			break;
		case IE_ANI_CODE_MIRROR:
			Anims[StanceID][Orient] = anims;
			break;
			
		case IE_ANI_SIX_FILES: //16 anims some are stored elsewhere
		case IE_ANI_ONE_FILE: //16 orientations
			Anims[StanceID][Orient] = anims;
			break;
		case IE_ANI_CODE_MIRROR_2: //9 orientations
			Anims[StanceID][Orient] = anims;
			break;
		case IE_ANI_TWO_FILES:
		case IE_ANI_TWENTYTWO:
		case IE_ANI_TWO_FILES_2:
		case IE_ANI_TWO_FILES_3:
		case IE_ANI_FOUR_FILES:
		case IE_ANI_SIX_FILES_2:
			Orient&=~1;
			Anims[StanceID][Orient] = anims;
			Anims[StanceID][Orient + 1] = anims;
			break;

		case IE_ANI_PST_ANIMATION_3:  //no stc just std
		case IE_ANI_PST_ANIMATION_2:  //no std just stc
		case IE_ANI_PST_ANIMATION_1:
			switch (StanceID) {
				case IE_ANI_WALK:
				case IE_ANI_RUN:
				case IE_ANI_PST_START:
					Anims[StanceID][Orient] = anims;
					break;
				default:
					Orient &=~1;
					Anims[StanceID][Orient] = anims;
					Anims[StanceID][Orient + 1] = anims;
					break;
			}
			break;

		case IE_ANI_PST_STAND:
			Orient &=~1;
			Anims[StanceID][Orient] = anims;
			Anims[StanceID][Orient+1] = anims;
			break;
		case IE_ANI_PST_GHOST:
			Orient = 0;
			StanceID = IE_ANI_AWAKE;
			Anims[StanceID][0] = anims;
			break;
		default:
			printMessage("CharAnimations","Unknown animation type\n",LIGHT_RED);
			abort();
	}
	delete equipdat;

	return Anims[StanceID][Orient];
}

static int one_file[19]={2, 1, 0, 0, 2, 3, 0, 1, 0, 4, 1, 0, 0, 0, 3, 1, 4, 4, 4};

void CharAnimations::GetAnimResRef(unsigned char StanceID,
									 unsigned char Orient,
									 char* NewResRef, unsigned char& Cycle,
									 int Part, EquipResRefData*& EquipData)
{
	char tmp[256];
	EquipData = 0;
	Orient &= 15;
	switch (GetAnimType()) {
		case IE_ANI_FOUR_FRAMES:
			AddFFSuffix( NewResRef, StanceID, Cycle, Orient, Part );
			break;

		case IE_ANI_NINE_FRAMES:
			AddNFSuffix( NewResRef, StanceID, Cycle, Orient, Part );
			break;

		case IE_ANI_CODE_MIRROR:
			AddVHRSuffix( NewResRef, StanceID, Cycle, Orient, EquipData );
			break;

		case IE_ANI_BIRD:
			Cycle = (ieByte) ((StanceID&1) * 9 + SixteenToNine[Orient]);
			break;

		case IE_ANI_ONE_FILE:
			Cycle = (ieByte) (one_file[StanceID] * 16 + Orient);
			break;

		case IE_ANI_SIX_FILES:
			AddSixSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWENTYTWO:  //5+3 animations
			AddMHRSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES_2:  //4+4 animations
			AddLR2Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES_3: //IWD style anims
			AddMMRSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES: 
			AddTwoFileSuffix(NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_FOUR_FILES:
			AddLRSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_SIX_FILES_2: //MOGR (variant of FOUR_FILES)
			AddLR3Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_CODE_MIRROR_2: //9 orientations
			AddVHR2Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_PST_ANIMATION_1:
		case IE_ANI_PST_ANIMATION_2:
		case IE_ANI_PST_ANIMATION_3:
			AddPSTSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_PST_STAND:
			sprintf(NewResRef,"%cSTD%4s",ResRef[0], ResRef+1);
			Cycle = (ieByte) SixteenToFive[Orient];
			break;
		case IE_ANI_PST_GHOST: // pst static animations
			//still doesn't handle the second cycle of the golem anim
			Cycle = 0;
			strnlwrcpy(NewResRef, AvatarTable[AvatarsRowNum].Prefixes[Part], 8);
			break;
		default:
			sprintf (tmp,"Unknown animation type in avatars.2da row: %d\n", AvatarsRowNum);
			printMessage ("CharAnimations",tmp, LIGHT_RED);
			abort();
	}
}

void CharAnimations::GetEquipmentResRef(const char* equipRef, bool offhand,
	char* ResRef, unsigned char& Cycle, EquipResRefData* equip)
{
	switch (GetAnimType()) {
		case IE_ANI_CODE_MIRROR:
			GetVHREquipmentRef( ResRef, Cycle, equipRef, offhand, equip );
			break;
		default:
			printMessage ("CharAnimations", "Unsupported animation type for equipment animation.\n", LIGHT_RED);
			abort();
		break;
	}
}

void CharAnimations::AddPSTSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	char *Prefix;

	switch (StanceID) {
		case IE_ANI_ATTACK:
			Cycle=SixteenToFive[Orient];
			Prefix="at1"; break;
		case IE_ANI_DAMAGE:
			Cycle=SixteenToFive[Orient];
			Prefix="hit"; break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			Cycle=SixteenToFive[Orient];
			Prefix="gup"; break;
		case IE_ANI_AWAKE:
			Cycle=SixteenToFive[Orient];
			Prefix="std"; break;
		case IE_ANI_READY:
			Cycle=SixteenToFive[Orient];
			Prefix="stc"; break;
		case IE_ANI_DIE:
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle=SixteenToFive[Orient];
			Prefix="dfb"; break;
		case IE_ANI_RUN:
			Cycle=SixteenToNine[Orient];
			Prefix="run"; break;
		case IE_ANI_WALK:
			Cycle=SixteenToNine[Orient];
			Prefix="wlk"; break;
		case IE_ANI_HEAD_TURN:
			Cycle=SixteenToFive[Orient];
			if (rand()&1) {
				Prefix="sf2";
				sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
				if (core->Exists(ResRef, IE_BAM_CLASS_ID) ) {
					return;
				}
			}
			Prefix="sf1";
			sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
			if (core->Exists(ResRef, IE_BAM_CLASS_ID) ) {
				return;
			}
			Prefix = "stc";
			break;
		case IE_ANI_PST_START:
			Cycle=0;
			Prefix="ms1"; break;
		default: //just in case
			Cycle=SixteenToFive[Orient];
			Prefix="stc"; break;
	}
	sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
}

void CharAnimations::AddVHR2Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g21" );
			break;

		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g2" );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g26" );
			Cycle+=45;
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "g25" );
			Cycle+=45;
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "g26" );
			Cycle+=54;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
			strcat( ResRef, "g12" );
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g15" );
			Cycle+=45;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g14" );
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
			strcat( ResRef, "g14" );
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g13" );
			Cycle+=27;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g11" );
			break;
		default:
			printf("VHR2 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
}
//                            0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
static char *StancePrefix[]={"3","2","5","5","4","4","2","2","5","4","1","3","3","3","4","1","4","4","4"};
static char *CyclePrefix[]= {"0","0","1","1","1","1","0","0","1","1","1","1","1","1","1","1","1","1","1"};
static unsigned int CycleOffset[] = {0,  0,  0,  0,  0,  9,  0,  0,  0, 18,  0,  0,  9,  18,  0,  0,  0,  0,  0};


// Note: almost like SixSuffix
void CharAnimations::AddFFSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part)
{
	switch (StanceID) {
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g3" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g3" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g3" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g2" );
			Cycle = 0 + Orient;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g2" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g2" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "g2" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g2" );
			Cycle = 64 + Orient;
			break;

		default:
			printf("Six Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;

	}
	ResRef[6]=(char) (Part+'1');
	ResRef[7]=0;
}

void CharAnimations::AddNFSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part)
{
	char prefix[10];

	Cycle = SixteenToNine[Orient];
	snprintf(prefix, 9, "%s%s%d%s%d", ResRef, StancePrefix[StanceID], Part+1,
			 CyclePrefix[StanceID], Cycle);
	strnlwrcpy(ResRef,prefix,8);
	Cycle=(ieByte) (Cycle+CycleOffset[StanceID]);
}

//Attack
//h1, h2, w2
static char *SlashPrefix[]={"a1","a4","a7"};
static char *BackPrefix[]={"a2","a5","a8"};
static char *JabPrefix[]={"a3","a6","a9"};
static char *RangedPrefix[]={"sa","sx","ss"};
static char *RangedPrefixOld[]={"sa","sx","a1"};

void CharAnimations::AddVHRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData*& EquipData)
{
	Cycle = SixteenToNine[Orient];
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		//Attack is a special case... it takes cycles randomly
		//based on the weapon type (TODO)
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, SlashPrefix[WeaponType] );
			strcpy( EquipData->Suffix, SlashPrefix[WeaponType] );
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, BackPrefix[WeaponType] );
			strcpy( EquipData->Suffix, BackPrefix[WeaponType] );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, JabPrefix[WeaponType] );
			strcpy( EquipData->Suffix, JabPrefix[WeaponType] );
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "g17" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 63;
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "ca" );
			strcpy( EquipData->Suffix, "ca" );
			Cycle += 9;
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "ca" );
			strcpy( EquipData->Suffix, "ca" );
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g14" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 36;
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "g15" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 45;
			break;
			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START: // to make ctrl-s work in BG2
			strcat( ResRef, "g19" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 81;
			break;

		case IE_ANI_HEAD_TURN:
			if (rand()&1) {
				strcat( ResRef, "g12" );
				Cycle += 18;
			} else {
				strcat( ResRef, "g18" );
				Cycle += 72;
			}
			strcpy( EquipData->Suffix, "g1" );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g13" ); //two handed
			strcpy( EquipData->Suffix, "g1" ); //two handed
			Cycle += 27;
			break;
			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			strcat( ResRef, RangedPrefix[RangedType] );
			strcpy( EquipData->Suffix, RangedPrefix[RangedType] );
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g16" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 54;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g16" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 54;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g11" );
			strcpy( EquipData->Suffix, "g1" );
			break;

		default:
			printf("VHR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetVHREquipmentRef(char* ResRef, unsigned char& Cycle,
										const char* equipRef, bool offhand,
										EquipResRefData* equip)
{
	Cycle = equip->Cycle;
	if (offhand) {
		sprintf( ResRef, "wq%c%c%co%s", GetSize(), equipRef[0], equipRef[1], equip->Suffix );
	} else {
		sprintf( ResRef, "wq%c%c%c%s", GetSize(), equipRef[0], equipRef[1], equip->Suffix );
	}
}

void CharAnimations::AddSixSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g3" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g3" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g3" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g2" );
			Cycle = 0 + Orient;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g2" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g2" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "g2" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g2" );
			Cycle = 64 + Orient;
			break;

		default:
			printf("Six Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;

	}
	if (Orient>9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddLR2Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Orient /= 2;

	switch (StanceID) {
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
		case IE_ANI_READY:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_HIDE:
		case IE_ANI_WALK:
		case IE_ANI_AWAKE:
			Cycle = 0 + Orient;
			break;

		case IE_ANI_SHOOT:
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_HEAD_TURN:
			Cycle = 8 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			Cycle = 24 + Orient;
			break;

		case IE_ANI_DAMAGE:
			Cycle = 16 + Orient;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle = 32 + Orient;
			break;
		default:
			printf("LR2 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
	if (Orient>=4) {
		strcat( ResRef, "g1e" );
	} else {
		strcat( ResRef, "g1" );
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
			strcat (ResRef, SlashPrefix[WeaponType]);
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat (ResRef, BackPrefix[WeaponType]);
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			strcat (ResRef, JabPrefix[WeaponType]);
			Cycle = Orient;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "ca" );
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "ca" );
			Cycle = Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g1" );
			Cycle = 40 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
			strcat( ResRef, "g1" );
			Cycle = 48 + Orient;
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_EMERGE:
			strcat( ResRef, "g1" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "g1" );
			Cycle = 16 + Orient;
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			Cycle = 24 + Orient;
			break;

			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			strcat (ResRef, RangedPrefixOld[RangedType]);
			Cycle = Orient;
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g1" );
			Cycle = 64 + Orient;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g1" );
			Cycle = 56 + Orient;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = Orient;
			break;
		default:
			printf("MHR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
	if (Orient>=5) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddTwoFileSuffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch(StanceID) {
		case IE_ANI_HEAD_TURN:
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle = 40 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_DIE:
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_WALK:
			Cycle = Orient / 2;
			break;
		default:
			Cycle = 8 + Orient / 2;
			break;
	}
	strcat( ResRef, "g1" );
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddLRSuffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g2" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			//these animations are missing
			strcat( ResRef, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			strcat( ResRef, "g1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "g1" );
			Cycle = 32 + Orient / 2;
			break;
			break;
		case IE_ANI_DIE:
			strcat( ResRef, "g1" );
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
			strcat( ResRef, "g1" );
			Cycle = 40 + Orient / 2;
			break;
		default:
			printf("LR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

//Only for the ogre animation (MOGR)
void CharAnimations::AddLR3Suffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g2" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			strcat( ResRef, "g3" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			strcat( ResRef, "g3" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "g3" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
			strcat( ResRef, "g3" );
			Cycle = 24 + Orient / 2;
			break;
		default:
			printf("LR3 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
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
			strcat( ResRef, "a1" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_SHOOT:
			strcat( ResRef, "a4" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "a2" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			strcat( ResRef, "sd" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "ca" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "sp" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "sc" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "gh" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "de" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "gu" );
			Cycle = ( Orient / 2 );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "sl" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "tw" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "wk" );
			Cycle = ( Orient / 2 );
			break;
		default:
			printf("MMR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			abort();
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}
