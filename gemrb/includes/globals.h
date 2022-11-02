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

#include "ie_types.h"
#include <type_traits>

#include <cmath>

#define VERSION_GEMRB "0.9.1-git"

#define GEMRB_STRING "GemRB v" VERSION_GEMRB
#define PACKAGE "GemRB"

#include "RGBAColor.h"
#include "errors.h"

#include "Region.h"
#include "Streams/DataStream.h"

#include <algorithm>
#include <bitset>
#include <climits>
#include <chrono>
#include <memory>

namespace GemRB {

//Global Variables

/////feature flags
enum GameFeatureFlags : uint32_t {
	GF_HAS_KAPUTZ,          			//pst
	GF_ALL_STRINGS_TAGGED,   			//bg1, pst, iwd1
	GF_HAS_SONGLIST,        			//bg2
	GF_TEAM_MOVEMENT,       			//pst
	GF_UPPER_BUTTON_TEXT,   			//bg2
	GF_LOWER_LABEL_TEXT,    			//bg2
	GF_HAS_PARTY_INI,       			//iwd2
	GF_SOUNDFOLDERS,        			//iwd2
	GF_IGNORE_BUTTON_FRAMES,			// all?
	GF_ONE_BYTE_ANIMID,     			// pst
	GF_HAS_DPLAYER,         			// not pst
	GF_HAS_EXPTABLE,        			// iwd, iwd2
	GF_HAS_BEASTS_INI,      			//pst; also for quests.ini
	GF_HAS_DESC_ICON,       			//bg
	GF_HAS_PICK_SOUND,      			//pst
	GF_IWD_MAP_DIMENSIONS,  			//iwd, iwd2
	GF_AUTOMAP_INI,         			//pst
	GF_SMALL_FOG,           			//bg1, pst
	GF_REVERSE_DOOR,        			//pst
	GF_PROTAGONIST_TALKS,   			//pst
	GF_HAS_SPELLLIST,       			//iwd2
	GF_IWD2_SCRIPTNAME,     			//iwd2, iwd, how
	GF_DIALOGUE_SCROLLS,    			//pst
	GF_KNOW_WORLD,          			//iwd2
	GF_REVERSE_TOHIT,       			//all except iwd2
	GF_SAVE_FOR_HALF,       			//pst
	GF_CHARNAMEISGABBER,   				//iwd2
	GF_MAGICBIT,            			//iwd, iwd2
	GF_CHECK_ABILITIES,     			//bg2 (others?)
	GF_CHALLENGERATING,     			//iwd2
	GF_SPELLBOOKICONHACK,   			//bg2
	GF_ENHANCED_EFFECTS,    			//iwd2 (maybe iwd/how too)
	GF_DEATH_ON_ZERO_STAT,  			//not in iwd2
	GF_SPAWN_INI,           			//pst, iwd, iwd2
	GF_IWD2_DEATHVARFORMAT,  			//iwd branch (maybe pst)
	GF_RESDATA_INI,         			//pst
	GF_OVERRIDE_CURSORPOS,  			//pst, iwd2
	GF_BREAKABLE_WEAPONS,     			//only bg1
	GF_3ED_RULES,              			//iwd2
	GF_LEVELSLOT_PER_CLASS,    			//iwd2
	GF_SELECTIVE_MAGIC_RES,    			//bg2, iwd2, (how)
	GF_HAS_HIDE_IN_SHADOWS,    			// bg2, iwd2
	GF_AREA_VISITED_VAR,    			//iwd, iwd2
	GF_PROPER_BACKSTAB,     			//bg2, iwd2, how?
	GF_ONSCREEN_TEXT,       			//pst
	GF_SPECIFIC_DMG_BONUS,				//how, iwd2
	GF_STRREF_SAVEGAME,       			//iwd2
	GF_SIMPLE_DISRUPTION,      			// ToBEx: simplified disruption
	GF_BIOGRAPHY_RES,               	//iwd branch
	GF_NO_BIOGRAPHY,                	//pst
	GF_STEAL_IS_ATTACK,             	//bg2 for sure
	GF_CUTSCENE_AREASCRIPTS,			//bg1, maybe more
	GF_FLEXIBLE_WMAP,               	//iwd
	GF_AUTOSEARCH_HIDDEN,           	//all except iwd2
	GF_PST_STATE_FLAGS,             	//pst complicates this
	GF_NO_DROP_CAN_MOVE,            	//bg1
	GF_JOURNAL_HAS_SECTIONS,        	//bg2
	GF_CASTING_SOUNDS,              	//all except pst and bg1
	GF_CASTING_SOUNDS2,             	//bg2
	GF_FORCE_AREA_SCRIPT,           	//how and iwd2 (maybe iwd1)
	GF_AREA_OVERRIDE,               	//pst maze and other hardcode
	GF_NO_NEW_VARIABLES,            	//pst
	GF_SOUNDS_INI,                  	//iwd/how/iwd2
	GF_USEPOINT_400,                	//all except pst and iwd2
	GF_USEPOINT_200,                	//iwd2
	GF_HAS_FLOAT_MENU,              	//pst
	GF_RARE_ACTION_VB,              	//pst
	GF_NO_UNDROPPABLE,              	//iwd,how
	GF_START_ACTIVE,                	//bg1
	GF_INFOPOINT_DIALOGS,           	//pst, but only bg1 has garbage there
	GF_IMPLICIT_AREAANIM_BACKGROUND,	//idw,how,iwd2
	GF_HEAL_ON_100PLUS,             	//bg1, bg2, pst
	GF_IN_PARTY_ALLOWS_DEAD,			//all except bg2
	GF_ZERO_TIMER_IS_VALID,         	// how, not bg2, other unknown
	GF_SHOP_RECHARGE,               	// all?
	GF_MELEEHEADER_USESPROJECTILE,  	// minimally bg2
	GF_FORCE_DIALOGPAUSE,           	// all except if using v1.04 DLG files (bg2, special)
	GF_RANDOM_BANTER_DIALOGS,       	// bg1
	GF_FIXED_MORALE_OPCODE,         	// bg2
	GF_HAPPINESS,                   	// all except pst and iwd2
	GF_EFFICIENT_OR,                	// does the OR trigger shortcircuit on success or not? Only in iwd2
	GF_LAYERED_WATER_TILES,				// TileOverlay for water has an extra half transparent layer (all but BG1)
	GF_CLEARING_ACTIONOVERRIDE,         // bg2, not iwd2
	GF_DAMAGE_INNOCENT_REP,             // not bg1
	GF_HAS_WEAPON_SETS,             	// iwd2

