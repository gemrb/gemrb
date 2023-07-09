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

#include "AnimationFactory.h"
#include "Audio.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Light.h"
#include "Logging/Logging.h"
#include "Map.h"
#include "Video/Pixels.h"
#include "Sprite2D.h"

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

Animation* ScriptedAnimation::PrepareAnimation(const AnimationFactory& af, Animation::index_t cycle, Animation::index_t i, bool loop) const
{
	Animation::index_t c = cycle;

	if (NumOrientations == 16 || OrientationFlags & IE_VVC_FACE_FIXED) {
		if (af.GetCycleCount() > i) c = i;
	} else if (NumOrientations == 5) {
		c = SixteenToFive[i];
	} else if (NumOrientations == 9) {
		c = SixteenToNine[i];
	}

	Animation *anim = af.GetCycle(c);
	if (anim) {
		anim->MirrorAnimation(BlitFlags(Transparency) & (BlitFlags::MIRRORX | BlitFlags::MIRRORY));
		//creature anims may start at random position, vvcs always start on 0
		anim->frameIdx = 0;
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
void ScriptedAnimation::LoadAnimationFactory(const AnimationFactory& af, int gettwin)
{
	//getcycle returns NULL if there is no such cycle
	//special case, PST double animations

	ResName = af.resRef;
	// some anims like FIREL.BAM in IWD contain empty cycles
	unsigned int cCount = 0;
	for (AnimationFactory::index_t i = 0; i < af.GetCycleCount() && af.GetCycleSize(i) > 0; ++i) {
		++cCount;
	}

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

	for (Animation::index_t i = 0; i < cCount; i++) {
		BlitFlags mirrorFlags = BlitFlags::NONE;
		Animation::index_t c = i;
		int p = i;
		if (type & DOUBLE) {
			c <<= 1;
			if (gettwin) c++;
			//this is needed for PST twin animations that contain 2 or 3 phases
			assert(p < 3);
			p *= MAX_ORIENT;
		} else if (type & FIVE) {
			c = SixteenToFive[c];
			if ((i & 15) >= 5) mirrorFlags = BlitFlags::MIRRORX;
		} else if (type & NINE) {
			c = SixteenToNine[c];
			if ((i & 15) >= 9) mirrorFlags = BlitFlags::MIRRORX;
		} else if (!(type & SEVENEYES)) {
			assert(p < 3);
			p *= MAX_ORIENT;
		}

		anims[p] = af.GetCycle(c);
		if (anims[p]) {
			anims[p]->frameIdx = 0;
			anims[p]->MirrorAnimation(mirrorFlags);
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
	if (!stream) {
		return;
	}

	char Signature[8];

	stream->Read( Signature, 8);
	if (strncmp( Signature, "VVC V1.0", 8 ) != 0) {
		Log(ERROR, "ScriptedAnimation", "Not a valid VVC file!");
		delete stream;
		return;
	}
	ResRef Anim1ResRef;
	int seq1, seq2, seq3;
	stream->ReadResRef( Anim1ResRef );
	// unused second resref; m_cShadowVidCellRef in the original
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword(Transparency);
	stream->Seek( 4, GEM_CURRENT_POS ); // unused in the original: m_bltInfo
	stream->ReadDword(SequenceFlags);
	stream->Seek( 4, GEM_CURRENT_POS ); // unused in the original: m_bltInfoExtra

	ieDword tmp;
	stream->ReadDword(tmp);
	XOffset = int(tmp);
	stream->ReadDword(tmp);  //this affects visibility
	YOffset = int(tmp);
	stream->Seek( 4, GEM_CURRENT_POS ); // (offset) position flags in the original, "use orientation" on IESDP
	stream->ReadDword(FrameRate);

	if (!FrameRate) FrameRate = ANI_DEFAULT_FRAMERATE;
	else if (FrameRate > 30) FrameRate = 30;

	stream->ReadDword(NumOrientations);
	stream->ReadDword(tmp);
	Orientation = ClampToOrientation(tmp);
	stream->ReadDword(OrientationFlags);
	stream->Seek( 8, GEM_CURRENT_POS ); // CResRef m_cNewPaletteRef in the original

	stream->ReadDword(tmp);  //this doesn't affect visibility
	ZOffset = int(tmp);

	stream->ReadDword(LightX); // and Lighting effect radius / width / glow
	stream->ReadDword(LightY);
	stream->ReadDword(LightZ); // glow intensity / brightness
	stream->ReadDword(Duration);
	// m_cVVCResRes in the original, supposedly a self-reference
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword(tmp); // 1 indexed, m_nStartSequence
	seq1 = ((signed) tmp) - 1;
	stream->ReadDword(tmp); // 1 indexed; 0 or less for none, m_nLoopSequence
	seq2 = ((signed) tmp) - 1;
	// original
	//   LONG    m_nCurrentSequence; //1 indexed
	//   DWORD   m_sequenceFlags; // only bit0 for continuous playback known
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadResRef( sounds[P_ONSET] );
	stream->ReadResRef( sounds[P_HOLD] );

	// original: CResRef   m_cAlphaBamRef;
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword(tmp); // m_nEndSequence
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

	if (sounds[P_HOLD].IsEmpty() && sounds[P_RELEASE].IsEmpty() && (SequenceFlags & IE_VVC_LOOP)) {
		sounds[P_HOLD] = sounds[P_ONSET];
		sounds[P_ONSET].Reset();
	}

	if (SequenceFlags & IE_VVC_BAM) {
		auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(Anim1ResRef, IE_BAM_CLASS_ID);
		if (!af) {
			Log(ERROR, "ScriptedAnimation", "Failed to load animation: {}!", Anim1ResRef);
			return;
		}
		for (AnimationFactory::index_t i = 0; i < MAX_ORIENT; i++) {
			AnimationFactory::index_t phaseHold = P_HOLD * MAX_ORIENT + i;

			if (seq1 >= 0) {
				AnimationFactory::index_t phaseOnset = P_ONSET * MAX_ORIENT + i;
				anims[phaseOnset] = PrepareAnimation(*af, static_cast<Animation::index_t>(seq1), i);
			}

			anims[phaseHold] = PrepareAnimation(*af, static_cast<Animation::index_t>(seq2), i, SequenceFlags & IE_VVC_LOOP);

			if (seq3 >= 0) {
				AnimationFactory::index_t phaseRelease = P_RELEASE * MAX_ORIENT + i;
				anims[phaseRelease] = PrepareAnimation(*af, static_cast<Animation::index_t>(seq3), i);
			}
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

void ScriptedAnimation::SetSound(int arg, const ResRef &sound)
{
	if (arg >= P_ONSET && arg <= P_RELEASE) {
		sounds[arg] = sound;
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

void ScriptedAnimation::SetFullPalette(const ResRef &PaletteResRef)
{
	palette = gamedata->GetPalette(PaletteResRef);
	if (twin) {
		twin->SetFullPalette(PaletteResRef);
	}
}

void ScriptedAnimation::SetFullPalette(int idx)
{
	ResRef PaletteResRef;
	PaletteResRef.Format("{:.7}{}", ResName, idx);
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

Animation::index_t ScriptedAnimation::GetCurrentFrame() const
{
	const Animation *anim = anims[P_HOLD * MAX_ORIENT];
	if (anim) {
		return anim->GetCurrentFrameIndex();
	}
	return 0;
}

ieDword ScriptedAnimation::GetSequenceDuration(ieDword multiplier) const
{
	//P_HOLD * MAX_ORIENT == MAX_ORIENT
	const Animation *anim = anims[P_HOLD * MAX_ORIENT];
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

void ScriptedAnimation::SetOrientation(orient_t orientation)
{
	if (NumOrientations > 1) {
		Orientation = orientation;
	} else {
		Orientation = S;
	}
	if (twin) {
		twin->Orientation = Orientation;
	}
}

bool ScriptedAnimation::UpdatePhase()
{
	const Game *game = core->GetGame();

	if (justCreated) {
		if (Phase == P_NOTINITED) {
			Log(ERROR, "ScriptedAnimation", "Not fully initialised VVC!");
			return true;
		}
		tick_t time = GetMilliseconds();
		if (starttime == 0) {
			starttime = time;
		}
		tick_t inc = 0;
		if ((time - starttime) >= tick_t(1000 / FrameRate)) {
			inc = (time - starttime) * FrameRate / 1000;
			starttime += inc * 1000 / FrameRate;
		}

		if (Delay >= inc) {
			Delay -= inc;
			return false;
		}
		Delay = 0;

		if (SequenceFlags & IE_VVC_LIGHTSPOT) {
			light = CreateLight(Size(LightX, LightY), LightZ);
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
		return false;
	}
	
	auto frame = anim->NextFrame();

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

void ScriptedAnimation::UpdateSound()
{
	if (Delay > 0 || SoundPhase > P_RELEASE) {
		return;
	}

	Point soundpos(Pos.x + XOffset, Pos.y + YOffset);
	if (!sound_handle || !sound_handle->Playing()) {
		while (SoundPhase <= P_RELEASE && sounds[SoundPhase].IsEmpty()) {
			SoundPhase++;
		}

		if (SoundPhase <= P_RELEASE) {
			bool loop = SoundPhase == P_HOLD && (SequenceFlags & IE_VVC_LOOP);
			sound_handle = core->GetAudioDrv()->Play(sounds[SoundPhase], SFX_CHAN_HITS, soundpos,
						   loop ? GEM_SND_LOOPING : 0);
			SoundPhase++;
		}
	} else {
		sound_handle->SetPos(soundpos);
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

bool ScriptedAnimation::UpdateDrawingState(orient_t orientation)
{
	if (!(OrientationFlags & IE_VVC_FACE_FIXED)) {
		SetOrientation(orientation);
	}
	
	if (twin) {
		twin->UpdateDrawingState(orientation);
	}
	
	if (UpdatePhase()) {
		//expired
		return true;
	}

	//delayed
	if (justCreated) {
		return false;
	}

	UpdateSound();
	
	return false;
}

//it is not sure if we need tint at all
void ScriptedAnimation::Draw(const Region &vp, Color tint, int height, BlitFlags flags) const
{
	if (twin) {
		twin->Draw(vp, tint, height, flags);
	}
	
	//delayed
	if (justCreated) {
		return;
	}
	
	if (Transparency & IE_VVC_TRANSPARENT) {
		flags |= BlitFlags::HALFTRANS;
	}
	if (Transparency & IE_VVC_SEPIA) {
		flags |= BlitFlags::SEPIA;
	}
	if (Transparency & IE_VVC_TINT) {
		flags |= BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD;
	}
	if (Transparency & IE_VVC_BLENDED) {
		flags |= BlitFlags::ONE_MINUS_DST;
	}

	//darken, greyscale, red tint are probably not needed if the global tint works
	//these are used in the original engine to implement weather/daylight effects
	//on the other hand
	
	if (Transparency & IE_VVC_NO_TIMESTOP) {
		flags &= ~BlitFlags::GREY;
	} else if (Transparency & IE_VVC_GREYSCALE) {
		flags |= BlitFlags::GREY;
	}

	if (flags & BlitFlags::COLOR_MOD) {
		ShaderTint(Tint, tint); // this tint is expected to already have the global tint applied
	}

	Point p(Pos.x - vp.x + XOffset, Pos.y - vp.y - ZOffset + YOffset);
	if (SequenceFlags & IE_VVC_HEIGHT) p.y -= height;

	if (SequenceFlags & IE_VVC_NOCOVER) {
		flags &= ~BlitFlags::STENCIL_MASK;
	}

	const Animation *anim = anims[Phase * MAX_ORIENT + Orientation];
	if (anim)
		VideoDriver->BlitGameSpriteWithPalette(anim->CurrentFrame(), palette, p, flags | BlitFlags::BLENDED, tint);

	if (light) {
		VideoDriver->BlitGameSprite(light, p, flags | BlitFlags::BLENDED, tint);
	}
}

Region ScriptedAnimation::DrawingRegion() const
{
	Region r = twin ? twin->DrawingRegion() : Region(Pos, Size());

	const Animation* anim = anims[Phase * MAX_ORIENT + Orientation];
	if (anim) {
		Region animArea = anim->animArea;
		animArea.x += XOffset + Pos.x;
		animArea.y += (YOffset - ZOffset) + Pos.y;
		r.ExpandToRegion(animArea);
	}
	
	if (light) {
		Region lightArea = light->Frame;
		lightArea.x = XOffset - light->Frame.x;
		lightArea.y = YOffset - ZOffset - light->Frame.y;
		r.ExpandToRegion(lightArea);
	}

	return r;
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
	Transparency |= IE_VVC_BLENDED;
	if (twin) {
		twin->SetBlend();
	}
}

void ScriptedAnimation::SetFade(ieByte initial, int speed)
{
	Tint = Color(255, 255, 255, initial);
	Fade=speed;
	Transparency|=BlitFlags::COLOR_MOD;
}

void ScriptedAnimation::GetPaletteCopy()
{
	if (palette)
		return;
	//it is not sure that the first position will have a resource in it
	//therefore the cycle
	for (const Animation *anim : anims) {
		if (!anim) continue;

		Holder<Sprite2D> spr = anim->GetFrame(0);
		if (!spr) continue;

		palette = spr->GetPalette()->Copy();
		Color shadowalpha = palette->col[1];
		shadowalpha.a /= 2; // FIXME: not sure if this should be /=2 or = 128 (they are probably the same value for all current uses);
		palette->CopyColorRange(&shadowalpha, &shadowalpha + 1, 1);
		//we need only one palette, so break here
		break;
	}
}

void ScriptedAnimation::AlterPalette(const RGBModifier& mod)
{
	GetPaletteCopy();
	if (!palette)
		return;
	palette->SetupGlobalRGBModification(palette, mod);
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
	if (ret->ZOffset >= 0) {
		ret->ZOffset = -1;
	}
	twin=NULL;
	return ret;
}

}
