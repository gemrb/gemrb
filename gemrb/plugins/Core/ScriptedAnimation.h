#ifndef SCRIPTEDANIMATION_H
#define SCRIPTEDANIMATION_H

#include "DataStream.h"
#include "Animation.h"

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
#define IE_VVC_GREYSCALE    0x00080000
#define IE_VVC_GLOWING      0x00200000
#define IE_VVC_RED_TINT		0x02000000

#define IE_VVC_LOOP			0x00000001

class GEM_EXPORT ScriptedAnimation
{
public:
	ScriptedAnimation(DataStream * stream, bool autoFree = true, long X = 0, long Y = 0);
	~ScriptedAnimation(void);
	Animation * anims[2];
	unsigned long Transparency;
	unsigned long SequenceFlags;
	long XPos, YPos, ZPos;
	unsigned long FrameRate;
	unsigned long FaceTarget;
	char Sounds[2][9];
};

#endif
