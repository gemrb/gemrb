/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ScriptedAnimation.cpp,v 1.22 2006/03/26 12:39:17 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "ScriptedAnimation.h"
#include "AnimationMgr.h"
#include "Interface.h"
#include "ResourceMgr.h"
#include "SoundMgr.h"
#include "Video.h"

/* Creating animation from BAM */
ScriptedAnimation::ScriptedAnimation(AnimationFactory *af, Point &p)
{
	//getcycle returns NULL if there is no such cycle
	for(unsigned int i=0;i<3;i++) {
		anims[i] = af->GetCycle( i );
		palettes[i] = NULL;
		sounds[i][0] = 0;
		if (anims[i]) {
			anims[i]->pos=0;
			anims[i]->gameAnimation=true;
		}
	}
	//if there is no hold anim, move the onset anim there
	if (!anims[P_HOLD]) {
		anims[P_HOLD]=anims[P_ONSET];
		anims[P_ONSET]=NULL;
	}
	//onset and release phases are to be played only once
	if (anims[P_ONSET])
		anims[P_ONSET]->Flags |= S_ANI_PLAYONCE;
	if (anims[P_RELEASE])
		anims[P_RELEASE]->Flags |= S_ANI_PLAYONCE;

	Transparency = 0;
	SequenceFlags = 0;
	XPos = YPos = ZPos = 0;
	FrameRate = 15;
	FaceTarget = 0;
	XPos += p.x;
	YPos += p.y;
	justCreated = true;
	memcpy(ResName, af->ResRef, 8);
	Phase = P_ONSET;
}

/* Creating animation from VVC */
ScriptedAnimation::ScriptedAnimation(DataStream* stream, Point &p, bool autoFree)
{
	anims[0] = NULL;
	anims[1] = NULL;
	anims[2] = NULL;
	palettes[0] = NULL;
	palettes[1] = NULL;
	palettes[2] = NULL;
	sounds[0][0] = 0;
	sounds[1][0] = 0;
	sounds[2][0] = 0;

	Transparency = 0;
	SequenceFlags = 0;
	XPos = YPos = ZPos = 0;
	FrameRate = 15;
	FaceTarget = 0;
	if (!stream) {
		return;
	}

	char Signature[8];

	stream->Read( Signature, 8);
	if (strncmp( Signature, "VVC V1.0", 8 ) != 0) {
		printf( "Not a valid VVC File\n" );
		if (autoFree)
			delete( stream );
		return;
	}
	ieResRef Anim1ResRef;  
	ieDword seq1, seq2;
	stream->ReadResRef( Anim1ResRef );
	//there is no proof it is a second resref
	//stream->ReadResRef( Anim2ResRef );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &Transparency );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &SequenceFlags );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &XPos );
	stream->ReadDword( &YPos );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &FrameRate );
	stream->ReadDword( &FaceTarget );
	stream->Seek( 16, GEM_CURRENT_POS );
	stream->ReadDword( &ZPos );
	stream->Seek( 24, GEM_CURRENT_POS );
	stream->ReadDword( &seq1 );
	stream->ReadDword( &seq2 );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadResRef( sounds[P_ONSET] );
	stream->ReadResRef( sounds[P_HOLD] );

	AnimationFactory* af = ( AnimationFactory* )
		core->GetResourceMgr()->GetFactoryResource( Anim1ResRef, IE_BAM_CLASS_ID );
	//no idea about vvc phases, i think they got no endphase?
	//they certainly got onset and hold phases
	anims[P_ONSET] = af->GetCycle( ( unsigned char ) seq1 );
	if (anims[P_ONSET]) {
		//creature anims may start at random position, vvcs always start on 0
		anims[P_ONSET]->pos=0;
		//vvcs are always paused
		anims[P_ONSET]->gameAnimation=true;
	}

	anims[P_HOLD] = af->GetCycle( ( unsigned char ) seq2 );  
	if (anims[P_HOLD]) {
		anims[P_HOLD]->pos=0;
		anims[P_HOLD]->gameAnimation=true;
		if (!(SequenceFlags&IE_VVC_LOOP) ) {
			anims[P_HOLD]->Flags |= S_ANI_PLAYONCE;
		}
	}

	XPos += p.x;
	YPos += p.y;
	justCreated = true;

	//copying resource name to the object, so it could be referenced by it
	//used by immunity/remove specific animation
	memcpy(ResName, stream->filename, 8);
	for(int i=0;i<8;i++) {
		if (ResName[i]=='.') ResName[i]=0;
	}
	Phase = P_ONSET;

	if (autoFree) {
		delete( stream );
	}
}

ScriptedAnimation::~ScriptedAnimation(void)
{
	Video *video = core->GetVideoDriver();

	for(unsigned int i=0;i<3;i++) {
		if (anims[i]) {
			delete( anims[i] );
		}
		video->FreePalette(palettes[i]);
	}
}

void ScriptedAnimation::SetPhase(int arg)
{
	if (arg>=0 && arg<=2)
		Phase = arg;
}

void ScriptedAnimation::SetSound(int arg, const ieResRef sound)
{
	if (arg>=0 && arg<=2)
		memcpy(sounds[arg],sound,sizeof(sound));
}

void ScriptedAnimation::SetPalette(int gradient, int start)
{
	unsigned int i;

	for(i=0;i<3;i++) {
		if (!palettes[i] && anims[i]) {
			// We copy the palette of its first frame into our own palette
			palettes[i]=core->GetVideoDriver()->GetPalette(anims[i]->GetFrame(0))->Copy();
		}
	}

	//default start
	if (start==-1) {
		start=4;
	}
	unsigned int size = 12;
	Color* NewPal = core->GetPalette( gradient&255, size );

	for(i=0;i<3;i++) {
		if (palettes[i]) {
			memcpy( &palettes[i]->col[start], NewPal, size*sizeof( Color ) );
		}
	}
	free( NewPal );
}

bool ScriptedAnimation::Draw(Region &screen, Point &Pos, Color &tint)
{
	Video *video = core->GetVideoDriver();

	if (justCreated) {
		justCreated = false;
		if (!anims[P_ONSET]) {
			Phase = P_HOLD;
		}
retry:
		if (sounds[Phase][0] != 0) {
			core->GetSoundMgr()->Play( sounds[Phase] );
		}
	}

	if (!anims[Phase]) {
		return true;
	}
	Sprite2D* frame = anims[Phase]->NextFrame();
	//automatically slip from onset to hold
	if (!frame || anims[Phase]->endReached) {
		if (Phase) {
			return true;
		}
		Phase = P_HOLD;
		goto retry;
	}
	ieDword flag = 0;
	if (Transparency & IE_VVC_TRANSPARENT) {
		flag |= BLIT_HALFTRANS;
	}

	if (Transparency & IE_VVC_BRIGHTEST) {
		flag |= BLIT_TINTED;
	}

	video->BlitGameSprite( frame, Pos.x + XPos + screen.x,
		 Pos.y + YPos + screen.y, flag,
		 tint, 0, palettes[Phase], &screen);
	return false;
}
