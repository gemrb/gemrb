/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// convenience classes for storing the complex armor class, to-hit and other combat state

#ifndef COMBATINFO_H
#define COMBATINFO_H

#include "exports.h"

namespace GemRB {

class Actor;

class GEM_EXPORT ArmorClass {
public:
	ArmorClass();

	int GetTotal() const;
	int GetNatural() const { return natural; };
	int GetDeflectionBonus() const { return deflectionBonus; };
	int GetArmorBonus() const { return armorBonus; };
	int GetShieldBonus() const { return shieldBonus; };
	int GetDexterityBonus() const { return dexterityBonus; };
	int GetWisdomBonus() const { return wisdomBonus; };
	int GetGenericBonus() const { return genericBonus; };

	void ResetAll();
	void SetOwner(Actor *owner);
	// no total, it is always kept up to date with RefreshTotal
	void SetNatural(int AC, int mod=1);
	void SetDeflectionBonus(int bonus, int mod=1);
	void SetArmorBonus(int bonus, int mod=1);
	void SetShieldBonus(int bonus, int mod=1);
	void SetDexterityBonus(int bonus, int mod=1);
	void SetWisdomBonus(int bonus, int mod=1);
	void SetGenericBonus(int bonus, int mod=1);

	void HandleFxBonus(int mod, bool permanent);
	void dump() const;

private:
	Actor *Owner;
	int total; // modified stat
	int natural; // base stat

	int deflectionBonus;
	int armorBonus;
	int shieldBonus;
	int dexterityBonus;
	int wisdomBonus;
	int genericBonus; // the iwd2 dodge bonus was also just applied here and is not a separate value
	// NOTE: some are currently left out, like BonusAgainstCreature and all the real separate AC-mod stats
	//   (in case of change) recheck all the introduced GetTotal()s with this in mind - before they were Modified[IE_ARMORCLASS] only

	void SetBonus(int &current, int bonus, int mod);
	void RefreshTotal();
};


}

#endif