#ifndef CHARANIMATIONS_H
#define CHARANIMATIONS_H

#include "Animation.h"
#include "../../includes/RGBAColor.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#define IE_ANI_STAND	0
#define IE_ANI_WALK		1
#define IE_ANI_ATTACK	2
#define IE_ANI_CAST		3
#define IE_ANI_TAKEHIT	4
#define IE_ANI_DEAD		5
#define IE_ANI_SLEEP	6

#define IE_ANI_MIRROR				0x00000001
#define IE_ANI_1H_BLUNT				0x00000002
#define IE_ANI_2H_BLUNT				0x00000004
#define IE_ANI_1H_SLASH				0x00000008
#define IE_ANI_2H_SLASH				0x00000010
#define IE_ANI_1H_PIERCE			0x00000020
#define IE_ANI_2H_PIERCE			0x00000040
#define IE_ANI_2W_BLUNT				0x00000080 //NOT SURE
#define IE_ANI_2W_SLASH				0x00000100 //NOT SURE
#define IE_ANI_2W_PIERCE			0x00000200 //NOT SURE
// I cannot really understand how the Casting Animations are coded... 
// The last Idea was that they are one per Magic School...
/*
#define IE_ANI_CAST_STANDARD		0x00000400
#define IE_ANI_CAST_ISTANT			0x00000800
#define IE_ANI_CAST_DIVINE			0x00001000 //NOT SURE
#define IE_ANI_CAST_DIVINE_INST		0x00002000 //NOT SURE
#define IE_ANI_CAST_LONG			0x00004000
*/
#define IE_ANI_CAST_ABJURATION		0x00000400
#define IE_ANI_CAST_CONJURATION		0x00000800
#define IE_ANI_CAST_DIVINATION		0x00001000
#define IE_ANI_CAST_ENCHANTMENT		0x00002000
#define IE_ANI_CAST_ILLUSIONISM		0x00004000
#define IE_ANI_CAST_EVOCATION		0x00008000
#define IE_ANI_CAST_NECROMANCY		0x00010000
#define IE_ANI_CAST_TRANSMUTATION	0x00020000

#define IE_ANI_WALK_DISARMED		0x00040000

#define IE_ANI_STAND_LOOKING		0x00080000
#define IE_ANI_STAND_2H_WEAPON		0x00100000

#define IE_ANI_TAKE_HIT_DISARMED	0x00200000
#define IE_ANI_TAKE_HIT_1H_WEAPON	0x00400000

#define IE_ANI_COMPOSED_DEAD		0x00800000

#define IE_ANI_STAND_DISARMED       0x01000000
#define IE_ANI_STAND_YAWNING		0x02000000

#define IE_ANI_DIEING				0x04000000

#define IE_ANI_BOW					0x08000000
#define IE_ANI_LAUNCH				0x10000000
#define IE_ANI_XBOW					0x20000000

#define IE_ANI_MIDRES				0x40000000
#define IE_ANI_LOWRES				0x80000000

#define IE_ANI_ALL					0x3FFFFFFF

class GEM_EXPORT CharAnimations
{
public:
	Animation * Anims[30][16];
	Color Palette[256];
	unsigned long LoadedFlag;
	unsigned long SupportedAnims;
	char * ResRef;
public:
	CharAnimations(char * BaseResRef, unsigned long AniType);
	~CharAnimations(void);
	Animation * GetAnimation(unsigned char AnimID, unsigned char Orient);
};

#endif
