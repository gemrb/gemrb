#ifndef ACTOR_H
#define ACTOR_H

#include <vector>
#include "Animation.h"
#include "CharAnimations.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Actor
{
public:
	CharAnimations *anims;
	unsigned short XPos, YPos, XDes, YDes, Animation;
	unsigned char Orientation, Race, Class, Gender, Metal, Minor, Major, Skin, Leather, Armor, Hair;
	char CREResRef[8], *LongName, *ShortName;
	Actor(void);
	~Actor(void);
};
#endif
