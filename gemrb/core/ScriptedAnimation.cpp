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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This class handles VVC files of BG2/ToB and converts BAM files to the
// common internal animation format on the fly.

#include "ScriptedAnimation.h"

#include "win32def.h"

#include "Animation.h"
#include "AnimationFactory.h"
#include "Audio.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Map.h"
#include "Sprite2D.h"
#include "Video.h"

namespace GemRB {

#define ILLEGAL 0         //
#define ONE 1             //hold
#define TWO 2             //onset + hold
#define THREE 3           //onset + hold + release
#define DOUBLE 4          //has twin (pst)
#define FIVE 8            //five faces (orientation)
#define NINE 16           //nine faces (orientation)
#define SEVENEYES 32      //special hack for seven eyes

#define MAX_CYCLE_TYPE 16
static const ieByte ctypes[MAX_CYCLE_TYPE]={
	ILLEGAL, ONE, TWO, THREE, TWO|DOUBLE, ONE|FIVE, THREE|DOUBLE, ILLEGAL,
	SEVENEYES, ONE|NINE, TWO|FIVE, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,THREE|FIVE,
};

static const ieByte SixteenToNine[3*MAX_ORIENT]={
	0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1,
	9,10,11,12,13,14,15,16,17,16,15,14,13,12,11,10,
	18,19,20,21,22,23,24,25,26,25,24,23,22,21,20,19
};
static const ieByte SixteenToFive[3*MAX_ORIENT]={
	0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 2, 2, 1, 1,
	5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 8, 8, 7, 7, 6, 6,
	10,10,11,11,12,12,13,13,14,14,13,13,12,12,11,11
};

ScriptedAnimation::ScriptedAnimation()
{
	Init();
}

void ScriptedAnimation::Init()
{
	memset(anims,0,sizeof(anims));
	sounds[0][0] = 0;
	sounds[1][0] = 0;
	sounds[2][0] = 0;
	Transparency = 0;
	Fade = 0;
	SequenceFlags = 0;
	XPos = YPos = ZPos = 0;
	FrameRate = ANI_DEFAULT_FRAMERATE;
	NumOrientations = 0;
	Orientation = 0;
	OrientationFlags = 0;
	Dither = 0;
	Duration = 0xffffffff;
	justCreated = true;
	PaletteName[0]=0;
	twin = NULL;
	Phase = P_NOTINITED;
	SoundPhase = P_NOTINITED;
	effect_owned = false;
	active = true;
	Delay = 0;
	light = NULL;
	LightX = 0;
	LightY = 0;
	LightZ = 0;
	starttime = 0;
}

Animation *ScriptedAnimation::PrepareAnimation(AnimationFactory *af, unsigned int cycle, unsigned int i, bool loop)
{
	int c = cycle;

	switch (NumOrientations) {
	case 5:
		c = SixteenToFive[i];
		break;
	case 9:
		c = SixteenToNine[i];
		break;
	case 16:
		if (af->GetCycleCount() > i) c = i;
		break;
	}
	Animation *anim = af->GetCycle(c);
	if (anim) {
		if (Transparency & IE_VVC_MIRRORX) {
			anim->MirrorAnimation();
		}
		if (Transparency & IE_VVC_MIRRORY) {
			anim->MirrorAnimationVert();
		}
		//creature anims may start at random position, vvcs always start on 0
		anim->pos = 0;
		//vvcs are always paused
		anim->gameAnimation = true;
		if (!loop) {
			anim->Flags |= S_ANI_PLAYONCE;
		}
		anim->fps = FrameRate;
	}
	return anim;
}

/* Creating animation from BAM */
void ScriptedAnimation::LoadAnimationFactory(AnimationFactory *af, int gettwin)
{
	//getcycle returns NULL if there is no such cycle
	//special case, PST double animations

	CopyResRef(ResName, af->ResRef);
	unsigned int cCount = af->GetCycleCount();
	if (cCount >= MAX_CYCLE_TYPE) {
		cCount = 1;
	}

	int type = ctypes[cCount];
	switch(gettwin) {
	case 2:
		if (type == TWO) {
			type = ONE|DOUBLE;
		}
		gettwin = 0;
		break;
	case 1:
		type = ONE|DOUBLE;
		break;
	}

	if (type == ILLEGAL) {
		cCount = 1;
		type = ONE;
	} else if (type & DOUBLE) {
		cCount /= 2;
	}

	//these fields mark that the anim cycles should 'follow' the target's orientation
	if (type & FIVE) {
		NumOrientations = 5;
		cCount = MAX_ORIENT * (type & 3);
	} else if (type & NINE) {
		NumOrientations = 9;
		cCount = MAX_ORIENT * (type & 3);
	} else {
		NumOrientations = 0;
	}

	for (unsigned int i = 0; i < cCount; i++) {
		bool mirror = false;
		int c = i;
		int p = i;
		if (type & DOUBLE) {
			c <<= 1;
			if (gettwin) c++;
			//this is needed for PST twin animations that contain 2 or 3 phases
			assert(p < 3);
			p *= MAX_ORIENT;
		} else if (type & FIVE) {
			c = SixteenToFive[c];
			if ((i & 15) >= 5) mirror = true;
		} else if (type & NINE) {
			c = SixteenToNine[c];
			if ((i & 15) >= 9) mirror = true;
		} else if (!(type & SEVENEYES)) {
			assert(p < 3);
			p *= MAX_ORIENT;
		}

		anims[p] = af->GetCycle(c);
		if (anims[p]) {
			anims[p]->pos = 0;
			if (mirror) {
				anims[p]->MirrorAnimation();
			}
			anims[p]->gameAnimation = true;
		}
	}

	for (unsigned int o = 0; o < MAX_ORIENT; o++) {
		unsigned int p_hold = P_HOLD * MAX_ORIENT + o;
		unsigned int p_onset = P_ONSET * MAX_ORIENT + o;
		unsigned int p_release = P_RELEASE * MAX_ORIENT + o;
		//if there is no hold anim, move the onset anim there
		if (!anims[p_hold]) {
			anims[p_hold]=anims[p_onset];
			anims[p_onset]=NULL;
		}
		//onset and release phases are to be played only once
		if (anims[p_onset])
			anims[p_onset]->Flags |= S_ANI_PLAYONCE;
		if (anims[p_release])
			anims[p_release]->Flags |= S_ANI_PLAYONCE;
	}
	SequenceFlags = IE_VVC_BAM|IE_VVC_LOOP;

	//we are getting a twin, no need of going further,
	//if there is any more common initialisation, it should
	//go above this point
	if (gettwin) {
		return;
	}
	if (type & DOUBLE) {
		twin = new ScriptedAnimation();
		twin->LoadAnimationFactory(af, 1);
	}
	SetPhase(P_ONSET);
}

/* Creating animation from VVC */
ScriptedAnimation::ScriptedAnimation(DataStream* stream)
{
	Init();
	if (!stream) {
		return;
	}

	char Signature[8];

	stream->Read( Signature, 8);
	if (strncmp( Signature, "VVC V1.0", 8 ) != 0) {
		print("Not a valid VVC File");
		delete stream;
		return;
	}
	ieResRef Anim1ResRef;
	int seq1, seq2, seq3;
	stream->ReadResRef( Anim1ResRef );
	// unused second resref; m_cShadowVidCellRef in the original
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &Transparency );
	stream->Seek( 4, GEM_CURRENT_POS ); // unused in the original: m_bltInfo
	stream->ReadDword( &SequenceFlags );
	stream->Seek( 4, GEM_CURRENT_POS ); // unused in the original: m_bltInfoExtra

