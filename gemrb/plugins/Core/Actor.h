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

/** USING DEFINITIONS AS DESCRIBED IN STATS.IDS */
#include "../../includes/ie_stats.h"
#define MAX_STATS 256
#define MAX_SCRIPTS 8

class GEM_EXPORT Actor
{
public:
	unsigned short XPos, YPos, XDes, YDes, Animation;
	unsigned char Orientation;
        long BaseStats[MAX_STATS];
        long Modified[MAX_STATS];
	CharAnimations *anims;
	unsigned char Race, Class, Gender, Metal, Minor, Major, Skin, Leather, Armor, Hair;
	char CREResRef[8], *LongName, *ShortName;
        char  ScriptName[33]; //death variable
        char  Scripts[MAX_SCRIPTS][9];
        char  Dialog[9];
        char  Icon[9];
public:
	Actor(void);
	~Actor(void);

	/** returns the animations */
	CharAnimations *GetAnims();
        /** Inits the Modified vector */
        void Init();
        /** Returns a Stat value */
        long  GetStat(unsigned char StatIndex);
        /** Returns the difference */
        long  GetMod(unsigned char StatIndex);
        /** Returns a Stat Base Value */
        long  GetBase(unsigned char StatIndex);
        /** Sets a Stat Base Value */
        bool  SetBase(unsigned char StatIndex, long Value);
        /** Sets the modified value in different ways, returns difference */
        int   NewMod(unsigned char StatIndex, long ModifierValue, long ModifierType);
        /** Sets the Scripting Name (death variable) */
        void  SetScriptName(const char * string)
        {
                if(string == NULL)
                        return;
                strncpy(ScriptName, string, 32);
        }
        /** Sets a Script ResRef */
        void  SetScript(int ScriptIndex, const char * ResRef)
        {
                if(ResRef == NULL)
                        return;
                if(ScriptIndex>=MAX_SCRIPTS)
                        return;
                strncpy(Scripts[ScriptIndex], ResRef, 8);
        }
        /** Sets the Dialog ResRef */
        void  SetDialog(const char * ResRef)
        {
                if(ResRef == NULL)
                        return;
                strncpy(Dialog, ResRef, 8);
        }
        /** Sets the Icon ResRef */
        void  SetIcon(const char * ResRef)
        {
                if(ResRef == NULL)
                        return;
                strncpy(Icon, ResRef, 8);
        }
        /** Gets the Character Long Name/Short Name */
        char *GetName(int which)
        {
		if(which) return LongName;
                return ShortName;
        }
        /** Gets the DeathVariable */
        char *GetScriptName(void)
        {
                return ScriptName;
        }
        /** Gets a Script ResRef */
        char *GetScript(int ScriptIndex)
        {
                return Scripts[ScriptIndex];
        }
        /** Gets the Dialog ResRef */
        char *GetDialog(void)
        {
                return Dialog;
        }
        /** Gets the Icon ResRef */
        char *GetIcon(void)
        {
                return Icon;
        }

};
#endif
