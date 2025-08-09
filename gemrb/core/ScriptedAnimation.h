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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#ifndef SCRIPTEDANIMATION_H
#define SCRIPTEDANIMATION_H

#include "exports.h"

#include "Animation.h"
#include "Orientation.h"

#include "Audio/Playback.h"

namespace GemRB {

class AnimationFactory;
class DataStream;
class Sprite2D;

//scripted animation flags
#define S_ANI_PLAYONCE Animation::Flags::Once

// originals: bg2 is the only one with VVC files and they use only IE_VVC_BLENDED, IE_VVC_MIRRORX, IE_VVC_MIRRORY
// bg(2)ee: also IE_VVC_3D_BLEND, IE_VVC_TRANSPARENT, IE_VVC_NOCOVER_2
// transparent 1, translucent 2
#define IE_VVC_TRANSPARENT 0x00000002
// translucent shadow 4
#define IE_VVC_BLENDED 0x00000008
#define IE_VVC_MIRRORX 0x00000010 // TODO: implement
#define IE_VVC_MIRRORY 0x00000020 // TODO: implement
#define IE_VVC_CLIPPED 0x00000040
// IE_VVC_COPYFROMBACK  	0x00000080
// IE_VVC_CLEARFILL  	0x00000100
#define IE_VVC_3D_BLEND    0x00000200 // CVEFVIDCELL_BLT_GLOW in the original TODO: implement
#define IE_VVC_NOCOVER_2   0x00000400
#define IE_VVC_NO_TIMESTOP 0x00000800 //ignore timestop palette
#define IE_VVC_NO_SEPIA    0x00001000 //ignore dream palette
#define IE_VVC_2D_BLEND    0x00002000
// 0x4000, 0x8000 unused

// start of Colour flags on iesdp (though same field in original bg2)
// originals: IE_VVC_GREYSCALE, IE_VVC_GLOWING, IE_VVC_SEPIA
// bg(2)ee: also IE_VVC_NOT_LIGHT_SOURCE, IE_VVC_LIGHT_SOURCE, IE_VVC_INT_BRIGHTNESS, IE_VVC_FULL_PALETTE
// IE_VVC_NOT_LIGHT_SOURCE 0x00010000 // Not light source TODO: implement
// IE_VVC_LIGHT_SOURCE 0x00020000 // Light source TODO: implement
#define IE_VVC_TINT 0x00030000 //2 bits need to be set for tint (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD)
// IE_VVC_INT_BRIGHTNESS 0x00040000 // Internal brightness TODO: implement
#define IE_VVC_GREYSCALE 0x00080000 //timestopped palette
//#define IE_VVC_DARKEN    0x00100000 // unused
#define IE_VVC_GLOWING 0x00200000 // internal gamma TODO: implement
// 0x00400000 Non-reserved palette
// IE_VVC_FULL_PALETTE 0x00800000 // Full palette TODO: implement
// 0x01000000 Unused
#define IE_VVC_SEPIA 0x02000000 //dream palette

// sequence flags
// originals, bg(2)ee used: IE_VVC_LOOP - IE_VVC_BAM, IE_VVC_NOCOVER - IE_VVC_HIGH_BRIGHTEN
#define IE_VVC_LOOP      0x00000001
#define IE_VVC_LIGHTSPOT 0x00000002 //draw lightspot; CVEFVIDCELL_GLOW
#define IE_VVC_HEIGHT    0x00000004
#define IE_VVC_BAM       0x00000008
// #define IE_VVC_OWN_PAL   0x00000010 // unused
// original: CVEFVIDCELL_DELETED  	0x00000020 //Used by linked effect
//#define IE_VVC_PURGEABLE	0x00000020 // purge on next ai update; uses unknown
#define IE_VVC_NOCOVER       0x00000040 // CVEFVIDCELL_DONOTCLIP
#define IE_VVC_MID_BRIGHTEN  0x00000080 // CVEFVIDCELL_BRIGHTEN3DONLYOFF TODO: implement
#define IE_VVC_HIGH_BRIGHTEN 0x00000100 // CVEFVIDCELL_BRIGHTENIFFAST TODO: implement


//#define IE_VVC_UNUSED           0xe0000000U
//gemrb specific sequence flags
#define IE_VVC_FREEZE 0x80000000
#define IE_VVC_STATIC 0x40000000 // position doesn't update with actor movement

//orientation flags
#define IE_VVC_FACE_TARGET           0x00000001
#define IE_VVC_FACE_TARGET_DIRECTION 0x00000002 // follow target
#define IE_VVC_FACE_TRAVEL_DIRECTION 0x00000004 // follow path
#define IE_VVC_FACE_FIXED            0x00000008

//phases
#define P_NOTINITED -1
#define P_ONSET     0
#define P_HOLD      1
#define P_RELEASE   2

class GEM_EXPORT ScriptedAnimation {
public:
	ScriptedAnimation() noexcept = default;
	ScriptedAnimation(const ScriptedAnimation&) = delete;
	~ScriptedAnimation();
	ScriptedAnimation& operator=(const ScriptedAnimation&) = delete;
	explicit ScriptedAnimation(DataStream* stream);
	void LoadAnimationFactory(const AnimationFactory& af, int gettwin = 0);
	//there are 3 phases: start, hold, release
	//it will usually cycle in the 2. phase
	//the anims could also be used 'orientation based' if NumOrientations is
	//set to 5, 9, 16
	Animation* anims[3 * MAX_ORIENT] {};
	//there is only one palette
	Holder<Palette> palette;
	ResRef sounds[3];
	Color Tint = ColorWhite;
	int Fade = 0;
	ieDword Transparency = 0;
	ieDword SequenceFlags = 0;
	int Dither = 0;
	Point Pos; // position of the effect in game coordinates
	int XOffset = 0, YOffset = 0, ZOffset = 0; // orientation relative to Pos
	ieDword LightX = 0, LightY = 0, LightZ = 0;
	Holder<Sprite2D> light = nullptr; // this is just a round/halftrans sprite, has no animation
	ieDword FrameRate = ANI_DEFAULT_FRAMERATE;
	ieDword NumOrientations = 0;
	orient_t Orientation = S;
	ieDword OrientationFlags = 0;
	ieDword Duration = 0xffffffff;
	ieDword Delay = 0;
	bool justCreated = true;
	ResRef ResName;
	int Phase = P_NOTINITED;
	int SoundPhase = P_NOTINITED;
	ScriptedAnimation* twin = nullptr;
	bool active = true;
	bool effect_owned = false;
	Holder<PlaybackHandle> soundHandle;
	tick_t starttime = 0;

public:
	//draws the next frame of the videocell
	bool UpdateDrawingState(orient_t orientation);
	void Draw(const Region& vp, Color tint, int height, BlitFlags flags) const;
	Region DrawingRegion() const;
	//sets phase (0-2)
	void SetPhase(int arg);
	//sets sound for phase (p_onset, p_hold, p_release)
	void SetSound(int arg, const ResRef& sound);
	//sets the animation to play only once
	void PlayOnce();
	//sets gradient colour slot to gradient
	void SetPalette(int gradient, int start = -1);
	//sets complete palette to ResRef
	void SetFullPalette(const ResRef& PaletteResRef);
	//sets complete palette to own name+index
	void SetFullPalette(int idx);
	Animation::index_t GetCurrentFrame() const;
	ieDword GetSequenceDuration(ieDword multiplier) const;
	/* sets up a delay in the beginning of the vvc */
	void SetDelay(ieDword delay);
	/* sets default duration if it wasn't set yet */
	void SetDefaultDuration(ieDword duration);
	/* sets up the direction of the vvc */
	void SetOrientation(orient_t orientation);
	/* transforms vvc to blended */
	void SetBlend();
	/* sets the position */
	void SetPos(const Point& pos);
	/* sets the effect owned flag */
	void SetEffectOwned(bool flag);
	/* sets fade effect at end of animation (pst feature) */
	void SetFade(ieByte initial, int speed);
	/* alters palette with rgb factor */
	void AlterPalette(const RGBModifier& rgb);
	/* returns possible twin after altering it to become underlay */
	ScriptedAnimation* DetachTwin();

private:
	Animation* PrepareAnimation(const AnimationFactory& af, Animation::index_t cycle, Animation::index_t i, bool loop = false) const;
	bool UpdatePhase();
	void GetPaletteCopy();
	void IncrementPhase();
	/* stops any sound playing */
	void StopSound();
	/* updates the sound playing */
	void UpdateSound();
};

}

#endif