	ieDword tmp;
	stream->ReadDword( &tmp );
	XPos = (signed) tmp;
	stream->ReadDword( &tmp );  //this affects visibility
	YPos = (signed) tmp;
	stream->Seek( 4, GEM_CURRENT_POS ); // (offset) position flags in the original, "use orientation" on IESDP
	stream->ReadDword( &FrameRate );

	if (!FrameRate) FrameRate = ANI_DEFAULT_FRAMERATE;
	else if (FrameRate > 30) FrameRate = 30;

	stream->ReadDword( &NumOrientations );
	stream->ReadDword( &Orientation );
	stream->ReadDword( &OrientationFlags );
	stream->Seek( 8, GEM_CURRENT_POS ); // CResRef m_cNewPaletteRef in the original

	stream->ReadDword( &tmp );  //this doesn't affect visibility
	ZPos = (signed) tmp;

	stream->ReadDword( &LightX ); // and Lighting effect radius / width / glow
	stream->ReadDword( &LightY );
	stream->ReadDword( &LightZ ); // glow intensity / brightness
	stream->ReadDword( &Duration );
	// m_cVVCResRes in the original, supposedly a self-reference
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &tmp ); // 1 indexed, m_nStartSequence
	seq1 = ((signed) tmp) - 1;
	stream->ReadDword( &tmp ); // 1 indexed; 0 or less for none, m_nLoopSequence
	seq2 = ((signed) tmp) - 1;
	// original
	//   LONG    m_nCurrentSequence; //1 indexed
	//   DWORD   m_sequenceFlags; // only bit0 for continuous playback known
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadResRef( sounds[P_ONSET] );
	stream->ReadResRef( sounds[P_HOLD] );

	// original: CResRef   m_cAlphaBamRef;
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &tmp ); // m_nEndSequence
	seq3 = ((signed) tmp) - 1;
	stream->ReadResRef( sounds[P_RELEASE] );
	// original bg2 has 4*84 of reserved space here

	//if there are no separate phases, then fill the p_hold fields
	if (seq2 < 0 && seq3 < 0) {
		seq2 = std::max(seq1, 0);
		seq1 = -1;
	} else if (seq3 >= 0) {
		// some files (like e.g. SPBLDTOP.VVC in BG2) are 0-indexed instead, sigh
		while (seq1 < 0 || seq2 < 0) {
			seq1++;
			seq2++;
			seq3++;
		}
	}

	if (sounds[P_HOLD][0] == 0 && sounds[P_RELEASE][0] == 0 && (SequenceFlags & IE_VVC_LOOP)) {
		CopyResRef(sounds[P_HOLD], sounds[P_ONSET]);
		sounds[P_ONSET][0] = 0;
	}

	if (SequenceFlags & IE_VVC_BAM) {
		AnimationFactory *af = (AnimationFactory *)
			gamedata->GetFactoryResource(Anim1ResRef, IE_BAM_CLASS_ID);
		if (!af) {
			Log(ERROR, "ScriptedAnimation", "Failed to load animation: %s!", Anim1ResRef);
			return;
		}
		for (unsigned int i = 0; i < MAX_ORIENT; i++) {
			unsigned int p_hold = P_HOLD * MAX_ORIENT + i;

			if (seq1 >= 0) {
				unsigned int p_onset = P_ONSET * MAX_ORIENT + i;
				anims[p_onset] = PrepareAnimation(af, seq1, i);
			}

			anims[p_hold] = PrepareAnimation(af, seq2, i, SequenceFlags & IE_VVC_LOOP);

			if (seq3 >= 0) {
				unsigned int p_release = P_RELEASE * MAX_ORIENT + i;
				anims[p_release] = PrepareAnimation(af, seq3, i);
			}
		}
		if (Transparency&IE_VVC_BLENDED) {
			GetPaletteCopy();
		}
	}

	SetPhase(P_ONSET);

	delete stream;
}

