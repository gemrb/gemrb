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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ScriptedAnimation.cpp,v 1.27 2006/03/29 17:42:44 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "ScriptedAnimation.h"
#include "AnimationMgr.h"
#include "Interface.h"
#include "ResourceMgr.h"
#include "SoundMgr.h"
#include "Video.h"
#include "Game.h"

/* Creating animation from BAM */
ScriptedAnimation::ScriptedAnimation(AnimationFactory *af, Point &p, int height)
{
	cover = NULL;
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
	FrameRate = 15;
	FaceTarget = 0;
	Duration = 0xffffffff;
	XPos = p.x;
	YPos = height;
	ZPos = p.y;
	justCreated = true;
	memcpy(ResName, af->ResRef, 8);
	SetPhase(P_ONSET);
}

/* Creating animation from VVC */
ScriptedAnimation::ScriptedAnimation(DataStream* stream, bool autoFree)
{
	cover = NULL;
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
	ieDword seq1, seq2, seq3;
	stream->ReadResRef( Anim1ResRef );
	//there is no proof it is a second resref
	//stream->ReadResRef( Anim2ResRef );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &Transparency );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &SequenceFlags );
	stream->Seek( 4, GEM_CURRENT_POS );
	ieDword tmp;
	stream->ReadDword( &tmp );
	XPos = (signed) tmp;
	stream->ReadDword( &tmp );  //this affects visibility
	ZPos = (signed) tmp;
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &FrameRate );
	stream->ReadDword( &FaceTarget );
	stream->Seek( 16, GEM_CURRENT_POS );
	stream->ReadDword( &tmp );  //this doesn't affect visibility
	YPos = (signed) tmp;
	stream->Seek( 12, GEM_CURRENT_POS );
	stream->ReadDword( &Duration );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &seq1 );
	stream->ReadDword( &seq2 );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadResRef( sounds[P_ONSET] );
	stream->ReadResRef( sounds[P_HOLD] );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &seq3 );
	stream->ReadResRef( sounds[P_RELEASE] );

	if (SequenceFlags&IE_VVC_BAM) {
		AnimationFactory* af = ( AnimationFactory* )
			core->GetResourceMgr()->GetFactoryResource( Anim1ResRef, IE_BAM_CLASS_ID );
		//no idea about vvc phases, i think they got no endphase?
		//they certainly got onset and hold phases
		anims[P_ONSET] = af->GetCycle( ( unsigned char ) seq1 );
		if (anims[P_ONSET]) {
			if (Transparency&IE_VVC_MIRRORX) {
				anims[P_ONSET]->MirrorAnimation();

			}
			if (Transparency&IE_VVC_MIRRORY) {
				anims[P_ONSET]->MirrorAnimationVert();
			}

			//creature anims may start at random position, vvcs always start on 0
			anims[P_ONSET]->pos=0;
			//vvcs are always paused
			anims[P_ONSET]->gameAnimation=true;
			anims[P_ONSET]->Flags |= S_ANI_PLAYONCE;
		}

		anims[P_HOLD] = af->GetCycle( ( unsigned char ) seq2 );  
		if (anims[P_HOLD]) {
		 	if (Transparency&IE_VVC_MIRRORX) {
					anims[P_HOLD]->MirrorAnimation();
			}
		 	if (Transparency&IE_VVC_MIRRORY) {
					anims[P_HOLD]->MirrorAnimationVert();
			}

			anims[P_HOLD]->pos=0;
			anims[P_HOLD]->gameAnimation=true;
			if (!(SequenceFlags&IE_VVC_LOOP) ) {
				anims[P_HOLD]->Flags |= S_ANI_PLAYONCE;
			}
		}

		anims[P_RELEASE] = af->GetCycle( ( unsigned char ) seq3 );  
		if (anims[P_RELEASE]) {
		 	if (Transparency&IE_VVC_MIRRORX) {
					anims[P_RELEASE]->MirrorAnimation();
			}
		 	if (Transparency&IE_VVC_MIRRORY) {
					anims[P_RELEASE]->MirrorAnimationVert();
			}

			anims[P_RELEASE]->pos=0;
			anims[P_RELEASE]->gameAnimation=true;
			anims[P_RELEASE]->Flags |= S_ANI_PLAYONCE;
		}
	}

	justCreated = true;

	//copying resource name to the object, so it could be referenced by it
	//used by immunity/remove specific animation
	memcpy(ResName, stream->filename, 8);
	for(int i=0;i<8;i++) {
		if (ResName[i]=='.') ResName[i]=0;
	}
	SetPhase(P_ONSET);

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
	SetSpriteCover(NULL);
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

//it is not sure if we need tint at all
bool ScriptedAnimation::Draw(Region &screen, Point &Pos, Color &tint, Map *area, int dither)
{
	Video *video = core->GetVideoDriver();

	if (justCreated) {
		justCreated = false;
		if (Duration!=0xffffffff) {
			Duration += core->GetGame()->Ticks;
		}
		if (!anims[P_ONSET]) {
			Phase = P_HOLD;
		}
retry:
		if (sounds[Phase][0] != 0) {
			core->GetSoundMgr()->Play( sounds[Phase] );
		}
	}

	if (!anims[Phase]) {
		if (Phase>=P_RELEASE) {
			return true;
		}
		Phase++;
		goto retry;
	}
	Sprite2D* frame = anims[Phase]->NextFrame();

	//explicit duration
	if (core->GetGame()->Ticks>Duration) {
		return true;
	}
	//automatically slip from onset to hold to release
	if (!frame || anims[Phase]->endReached) {
		if (Phase>=P_RELEASE) {
			return true;
		}
		Phase++;
		goto retry;
	}
	ieDword flag = 0;
	if (Transparency & IE_VVC_TRANSPARENT) {
		flag |= BLIT_HALFTRANS;
	}

	if (Transparency & IE_VVC_BLENDED) {
		flag |= BLIT_BLENDED;
	}

	if ((Transparency & IE_VVC_TINT)==IE_VVC_TINT) {
		flag |= BLIT_TINTED;
	}

	int cx = Pos.x + XPos;
	int cy = Pos.y + ZPos + YPos;
	if (cover && ( (SequenceFlags&IE_VVC_NOCOVER) || 
		(!cover->Covers(cx, cy, frame->XPos, frame->YPos, frame->Width, frame->Height))) ) {
		SetSpriteCover(NULL);
	}
	if (!(cover || (SequenceFlags&IE_VVC_NOCOVER)) ) {    
		cover = area->BuildSpriteCover(cx, cy, -anims[Phase]->animArea.x, 
			-anims[Phase]->animArea.y, anims[Phase]->animArea.w, anims[Phase]->animArea.h, dither);
		assert(cover->Covers(cx, cy, frame->XPos, frame->YPos, frame->Width, frame->Height));
	}

	video->BlitGameSprite( frame, cx + screen.x, cy + screen.y, flag, tint, cover, palettes[Phase], &screen);
	return false;
}
