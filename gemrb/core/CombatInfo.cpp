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

#include "CombatInfo.h"

#include "Interface.h"
#include "System/StringBuffer.h"
#include "Scriptable/Actor.h"

namespace GemRB {

static bool third = false;

/*
 * Shared code between the classes
 */
static void SetBonusInternal(int& current, int bonus, int mod)
{
	int newBonus = current;

	switch (mod) {
		case 0: // cummulative modifier
			if (third) {
				// 3ed boni don't stack
				// but, use some extra logic so any negative boni first try to cancel out
				int tmp = bonus;
				if ((current < 0) ^ (bonus < 0)) {
					tmp = current + bonus;
				}
				if (tmp != bonus) {
					// we just summed the boni, so we need to be careful about the resulting sign, since (-2+3)>(-2), but abs(-2+3)<abs(-2)
					if (tmp > current) {
						newBonus = tmp;
					} // else leave it be at the current value
				} else {
					if (abs(tmp) > abs(current)) {
						newBonus = tmp;
					} // else leave it be at the current value
				}
			} else {
				newBonus += bonus;
			}
			break;
		// like with other effects, the following options have chicked-and-egg problems and the result depends on the order of application
		case 1: // flat modifier
			newBonus = bonus;
			break;
		case 2: // percent modifier
			newBonus = current * bonus / 100;
			break;
		default:
			error("CombatInfo", "Bad bonus mod type: %d", mod);
			break;
	}

	current = newBonus;
}


/*
 * Class holding the main armor class stat and general boni
 */
ArmorClass::ArmorClass()
{
	natural = 0;
	Owner = NULL;
	ResetAll();

	third = !!core->HasFeature(GF_3ED_RULES);
}

void ArmorClass::SetOwner( Actor* owner)
{
	Owner = owner;
	// rerun this, so both the stats get set correctly
	SetNatural(natural);
}

int ArmorClass::GetTotal() const
{
	return total;
}

void ArmorClass::RefreshTotal()
{
	total = natural + deflectionBonus + armorBonus + shieldBonus + dexterityBonus + wisdomBonus + genericBonus;
	// add a maximum_values[IE_ARMORCLASS] check here if needed
	if (Owner) { // not true for a short while during init, but we make amends immediately
		Owner->Modified[IE_ARMORCLASS] = total;
	}
}

// resets all the boni (natural is skipped, since it holds the base value)
void ArmorClass::ResetAll() {
	deflectionBonus = 0;
	armorBonus = 0;
	shieldBonus = 0;
	dexterityBonus = 0;
	wisdomBonus = 0;
	genericBonus = 0;
	RefreshTotal();
}

void ArmorClass::SetNatural(int AC, int /*mod*/)
{
	natural = AC;
	if (Owner) { // not true for a short while during init, but we make amends immediately
		Owner->BaseStats[IE_ARMORCLASS] = AC;
	}
	RefreshTotal();
}

void ArmorClass::SetDeflectionBonus(int bonus, int mod)
{
	SetBonus(deflectionBonus, bonus, mod);
}

void ArmorClass::SetArmorBonus(int bonus, int mod)
{
	SetBonus(armorBonus, bonus, mod);
}

void ArmorClass::SetShieldBonus(int bonus, int mod)
{
	SetBonus(shieldBonus, bonus, mod);
}

void ArmorClass::SetDexterityBonus(int bonus, int mod)
{
	SetBonus(dexterityBonus, bonus, mod);
}

void ArmorClass::SetWisdomBonus(int bonus, int mod)
{
	SetBonus(wisdomBonus, bonus, mod);
}

void ArmorClass::SetGenericBonus(int bonus, int mod)
{
	SetBonus(genericBonus, bonus, mod);
}

void ArmorClass::SetBonus(int& current, int bonus, int mod)
{
	SetBonusInternal(current, bonus, mod);
	RefreshTotal();
}

void ArmorClass::HandleFxBonus(int mod, bool permanent)
{
	if (permanent) {
		if (Owner->IsReverseToHit()) {
			SetNatural(natural-mod);
		} else {
			SetNatural(natural+mod);
		}
		return;
	}
	// this was actually aditively modifying Modified directly before
	if (Owner->IsReverseToHit()) {
		SetGenericBonus(-mod, 0);
	} else {
		SetGenericBonus(mod, 0);
	}
}

void ArmorClass::dump() const
{
	StringBuffer buffer;
	buffer.appendFormatted("Debugdump of ArmorClass of %s:\n", Owner->GetName(1));
	buffer.appendFormatted("TOTAL: %d\n", total);
	buffer.appendFormatted("Natural: %d\tGeneric: %d\tDeflection: %d\n", natural, genericBonus, deflectionBonus);
	buffer.appendFormatted("Armor: %d\tShield: %d\n", armorBonus, shieldBonus);
	buffer.appendFormatted("Dexterity: %d\tWisdom: %d\n\n", dexterityBonus, wisdomBonus);
	Log(DEBUG, "ArmorClass", buffer);
}


}