ScriptedAnimation::~ScriptedAnimation(void)
{
	for (Animation *anim : anims) {
		delete anim;
	}

	palette = nullptr;

	if (twin) {
		delete twin;
	}
	StopSound();
}

void ScriptedAnimation::SetPhase(int arg)
{
	if (arg >= P_ONSET && arg <= P_RELEASE) {
		Phase = arg;
		SoundPhase = arg;
	}

	StopSound();

	if (twin) {
		twin->SetPhase(Phase);
	}
}

void ScriptedAnimation::SetSound(int arg, const ieResRef sound)
{
	if (arg >= P_ONSET && arg <= P_RELEASE) {
		CopyResRef(sounds[arg], sound);
	}
	//no need to call the twin
}

void ScriptedAnimation::PlayOnce()
{
	SequenceFlags &= ~IE_VVC_LOOP;
	for (Animation *anim : anims) {
		if (anim) {
			anim->Flags |= S_ANI_PLAYONCE;
		}
	}
	if (twin) {
		twin->PlayOnce();
	}
}

void ScriptedAnimation::SetFullPalette(const ieResRef PaletteResRef)
{
	palette = gamedata->GetPalette(PaletteResRef);
	memcpy(PaletteName, PaletteResRef, sizeof(PaletteName) );
	if (twin) {
		twin->SetFullPalette(PaletteResRef);
	}
}

