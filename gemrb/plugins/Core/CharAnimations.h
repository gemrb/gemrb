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
public:
	Animation * Anims[18][16];
	Color Palette[256];
	unsigned long LoadedFlag;
	unsigned char OrientCount, MirrorType;
	unsigned char ArmorType, WeaponType, RangedType;
	char * ResRef;
public:
	CharAnimations(char * BaseResRef, unsigned char OrientCount, unsigned char MirrorType);
	~CharAnimations(void);
	Animation * GetAnimation(unsigned char AnimID, unsigned char Orient);
private:
	void AddVHRSuffix(char * ResRef, unsigned char AnimID, unsigned char &Cycle, unsigned char Orient)
	{
		switch(ArmorType)
			{
			case 0: // No Armor
				strcat(ResRef, "1");
			break;

			case 1: // Light Armor
				strcat(ResRef, "2");
			break;

			case 2: // Medium Armor
				strcat(ResRef, "3");
			break;

			case 3: // Heavy Armor
				strcat(ResRef, "4");
			break;
			}
		switch(AnimID)
			{
			//Attack is a special case... it cycles randomly
			//through SLASH, BACKSLASH and JAB so we will choose
			//which animation return randomly
			case IE_ANI_ATTACK:
			case IE_ANI_ATTACK_SLASH:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A1");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A3");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A7");
					break;
					}
				Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_ATTACK_BACKSLASH:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A2");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A4");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A8");
					break;
					}
				Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_ATTACK_JAB:
				{
				switch(WeaponType)
					{
					case IE_ANI_WEAPON_1H:
                        strcat(ResRef, "A3");
					break;

					case IE_ANI_WEAPON_2H:
						strcat(ResRef, "A5");
					break;

					case IE_ANI_WEAPON_2W:
						strcat(ResRef, "A9");
					break;
					}
				Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_AWAKE:
				{
				strcat(ResRef, "G1");
				Cycle = 9 + (Orient % 9);
				}
			break;

			case IE_ANI_CAST:
				{
				strcat(ResRef, "CA");
				Cycle = 9 + (Orient % 9);
				}
			break;

			case IE_ANI_CONJURE:
				{
				strcat(ResRef, "CA");
				Cycle = (Orient % 9);
				}
			break;

			case IE_ANI_DAMAGE:
				{
				strcat(ResRef, "G14");
				Cycle = 36 + (Orient % 9);
				}
			break;

			case IE_ANI_DIE:
				{
				strcat(ResRef, "G15");
				Cycle = 45 + (Orient % 9);
				}
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
			case IE_ANI_EMERGE:
				{
				strcat(ResRef, "G15");
				Cycle = 45 + (Orient % 9);
				}
			break;

			case IE_ANI_HEAD_TURN:
				{
				strcat(ResRef, "G12");
				Cycle = 18 + (Orient % 9);
				}
			break;

			//Unknown... maybe only a transparency effect apply
			case IE_ANI_HIDE:
				{

				}
			break;

			case IE_ANI_READY:
				{
				strcat(ResRef, "G13");
				Cycle = 27 + (Orient % 9);
				}
			break;

			//This depends on the ranged weapon equipped
			case IE_ANI_SHOOT:
				{
				switch(RangedType)
					{
					case IE_ANI_RANGED_BOW:
						{
						strcat(ResRef, "SA");
						Cycle = (Orient % 9);
						}
					break;

					case IE_ANI_RANGED_XBOW:
						{
						strcat(ResRef, "SX");
						Cycle = (Orient % 9);
						}
					break;
					
					case IE_ANI_RANGED_THROW:
						{
						strcat(ResRef, "SS");
						Cycle = (Orient % 9);
						}
					break;
					}
				}
			break;

			case IE_ANI_SLEEP:
				{
				strcat(ResRef, "G16");
				Cycle = 54 + (Orient % 9);
				}
			break;

			case IE_ANI_TWITCH:
				{
				strcat(ResRef, "G17");
				Cycle = 72 + (Orient % 9);
				}
			break;

			case IE_ANI_WALK:
				{
				strcat(ResRef, "G11");
				Cycle = (Orient % 9);
				}
			break;
			}
	}

	void GetAnimResRef(unsigned char AnimID, unsigned char Orient, char * ResRef, unsigned char & Cycle)
	{
		switch(MirrorType) 
			{
			case IE_ANI_CODE_MIRROR:
				{
					if(OrientCount == 9) {
						strcpy(ResRef, this->ResRef);
						AddVHRSuffix(ResRef, AnimID, Cycle, Orient);
						ResRef[8] = 0;
					}
					else {

					}
				}
			break;
			}
	}
};

#endif
