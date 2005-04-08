#ifndef SCRIPTEDANIMATION_H
#define SCRIPTEDANIMATION_H

#include "DataStream.h"
#include "AnimationFactory.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#define IE_VVC_TRANSPARENT	0x00000002
#define IE_VVC_BRIGHTEST	0x00000008
#define IE_VVC_GREYSCALE	0x00080000
#define IE_VVC_GLOWING  	0x00200000
#define IE_VVC_RED_TINT		0x02000000

#define IE_VVC_LOOP			0x00000001

class GEM_EXPORT ScriptedAnimation {
public:
	ScriptedAnimation(AnimationFactory *af, Point &position);
	ScriptedAnimation(DataStream* stream, Point &position, bool autoFree = true);
	~ScriptedAnimation(void);
	Animation* anims[2];
	ieDword Transparency;
	ieDword SequenceFlags;
	ieDword XPos, YPos, ZPos;
	ieDword FrameRate;
	ieDword FaceTarget;
	ieResRef Sounds[2];
	bool justCreated;
	bool autoSwitchOnEnd;
};

#endif
