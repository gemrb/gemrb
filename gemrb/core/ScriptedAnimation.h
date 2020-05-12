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

#include "Audio.h"
#include "Palette.h"
#include "SpriteCover.h"

namespace GemRB {

class Animation;
class AnimationFactory;
class DataStream;
class Map;
class Sprite2D;

//scripted animation flags 
#define S_ANI_PLAYONCE        8        //(same as area animation)

// transparent 1, translucent 2
#define IE_VVC_TRANSPARENT	0x00000002
// translucent shadow 4
#define IE_VVC_BLENDED		0x00000008
#define IE_VVC_MIRRORX    	0x00000010
#define IE_VVC_MIRRORY   	0x00000020
#define IE_VVC_CLIPPED  	0x00000040
// IE_VVC_COPYFROMBACK  	0x00000080
// IE_VVC_CLEARFILL  	0x00000100
#define IE_VVC_3D_BLEND 	0x00000200 // CVEFVIDCELL_BLT_GLOW in the original
#define IE_VVC_NOCOVER_2  	0x00000400
#define IE_VVC_NO_TIMESTOP      0x00000800   //ignore timestop palette
#define IE_VVC_NO_SEPIA         0x00001000   //ignore dream palette
#define IE_VVC_2D_BLEND         0x00002000
// 0x4000, 0x8000 unused
// start of Colour flags on iesdp (though same field in original bg2)
// 0x10000 Not light source
// 0x20000 Light source
#define IE_VVC_TINT     	0x00030000   //2 bits need to be set for tint
// 0x40000 Internal brightness
#define IE_VVC_GREYSCALE	0x00080000   //timestopped palette
#define IE_VVC_DARKEN       0x00100000   // unused
#define IE_VVC_GLOWING  	0x00200000   //internal gamma
// 0x00400000 Non-reserved palette
// 0x00800000 Full palette
// 0x01000000 Unused
#define IE_VVC_SEPIA		0x02000000   //dream palette

#define IE_VVC_LOOP		0x00000001
#define IE_VVC_LIGHTSPOT        0x00000002   //draw lightspot; CVEFVIDCELL_GLOW
#define IE_VVC_HEIGHT           0x00000004
#define IE_VVC_BAM		0x00000008
#define IE_VVC_OWN_PAL  	0x00000010
// original: CVEFVIDCELL_DELETED  	0x00000020 //Used by linked effect
//#define IE_VVC_PURGEABLE	0x00000020 // purge on next ai update; uses unknown
#define IE_VVC_NOCOVER		0x00000040 // CVEFVIDCELL_DONOTCLIP
#define IE_VVC_MID_BRIGHTEN 	0x00000080 // CVEFVIDCELL_BRIGHTEN3DONLYOFF
#define IE_VVC_HIGH_BRIGHTEN	0x00000100 // CVEFVIDCELL_BRIGHTENIFFAST


//#define IE_VVC_UNUSED           0xe0000000U
//gemrb specific sequence flags
#define IE_VVC_FREEZE     0x80000000

//phases
#define P_NOTINITED -1
#define P_ONSET   0
#define P_HOLD    1
#define P_RELEASE 2

class GEM_EXPORT ScriptedAnimation {
public:
	ScriptedAnimation();
	~ScriptedAnimation(void);
	ScriptedAnimation(DataStream* stream);
	void Init();
	void LoadAnimationFactory(AnimationFactory *af, int gettwin = 0);
	void Override(ScriptedAnimation *templ);
	//there are 3 phases: start, hold, release
	//it will usually cycle in the 2. phase
	//the anims could also be used 'orientation based' if FaceTarget is
	//set to 5, 9, 16
	Animation* anims[3*MAX_ORIENT];
	//there is only one palette
	Palette *palette;
	ieResRef sounds[3];
	ieResRef PaletteName;
	Color Tint;
	int Fade;
	ieDword Transparency;
	ieDword SequenceFlags;
	int Dither;
	//these are signed
	int XPos, YPos, ZPos;
	ieDword LightX, LightY, LightZ;
	Sprite2D* light;//this is just a round/halftrans sprite, has no animation
	ieDword FrameRate;
	ieDword FaceTarget;
	ieByte Orientation;
	ieDword Duration;
	ieDword Delay;
	bool justCreated;
	ieResRef ResName;
	int Phase;
	SpriteCover* cover;
	ScriptedAnimation *twin;
	bool active;
	bool effect_owned;
	Holder<SoundHandle> sound_handle;
	unsigned long starttime;
public:
	//draws the next frame of the videocell
	bool Draw(const Region &screen, const Point &Pos, const Color &tint, Map *area, int dither, int orientation, int height);
	//sets phase (0-2)
	void SetPhase(int arg);
	//sets sound for phase (p_onset, p_hold, p_release)
	void SetSound(int arg, const ieResRef sound);
	//sets the animation to play only once
	void PlayOnce();
	//sets gradient colour slot to gradient
	void SetPalette(int gradient, int start=-1);
	//sets complete palette to ResRef
	void SetFullPalette(const ieResRef PaletteResRef);
	//sets complete palette to own name+index
	void SetFullPalette(int idx);
	//sets spritecover
	void SetSpriteCover(SpriteCover* c) { delete cover; cover = c; }
	/* get stored SpriteCover */
	SpriteCover* GetSpriteCover() const { return cover; }
	int GetCurrentFrame();
	ieDword GetSequenceDuration(ieDword multiplier);
	/* sets up a delay in the beginning of the vvc */
	void SetDelay(ieDword delay);
	/* sets default duration if it wasn't set yet */
	void SetDefaultDuration(ieDword duration);
	/* sets up the direction of the vvc */
	void SetOrientation(int orientation);
	/* transforms vvc to blended */
	void SetBlend();
	/* sets the effect owned flag */
	void SetEffectOwned(bool flag);
	/* sets fade effect at end of animation (pst feature) */
	void SetFade(ieByte initial, int speed);
	/* alters palette with rgb factor */
	void AlterPalette(const RGBModifier &rgb);
	/* returns possible twin after altering it to become underlay */
	ScriptedAnimation *DetachTwin();
private:
	void PrepareAnimation(Animation *anim, ieDword Transparency);
	void PreparePalette();
	bool HandlePhase(Sprite2D *&frame);
	void GetPaletteCopy();
};

}

#endif
