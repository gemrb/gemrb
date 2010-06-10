/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file globals.h
 * Some global definitions and includes
 * @author The GemRB Project
 */


#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ie_types.h"

#define VERSION_GEMRB "0.6.0"

#define GEMRB_STRING "GemRB v" VERSION_GEMRB

#ifndef GLOBALS_ONLY_DEFS

#include "RGBAColor.h"
#include "SClassID.h"
#include "errors.h"
#include "win32def.h"

#include "AnimStructures.h"
#include "DataStream.h"
#include "Region.h"
#include "Sprite2D.h"
#include "VideoMode.h"
#include "VideoModes.h"

#include <cstdio>
#include <cstdlib>

#endif //GLOBALS_ONLY_DEFS

//Global Variables

#define IE_NORMAL 0
#define IE_SHADED 1

#define IE_STR_STRREFON   1
#define IE_STR_SOUND      2
#define IE_STR_SPEECH     4
#define IE_STR_ALLOW_ZERO 8     //0 strref is allowed
#define IE_STR_STRREFOFF  256
#define IE_STR_REMOVE_NEWLINE  0x1000 // gemrb extension: remove extraneus ending newline

// bitflag operations
// !!! Keep these synchronized with GUIDefines.py !!!
#define BM_SET  0 //gemrb extension
#define BM_AND  1
#define BM_OR   2
#define BM_XOR  3
#define BM_NAND 4 //gemrb extension

//IDS Importer Defines
#define IDS_VALUE_NOT_LOCATED -65535 // GetValue returns this if text is not found in arrays ... this needs to be a unique number that does not exist in the value[] array
#define GEM_ENCRYPTION_KEY "\x88\xa8\x8f\xba\x8a\xd3\xb9\xf5\xed\xb1\xcf\xea\xaa\xe4\xb5\xfb\xeb\x82\xf9\x90\xca\xc9\xb5\xe7\xdc\x8e\xb7\xac\xee\xf7\xe0\xca\x8e\xea\xca\x80\xce\xc5\xad\xb7\xc4\xd0\x84\x93\xd5\xf0\xeb\xc8\xb4\x9d\xcc\xaf\xa5\x95\xba\x99\x87\xd2\x9d\xe3\x91\xba\x90\xca"

/////feature flags
#define  GF_HAS_KAPUTZ           	0 //pst
#define  GF_ALL_STRINGS_TAGGED   	1 //bg1, pst, iwd1
#define  GF_HAS_SONGLIST        	2 //bg2
#define  GF_TEAM_MOVEMENT       	3 //pst
#define  GF_UPPER_BUTTON_TEXT   	4 //bg2
#define  GF_LOWER_LABEL_TEXT    	5 //bg2
#define  GF_HAS_PARTY_INI       	6 //iwd2
#define  GF_SOUNDFOLDERS        	7 //iwd2
#define  GF_IGNORE_BUTTON_FRAMES	8 // all?
#define  GF_ONE_BYTE_ANIMID     	9 // pst
#define  GF_HAS_DPLAYER         	10 // not pst
#define  GF_HAS_EXPTABLE        	11 // iwd, iwd2
#define  GF_HAS_BEASTS_INI      	12 //pst; also for quests.ini
#define  GF_HAS_DESC_ICON       	13 //bg
#define  GF_HAS_PICK_SOUND      	14 //pst
#define  GF_IWD_MAP_DIMENSIONS  	15 //iwd, iwd2
#define  GF_AUTOMAP_INI         	16 //pst
#define  GF_SMALL_FOG           	17 //bg1, pst
#define  GF_REVERSE_DOOR        	18 //pst
#define  GF_PROTAGONIST_TALKS   	19 //pst
#define  GF_HAS_SPELLLIST       	20 //iwd2
#define  GF_IWD2_SCRIPTNAME     	21 //iwd2, iwd, how
#define  GF_DIALOGUE_SCROLLS    	22 //pst
#define  GF_KNOW_WORLD          	23 //iwd2
#define  GF_REVERSE_TOHIT       	24 //all except iwd2
#define  GF_SAVE_FOR_HALF       	25 //pst
#define  GF_CHARNAMEISGABBER   		26 //iwd2
#define  GF_MAGICBIT            	27 //iwd, iwd2
#define  GF_CHECK_ABILITIES     	28 //bg2 (others?)
#define  GF_CHALLENGERATING     	29 //iwd2
#define  GF_SPELLBOOKICONHACK   	30 //bg2
#define  GF_ENHANCED_EFFECTS    	31 //iwd2 (maybe iwd/how too)
#define  GF_DEATH_ON_ZERO_STAT  	32 //not in iwd2
#define  GF_SPAWN_INI           	33 //pst, iwd, iwd2
#define  GF_IWD2_DEATHVARFORMAT  	34 //iwd branch (maybe pst)
#define  GF_RESDATA_INI         	35 //pst
#define  GF_OVERRIDE_CURSORPOS  	36 //pst, iwd2
#define  GF_BREAKABLE_WEAPONS     	37 //bg1, not bg2 and iwd2, maybe others
#define  GF_3ED_RULES              	38 //iwd2
#define  GF_LEVELSLOT_PER_CLASS    	39 //iwd2
#define  GF_SELECTIVE_MAGIC_RES    	40 //bg2, iwd2, (how)
#define  GF_HAS_HIDE_IN_SHADOWS    	41 // not in bg1 and pst
#define  GF_AREA_VISITED_VAR    	42 //iwd, iwd2
#define  GF_PROPER_BACKSTAB     	43 //bg2, iwd2, how?
#define  GF_ONSCREEN_TEXT       	44 //pst
#define  GF_SPECIFIC_DMG_BONUS		45 //how, iwd2
#define  GF_STRREF_SAVEGAME       	46 //iwd2
//update this
#define GF_COUNT 47

