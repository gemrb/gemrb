#ifndef CHARANIMATIONS_H
#define CHARANIMATIONS_H

#include "Animation.h"
#include "../../includes/RGBAColor.h"
#include "TableMgr.h"
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

#define IE_ANI_ATTACK			0
#define IE_ANI_AWAKE			1
#define IE_ANI_CAST				2
#define IE_ANI_CONJURE			3
#define IE_ANI_DAMAGE			4
#define IE_ANI_DIE				5
#define IE_ANI_HEAD_TURN		6
#define IE_ANI_READY			7
#define IE_ANI_SHOOT			8
#define IE_ANI_TWITCH			9
#define IE_ANI_WALK				10
#define IE_ANI_ATTACK_SLASH		11
#define IE_ANI_ATTACK_BACKSLASH	12
#define IE_ANI_ATTACK_JAB		13
#define IE_ANI_EMERGE			14
#define IE_ANI_HIDE				15
#define IE_ANI_SLEEP			16

#define IE_ANI_CODE_MIRROR		0
#define IE_ANI_ONE_FILE			1
#define IE_ANI_TWO_FILES		2
#define IE_ANI_FOUR_FILES		3

#define IE_ANI_NO_ARMOR			0
#define IE_ANI_LIGHT_ARMOR		1
#define IE_ANI_MEDIUM_ARMOR		2
#define IE_ANI_HEAVY_ARMOR		3

#define IE_ANI_WEAPON_1H		0
#define IE_ANI_WEAPON_2H		1
#define IE_ANI_WEAPON_2W		2

#define IE_ANI_RANGED_BOW		0
#define IE_ANI_RANGED_XBOW		1
#define IE_ANI_RANGED_THROW		2


class GEM_EXPORT CharAnimations
{
private:
	Animation * Anims[18][16];
	Color Palette[256];
public:
	unsigned long LoadedFlag, RowIndex;
	unsigned char OrientCount, MirrorType;
	unsigned char ArmorType, WeaponType, RangedType;
	char * ResRef;
	TableMgr * Avatars;
public:
	CharAnimations(char * BaseResRef, unsigned char OrientCount, unsigned char MirrorType, int RowIndex);
	~CharAnimations(void);
	Animation * GetAnimation(unsigned char AnimID, unsigned char Orient);
	void SetNewPalette(Color * Pal)
	{
		memcpy(Palette, Pal, 256*sizeof(Color));
		for(int i = 0; i < 18; i++) {
			for(int j = 0; j < 16; j++) {
				if(Anims[i][j])
					Anims[i][j]->ChangePalette = true;
			}
		}
	}
private:
	void AddVHRSuffix(char * ResRef, unsigned char AnimID, unsigned char &Cycle, unsigned char Orient);
	void GetAnimResRef(unsigned char AnimID, unsigned char Orient, char * ResRef, unsigned char & Cycle)
	{
		switch(MirrorType) 
			{
			case IE_ANI_CODE_MIRROR:
				{
					if(OrientCount == 9) {
						AddVHRSuffix(ResRef, AnimID, Cycle, Orient);
						ResRef[8] = 0;
					}
					else {
						
					}
				}
			break;

			case IE_ANI_TWO_FILES:
				{
					if(OrientCount == 5) {
						char * val = Avatars->QueryField(RowIndex, AnimID+3);
						if(val[0] == '*') {
							ResRef[0] = 0;
							return;
						}
						Cycle = atoi(val) + (Orient/2);
						switch(AnimID) {
							case IE_ANI_ATTACK:
							case IE_ANI_ATTACK_BACKSLASH:
							case IE_ANI_ATTACK_SLASH:
							case IE_ANI_ATTACK_JAB:
							case IE_ANI_CAST:
							case IE_ANI_CONJURE:
							case IE_ANI_SHOOT:
								strcat(ResRef, "G2");
							break;

							default:
								strcat(ResRef, "G1");
							break;
						}
						if(Orient > 10) {
							strcat(ResRef, "E");
						}
					}
				}
			break;
			}
	}
};

#endif
