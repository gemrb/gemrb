#ifndef SCRIPTEDANIMATION_H
#define SCRIPTEDANIMATION_H

#include "DataStream.h"
#include "AnimationFactory.h"
#include "Palette.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//scripted animation flags 
#define S_ANI_PLAYONCE        8        //(same as area animation)

#define IE_VVC_TRANSPARENT	0x00000002
#define IE_VVC_BRIGHTEST	0x00000008
#define IE_VVC_GREYSCALE	0x00080000
#define IE_VVC_GLOWING  	0x00200000
#define IE_VVC_RED_TINT		0x02000000

#define IE_VVC_LOOP			0x00000001

//phases
#define P_ONSET   0
#define P_HOLD    1
#define P_RELEASE 2

class GEM_EXPORT ScriptedAnimation {
public:
	ScriptedAnimation(AnimationFactory *af, Point &position);
	ScriptedAnimation(DataStream* stream, Point &position, bool autoFree = true);
	~ScriptedAnimation(void);
	//there are 3 phases: start, hold, release
	//it will usually cycle in the 2. phase
	Animation* anims[3];
	Palette *palettes[3];
	ieResRef sounds[3];
	ieDword Transparency;
	ieDword SequenceFlags;
	ieDword XPos, YPos, ZPos;
	ieDword FrameRate;
	ieDword FaceTarget;
	bool justCreated;
	ieResRef ResName;
	int Phase;
public:
	//draws the next frame of the videocell
	bool Draw(Region &screen, Point &Pos, Color &tint);
	//sets phase (0-2)
	void SetPhase(int arg);
	//sets sound for phase (p_onset, p_hold, p_release)
	void SetSound(int arg, const ieResRef sound);
	//sets gradient colour slot to gradient
	void SetPalette(int gradient, int start=-1);
};

#endif