//the number of item usage fields (used in CREItem and STOItem)
#define CHARGE_COUNTERS  3

/////AI global defines
#define AI_UPDATE_TIME	15

/////maximum animation orientation count (used in many places)
#define MAX_ORIENT				16

/////globally used functions

class Scriptable;
class Actor;

/* this function will work with pl/cz special characters */

extern unsigned char pl_uppercase[256];
extern unsigned char pl_lowercase[256];

GEM_EXPORT void strnlwrcpy(char* d, const char *s, int l);
GEM_EXPORT void strnuprcpy(char* d, const char *s, int l);
GEM_EXPORT void strnspccpy(char* d, const char *s, int l);
#ifndef HAVE_STRNLEN
GEM_EXPORT int strnlen(const char* string, int maxlen);
#endif
GEM_EXPORT unsigned char GetOrient(const Point &s, const Point &d);
GEM_EXPORT unsigned int Distance(const Point pos, const Point pos2);
GEM_EXPORT unsigned int Distance(const Point pos, Scriptable *b);
GEM_EXPORT unsigned int PersonalDistance(const Point pos, Scriptable *b);
GEM_EXPORT unsigned int Distance(Scriptable *a, Scriptable *b);
GEM_EXPORT unsigned int PersonalDistance(Scriptable *a, Scriptable *b);
GEM_EXPORT int EARelation(Scriptable *a, Actor *b);
GEM_EXPORT bool dir_exists(const char* path);
GEM_EXPORT int strlench(const char* string, char ch);
#ifndef HAVE_STRNDUP
GEM_EXPORT char* strndup(const char* s, size_t l);
#endif

#ifndef WIN32
GEM_EXPORT char* strupr(char* string);
GEM_EXPORT char* strlwr(char* string);
#endif

#ifdef WIN32
#define GetTime(store) store = GetTickCount()
#else
#include <sys/time.h>
#define GetTime(store) \
	{ \
		struct timeval tv; \
		gettimeofday(&tv, NULL); \
		store = (tv.tv_usec/1000) + (tv.tv_sec*1000); \
	}
#endif

//struct ActorBlock;

inline int MIN(int a, int b)
{
	return (a > b ? b : a);
}

inline int MAX(int a, int b)
{
	return (a < b ? b : a);
}

inline bool valid_number(const char* string, long& val)
{
        char* endpr;

        val = (long) strtoul( string, &endpr, 0 );
        return ( const char * ) endpr != string;
}

//we need 32+6 bytes at least, because we store 'context' in the variable
//name too
#define MAX_VARIABLE_LENGTH  40

#endif //! GLOBALS_H