	GF_COUNT // sentinal count
};

//the number of item usage fields (used in CREItem and STOItem)
#define CHARGE_COUNTERS  3

/////AI global defines
#define AI_UPDATE_TIME	15

/////globally used functions

class Scriptable;
class Actor;

GEM_EXPORT unsigned int Distance(const Point &pos, const Scriptable *b);
GEM_EXPORT unsigned int SquaredMapDistance(const Point &pos, const Scriptable *b);
GEM_EXPORT unsigned int PersonalDistance(const Point &p, const Scriptable *b);
GEM_EXPORT unsigned int SquaredPersonalDistance(const Point &pos, const Scriptable *b);
GEM_EXPORT unsigned int Distance(const Scriptable *a, const Scriptable *b);
GEM_EXPORT unsigned int SquaredDistance(const Scriptable *a, const Scriptable *b);
GEM_EXPORT unsigned int PersonalDistance(const Scriptable *a, const Scriptable *b);
GEM_EXPORT unsigned int SquaredPersonalDistance(const Scriptable *a, const Scriptable *b);
GEM_EXPORT unsigned int SquaredMapDistance(const Scriptable *a, const Scriptable *b);
GEM_EXPORT unsigned int PersonalLineDistance(const Point &v, const Point &w, const Scriptable *s, double *proj);
GEM_EXPORT double Feet2Pixels(int feet, double angle);
GEM_EXPORT bool WithinAudibleRange(const Actor *actor, const Point &dest);
GEM_EXPORT bool WithinRange(const Scriptable *actor, const Point &dest, int distance);
GEM_EXPORT bool WithinPersonalRange(const Scriptable *actor, const Point &dest, int distance);
GEM_EXPORT bool WithinPersonalRange(const Scriptable* scr1, const Scriptable* scr2, int distance);
GEM_EXPORT int EARelation(const Scriptable *a, const Actor *b);
GEM_EXPORT bool Schedule(ieDword schedule, ieDword time);

#define SCHEDULE_MASK(time) (1 << core->Time.GetHour(time - core->Time.hour_size/2))

using tick_t = unsigned long; // milliseconds
inline tick_t GetMilliseconds()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

template<typename T>
T strtosigned(const char* str, char** endptr = nullptr, int base = 0)
{
	static_assert(std::is_integral<T>::value, "Type must be integral.");
	static_assert(sizeof(T) <= sizeof(long), "Type is too big for conversion.");
	static_assert(std::is_signed<T>::value, "Type must be signed");
	
	long ret = strtol(str, endptr, base);
	if (ret > std::numeric_limits<T>::max()) {
		return std::numeric_limits<T>::max();
	}
	
	if (ret < std::numeric_limits<T>::min()) {
		return std::numeric_limits<T>::min();
	}
	
	return static_cast<T>(ret);
}

template<typename T>
inline bool valid_signednumber(const char* string, T& val)
{
	char* endpr = nullptr;
	val = strtosigned<T>(string, &endpr, 0);
	return endpr != string;
}

template<typename T>
T strtounsigned(const char* str, char** endptr = nullptr, int base = 0)
{
	static_assert(std::is_integral<T>::value, "Type must be integral.");
	static_assert(sizeof(T) <= sizeof(long), "Type is too big for conversion.");
	static_assert(std::is_unsigned<T>::value, "Type must be unsigned");
	
	unsigned long ret = strtoul(str, endptr, base);
	if (ret > std::numeric_limits<T>::max()) {
		return std::numeric_limits<T>::max();
	}
	
	return static_cast<T>(ret);
}

template<typename T>
inline bool valid_unsignednumber(const char* string, T& val)
{
	char* endpr = nullptr;
	val = strtounsigned<T>(string, &endpr, 0);
	return endpr != string;
}

// bitflag operations
// !!! Keep these synchronized with GUIDefines.py !!!
enum class BitOp : int {
	SET = 0, //gemrb extension
	AND = 1,
	OR = 2,
	XOR = 3,
	NAND = 4 //gemrb extension
};

template <typename T>
inline bool SetBits(T& flag, const T& value, BitOp mode)
{
	switch(mode) {
		case BitOp::OR: flag |= value; break;
		case BitOp::NAND: flag &= ~value; break;
		case BitOp::SET: flag = value; break;
		case BitOp::AND: flag &= value; break;
		case BitOp::XOR: flag ^= value; break;
		default:
			return false;
	}
	return true;
}

template <typename T>
inline size_t CountBits(const T& i)
{
	// using a bitset is guaranteed to be portable
	// it is also usually much faster than a loop over the bits since it is often implemented with a CPU popcount instruction
	static std::bitset<sizeof(T) * CHAR_BIT> bits;
	bits = i;
	return bits.count();
}

template <typename T>
inline T Clamp(const T& n, const T& lower, const T& upper)
{
	return std::max(lower, std::min(n, upper));
}

template <>
inline Point Clamp(const Point& p, const Point& lower, const Point& upper)
{
	Point ret;
	ret.x = std::max(lower.x, std::min(p.x, upper.x));
	ret.y = std::max(lower.y, std::min(p.y, upper.y));
	return ret;
}

// WARNING: these are meant to be fast, if you are concerned with overflow/underflow don't use them
template <typename T>
inline T CeilDivUnsigned(T dividend, T divisor)
{
	//static_assert(std::is_unsigned<T>::value, "This quick round only works for positive values.");
	return (dividend + divisor - 1) / divisor;
}

template <typename T>
inline T CeilDivSlow(T dividend, T divisor)
{
	if (divisor < 0) {
		return (dividend - divisor - 1) / divisor;
	} else {
		return (dividend + divisor - 1) / divisor;
	}
}

template <typename T>
inline T CeilDiv(T dividend, T divisor)
{
	static_assert(std::is_integral<T>::value, "Only integral types allowed");
	// the compiler should be able to boil all this away (at least in release mode)
	// and just inline the fastest version supported by T
	if (std::is_unsigned<T>::value) {
		return CeilDivUnsigned(dividend, divisor);
	} else {
		return CeilDivSlow(dividend, divisor);
	}
}

// TODO: remove this when we switch to c++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

//the maximum supported game CD count
#define MAX_CD               6

}

#endif //! GLOBALS_H