void ScriptedAnimation::SetFullPalette(int idx)
{
	ieResRef PaletteResRef;

	//make sure this field is zero terminated, or strlwr will run rampant!!!
	snprintf(PaletteResRef, sizeof(PaletteResRef), "%.7s%d", ResName, idx);
	strnlwrcpy(PaletteResRef, PaletteResRef, 8);
	SetFullPalette(PaletteResRef);
	//no need to call twin
}

void ScriptedAnimation::SetPalette(int gradient, int start)
{
	//get a palette
	GetPaletteCopy();
	if (!palette)
		return;
	//default start
	if (start==-1) {
		start=4;
	}

	constexpr int PALSIZE = 12;
	const auto& pal16 = core->GetPalette16(gradient);
	palette->CopyColorRange(&pal16[0], &pal16[PALSIZE], start);

	if (twin) {
		twin->SetPalette(gradient, start);
	}
}

int ScriptedAnimation::GetCurrentFrame()
{
	Animation *anim = anims[P_HOLD*MAX_ORIENT];
	if (anim) {
		return anim->GetCurrentFrameIndex();
	}
	return 0;
}

ieDword ScriptedAnimation::GetSequenceDuration(ieDword multiplier)
{
	//P_HOLD * MAX_ORIENT == MAX_ORIENT
	Animation *anim = anims[P_HOLD*MAX_ORIENT];
	if (anim) {
		return anim->GetFrameCount()*multiplier/FrameRate;
	}
	return 0;
}

void ScriptedAnimation::SetDelay(ieDword delay)
{
	Delay = delay;
	if (twin) {
		twin->Delay=delay;
	}
}

void ScriptedAnimation::SetDefaultDuration(ieDword duration)
{
	if (!(SequenceFlags & (IE_VVC_LOOP|IE_VVC_FREEZE))) return;
	if (Duration==0xffffffff) {
		Duration = duration;
	}
	if (twin) {
		twin->Duration=Duration;
	}
}

void ScriptedAnimation::SetOrientation(int orientation)
{
	if (orientation == -1) {
		return;
	}
	if (NumOrientations > 1) {
		Orientation = (ieByte) orientation;
	} else {
		Orientation = 0;
	}
	if (twin) {
		twin->Orientation = Orientation;
	}
}

