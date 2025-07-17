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

#include "RGBAColor.h"
#include "errors.h"
#include "ie_types.h"

#include "EnumFlags.h"
#include "Region.h"

#include "Streams/DataStream.h"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <climits>
#include <cmath>
#include <memory>
#include <type_traits>

namespace GemRB {

//Global Variables

/////feature flags
enum class GFFlags : uint32_t {
	// this enum must begin at 0 for iteration
	HAS_KAPUTZ = 0, //pst
	ALL_STRINGS_TAGGED, //bg1, pst, iwd1
	HAS_SONGLIST, //bg2
	TEAM_MOVEMENT, //pst
	UPPER_BUTTON_TEXT, //bg2
	LOWER_LABEL_TEXT, //bg2
	HAS_PARTY_INI, //iwd2
	SOUNDFOLDERS, //iwd2
	IGNORE_BUTTON_FRAMES, // all?
	ONE_BYTE_ANIMID, // pst
	HAS_DPLAYER, // not pst
	HAS_EXPTABLE, // iwd, iwd2
	HAS_BEASTS_INI, //pst; also for quests.ini
	HAS_EE_EFFECTS, //ees
	HAS_PICK_SOUND, //pst
	IWD_MAP_DIMENSIONS, //iwd, iwd2
	AUTOMAP_INI, //pst
	SMALL_FOG, //bg1, pst
	REVERSE_DOOR, //pst
	PROTAGONIST_TALKS, //pst
	HAS_SPELLLIST, //iwd2
	IWD2_SCRIPTNAME, //iwd2, iwd, how
	DIALOGUE_SCROLLS, //pst
	KNOW_WORLD, //iwd2
	REVERSE_TOHIT, //all except iwd2
	SAVE_FOR_HALF, //pst
	CHARNAMEISGABBER, //iwd2
	MAGICBIT, //iwd, iwd2
	CHECK_ABILITIES, //bg2 (others?)
	CHALLENGERATING, //iwd2
	SPELLBOOKICONHACK, //bg2
	ENHANCED_EFFECTS, //iwd2 (maybe iwd/how too)
	DEATH_ON_ZERO_STAT, //not in iwd2
	SPAWN_INI, //pst, iwd, iwd2
	IWD2_DEATHVARFORMAT, //iwd branch (maybe pst)
	RESDATA_INI, //pst
	BREAKABLE_WEAPONS, //only bg1
	RULES_3ED, //iwd2
	LEVELSLOT_PER_CLASS, //iwd2
	SELECTIVE_MAGIC_RES, //bg2, iwd2, (how)
	HAS_HIDE_IN_SHADOWS, // bg2, iwd2
	AREA_VISITED_VAR, //iwd, iwd2
	PROPER_BACKSTAB, //bg2, iwd2, how?
	ONSCREEN_TEXT, //pst
	SPECIFIC_DMG_BONUS, //how, iwd2
	STRREF_SAVEGAME, //iwd2
	SIMPLE_DISRUPTION, // ToBEx: simplified disruption
	BIOGRAPHY_RES, //iwd branch
	NO_BIOGRAPHY, //pst
	STEAL_IS_ATTACK, //bg2 for sure
	CUTSCENE_AREASCRIPTS, //bg1, maybe more
	FLEXIBLE_WMAP, //iwd
	AUTOSEARCH_HIDDEN, //all except iwd2
	PST_STATE_FLAGS, //pst complicates this
	NO_DROP_CAN_MOVE, //bg1
	JOURNAL_HAS_SECTIONS, //bg2
	CASTING_SOUNDS, //all except pst and bg1
	CASTING_SOUNDS2, //bg2
	FORCE_AREA_SCRIPT, //how and iwd2 (maybe iwd1)
	AREA_OVERRIDE, //pst maze and other hardcode
	NO_NEW_VARIABLES, //pst
	SOUNDS_INI, //iwd/how/iwd2
	USEPOINT_400, //all except pst and iwd2
	USEPOINT_200, //iwd2
	HAS_FLOAT_MENU, //pst
	NO_UNDROPPABLE, //iwd,how
	START_ACTIVE, //bg1
	INFOPOINT_DIALOGS, //pst, but only bg1 has garbage there
	IMPLICIT_AREAANIM_BACKGROUND, //idw,how,iwd2
	HEAL_ON_100PLUS, //bg1, bg2, pst
	IN_PARTY_ALLOWS_DEAD, //all except bg2
	ZERO_TIMER_IS_VALID, // how, not bg2, other unknown
	SHOP_RECHARGE, // all?
	MELEEHEADER_USESPROJECTILE, // minimally bg2
	FORCE_DIALOGPAUSE, // all except if using v1.04 DLG files (bg2, special)
	RANDOM_BANTER_DIALOGS, // bg1
	FIXED_MORALE_OPCODE, // bg2
	HAPPINESS, // all except pst and iwd2
	EFFICIENT_OR, // does the OR trigger shortcircuit on success or not? Only in iwd2
	LAYERED_WATER_TILES, // TileOverlay for water has an extra half transparent layer (all but BG1)
	CLEARING_ACTIONOVERRIDE, // bg2, not iwd2
	DAMAGE_INNOCENT_REP, // not bg1
	HAS_WEAPON_SETS, // iwd2
	HIGHLIGHT_OUTLINE_ONLY, // all
	IWD_REST_SPAWNS, // iwd1, not bgs or iwdee
	HAS_CONTINUATION, // all but iwds
	SELLABLE_CRITS_NO_CONV, // bg1, iwd1
	BETTER_OF_HEARING, // all but bg1 and psts