bool ScriptedAnimation::HandlePhase(Holder<Sprite2D> &frame)
{
	Game *game = core->GetGame();

	if (justCreated) {
		if (Phase == P_NOTINITED) {
			Log(ERROR, "ScriptedAnimation", "Not fully initialised VVC!");
			return true;
		}
		unsigned long time = game->Ticks;
		if (starttime == 0) {
			starttime = time;
		}
		unsigned int inc = 0;
		if ((time - starttime) >= (unsigned long) (1000 / FrameRate)) {
			inc = (time - starttime) * FrameRate / 1000;
			starttime += inc * 1000 / FrameRate;
		}

		if (Delay >= inc) {
			Delay -= inc;
			return false;
		}
		Delay = 0;

		if (SequenceFlags & IE_VVC_LIGHTSPOT) {
			light = core->GetVideoDriver()->CreateLight(LightX, LightZ);
		}

		if (Duration != 0xffffffff) {
			Duration += core->GetGame()->GameTime;
		}

		justCreated = false;
	}

	// if we're looping forever and we didn't get 'bumped' by an effect
	if (effect_owned && (SequenceFlags & IE_VVC_LOOP) && Duration == 0xffffffff && !active) {
		PlayOnce();
	}

retry:
	if (Phase > P_RELEASE) {
		return true;
	}

	Animation *anim = anims[Phase * MAX_ORIENT + Orientation];
	if (!anim) {
		IncrementPhase();
		goto retry;
	}

	if (game->IsTimestopActive()) {
		frame = anim->LastFrame();
		return false;
	} else {
		frame = anim->NextFrame();
	}

	//explicit duration
	if (Phase == P_HOLD && game->GameTime > Duration) {
		IncrementPhase();
		goto retry;
	}
	if (SequenceFlags & IE_VVC_FREEZE) {
		return false;
	}

	//automatically slip from onset to hold to release
	if (!frame || anim->endReached) {
		//this section implements the freeze fading effect (see ice dagger)
		if (frame && Fade && Tint.a && (Phase == P_HOLD)) {
			if (Tint.a <= Fade) {
				return true;
			}
			Tint.a -= Fade;
			return false;
		}
		IncrementPhase();
		goto retry;
	}
	return false;
}

void ScriptedAnimation::StopSound()
{
	if (sound_handle) {
		sound_handle->Stop();
		sound_handle.release();
	}
}

void ScriptedAnimation::UpdateSound(const Point &pos)
{
	if (Delay > 0 || SoundPhase > P_RELEASE) {
		return;
	}

	if (!sound_handle || !sound_handle->Playing()) {
		while (SoundPhase <= P_RELEASE && sounds[SoundPhase][0] == 0) {
			SoundPhase++;
		}

		if (SoundPhase <= P_RELEASE) {
			sound_handle = core->GetAudioDrv()->Play(sounds[SoundPhase], SFX_CHAN_HITS, XPos + pos.x, YPos + pos.y,
						   (SoundPhase == P_HOLD && (SequenceFlags & IE_VVC_LOOP)) ? GEM_SND_LOOPING : 0);
			SoundPhase++;
		}
	} else {
		sound_handle->SetPos(XPos + pos.x, YPos + pos.y);
	}
}

void ScriptedAnimation::IncrementPhase()
{
	if (Phase == P_HOLD && sound_handle && (SequenceFlags & IE_VVC_LOOP)) {
		// kill looping sound
		sound_handle->StopLooping();
	}
	Phase++;
}

//it is not sure if we need tint at all
bool ScriptedAnimation::Draw(const Point &Pos, const Color &p_tint, Map *area, bool dither, int orientation, int height)
{
	if (!(OrientationFlags & IE_VVC_FACE_FIXED)) {
		SetOrientation(orientation);
	}

	// not sure
	if (twin) {
		twin->Draw(Pos, p_tint, area, dither, -1, height);
	}

	Video *video = core->GetVideoDriver();
	Game *game = core->GetGame();

	Holder<Sprite2D> frame;

	if (HandlePhase(frame)) {
		//expired
		return true;
	}

	//delayed
	if (justCreated) {
		return false;
	}

	UpdateSound(Pos);
	
	uint32_t flag = BLIT_NO_FLAGS;
	if (Transparency & IE_VVC_TRANSPARENT) {
		flag |= BLIT_HALFTRANS;
	}

	Color tint = Tint;

	//darken, greyscale, red tint are probably not needed if the global tint works
	//these are used in the original engine to implement weather/daylight effects
	//on the other hand

	if ((Transparency & IE_VVC_GREYSCALE || game->IsTimestopActive()) && !(Transparency & IE_VVC_NO_TIMESTOP)) {
		flag |= BLIT_GREY;
	}

	if (Transparency & IE_VVC_SEPIA) {
		flag |= BLIT_SEPIA;
	}

	if ((Transparency & IE_VVC_TINT) == IE_VVC_TINT) {
		tint = p_tint;
	}

	ieDword flags = flag;
	if (Transparency & BLIT_TINTED) {
		flags |= BLIT_TINTED;
		game->ApplyGlobalTint(tint, flags);
	}

	int cx = Pos.x + XPos;
	int cy = Pos.y - ZPos + YPos;
	if (SequenceFlags & IE_VVC_HEIGHT) cy -= height;

	if(!(SequenceFlags&IE_VVC_NOCOVER)) {
		if (dither) {
			flags |= BLIT_STENCIL_ALPHA;
		} else if (core->FogOfWar&FOG_DITHERSPRITES) {
			flags |= BLIT_STENCIL_BLUE;
		} else {
			flags |= BLIT_STENCIL_RED;
		}
	}

	video->BlitGameSpriteWithPalette(frame, palette.get(), cx, cy, flags, tint);

	if (light) {
		video->BlitGameSprite(light, cx, cy, flags^flag, tint, NULL);
	}
	return false;
}

Region ScriptedAnimation::DrawingRegion() const
{
	Animation* anim = anims[Phase*MAX_ORIENT+Orientation];
	return (anim) ? anim->animArea : Region();
}

void ScriptedAnimation::SetEffectOwned(bool flag)
{
	effect_owned=flag;
	if (twin) {
		twin->effect_owned=flag;
	}
}

void ScriptedAnimation::SetBlend()
{
	if ((Transparency&IE_VVC_BLENDED) == 0) {
		Transparency |= IE_VVC_BLENDED;
		if (palette) {
			palette = nullptr;
		}
		GetPaletteCopy();
		if (twin)
			twin->SetBlend();
	}
}

void ScriptedAnimation::SetFade(ieByte initial, int speed)
{
	Tint.r=255;
	Tint.g=255;
	Tint.b=255;
	Tint.a=initial;
	Fade=speed;
	Transparency|=BLIT_TINTED;
}

void ScriptedAnimation::GetPaletteCopy()
{
	if (palette)
		return;
	//it is not sure that the first position will have a resource in it
	//therefore the cycle
	for (Animation *anim : anims) {
		if (anim) {
			Holder<Sprite2D> spr = anim->GetFrame(0);
			if (spr) {
				palette = spr->GetPalette()->Copy();
				if ((Transparency&IE_VVC_BLENDED) && palette->HasAlpha() == false) {
					palette->CreateShadedAlphaChannel();
				} else {
					Color shadowalpha = palette->col[1];
					shadowalpha.a /= 2; // FIXME: not sure if this should be /=2 or = 128 (they are probably the same value for all current uses);
					palette->CopyColorRange(&shadowalpha, &shadowalpha + 1, 1);
				}
				//we need only one palette, so break here
				break;
			}
		}
	}
}

void ScriptedAnimation::AlterPalette(const RGBModifier& mod)
{
	GetPaletteCopy();
	if (!palette)
		return;
	palette->SetupGlobalRGBModification(palette.get(), mod);
	if (twin) {
		twin->AlterPalette(mod);
	}
}

ScriptedAnimation *ScriptedAnimation::DetachTwin()
{
	if (!twin) {
		return NULL;
	}
	ScriptedAnimation * ret = twin;
	//ret->Frame.y+=ret->ZPos+1;
	if (ret->ZPos>=0) {
		ret->ZPos=-1;
	}
	twin=NULL;
	return ret;
}

}