	count // must be last
};

//the number of item usage fields (used in CREItem and STOItem)
#define CHARGE_COUNTERS 3

/////globally used functions

class Scriptable;
class Actor;

GEM_EXPORT unsigned int Distance(const Point& pos, const Scriptable* b);
GEM_EXPORT unsigned int PersonalDistance(const Point& p, const Scriptable* b);
GEM_EXPORT unsigned int SquaredPersonalDistance(const Point& pos, const Scriptable* b);
GEM_EXPORT unsigned int Distance(const Scriptable* a, const Scriptable* b);
GEM_EXPORT unsigned int SquaredDistance(const Scriptable* a, const Scriptable* b);
GEM_EXPORT unsigned int PersonalDistance(const Scriptable* a, const Scriptable* b);
GEM_EXPORT unsigned int SquaredPersonalDistance(const Scriptable* a, const Scriptable* b);
GEM_EXPORT unsigned int PersonalLineDistance(const Point& v, const Point& w, const Scriptable* s, float_t* proj);
GEM_EXPORT float_t Feet2Pixels(int feet, float_t angle);
GEM_EXPORT bool WithinAudibleRange(const Actor* actor, const Point& dest);
GEM_EXPORT bool WithinRange(const Scriptable* actor, const Point& dest, int distance);
GEM_EXPORT bool WithinPersonalRange(const Scriptable* actor, const Point& dest, int distance);
GEM_EXPORT bool WithinPersonalRange(const Scriptable* scr1, const Scriptable* scr2, int distance);
GEM_EXPORT int EARelation(const Scriptable* a, const Actor* b);
GEM_EXPORT bool Schedule(ieDword schedule, ieDword time);

#define SCHEDULE_MASK(time) (1 << core->Time.GetHour(time - core->Time.hour_size / 2))

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

template<typename T>
inline bool SetBits(T& flag, const T& value, BitOp mode)
{
	switch (mode) {
		case BitOp::OR:
			flag |= value;
			break;
		case BitOp::NAND:
			flag &= ~value;
			break;
		case BitOp::SET:
			flag = value;
			break;
		case BitOp::AND:
			flag &= value;
			break;
		case BitOp::XOR:
			flag ^= value;
			break;
		default:
			return false;
	}
	return true;
}

template<typename T>
inline size_t CountBits(const T& i)
{
	// using a bitset is guaranteed to be portable
	// it is also usually much faster than a loop over the bits since it is often implemented with a CPU popcount instruction
	static std::bitset<sizeof(T) * CHAR_BIT> bits;
	bits = i;
	return bits.count();
}

template<typename T, std::enable_if_t<std::is_scalar<T>::value, bool> = true>
inline T Clamp(const T& n, const T& lower, const T& upper)
{
	return std::max(lower, std::min(n, upper));
}

// this is like static_cast, but clamps to the range supported by DST instead of turning large positive numbers negative and large negatives 0
template<typename DST, typename SRC>
inline DST Clamp(const SRC& n)
{
	static_assert(sizeof(DST) <= sizeof(SRC), "Clamping a SRC smaller than DST has no effect.");
	return static_cast<DST>(Clamp<SRC>(n, std::numeric_limits<DST>::min(), std::numeric_limits<DST>::max()));
}

template<typename T, std::enable_if_t<std::is_base_of<BasePoint, T>::value, bool> = true>
inline T Clamp(const T& p, const T& lower, const T& upper)
{
	T ret;
	ret.x = std::max(lower.x, std::min(p.x, upper.x));
	ret.y = std::max(lower.y, std::min(p.y, upper.y));
	return ret;
}

// WARNING: these are meant to be fast, if you are concerned with overflow/underflow don't use them
template<typename T>
inline T CeilDivUnsigned(T dividend, T divisor)
{
	//static_assert(std::is_unsigned<T>::value, "This quick round only works for positive values.");
	return (dividend + divisor - 1) / divisor;
}

template<typename T>
inline T CeilDivSlow(T dividend, T divisor)
{
	if (divisor < 0) {
		return (dividend - divisor - 1) / divisor;
	} else {
		return (dividend + divisor - 1) / divisor;
	}
}

template<typename T>
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

inline std::string YesNo(bool test)
{
	return test ? "Yes" : "No";
}

}

#endif //! GLOBALS_H
