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
#include "Scriptable/Actor.h"

namespace GemRB {

static bool third = false;

/*
 * Shared code between the classes
 */
static void SetBonusInternal(int& current, int bonus, int mod, bool cumulative = false)
{
	int newBonus = current;
	int tmp;

	switch (mod) {
		case 0: // cumulative modifier
			if (!third || cumulative) {
				newBonus += bonus;
				break;
			}

			// 3ed boni don't stack
			// but, use some extra logic so any negative boni first try to cancel out
			tmp = bonus;
			if ((current < 0) ^ (bonus < 0)) {
				tmp = current + bonus;
			}
			if (tmp != bonus) {
				// we just summed the boni, so we need to be careful about the resulting sign, since (-2+3)>(-2), but abs(-2+3)<abs(-2)
				if (tmp > current) {
					newBonus = tmp;
				} // else leave it be at the current value
			} else {
				if (std::abs(tmp) > std::abs(current)) {
					newBonus = tmp;
				} // else leave it be at the current value
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
			error("CombatInfo", "Bad bonus mod type: {}", mod);
	}

	current = newBonus;
}


/*
 * Class holding the main armor class stat and general boni
 */
ArmorClass::ArmorClass()
{
	ResetAll();

	third = core->HasFeature(GFFlags::RULES_3ED);
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
	// iwd2 generic AC bonus is the only stacking one
	SetBonus(genericBonus, bonus, mod, true);
}

void ArmorClass::SetBonus(int& current, int bonus, int mod, bool cumulative)
{
	SetBonusInternal(current, bonus, mod, cumulative);
	RefreshTotal();
}

void ArmorClass::HandleFxBonus(int mod, bool permanent)
{
	if (permanent) {
		if (Actor::IsReverseToHit()) {
			SetNatural(natural-mod);
		} else {
			SetNatural(natural+mod);
		}
		return;
	}
	// this was actually aditively modifying Modified directly before
	if (Actor::IsReverseToHit()) {
		SetGenericBonus(-mod, 0);
	} else {
		SetGenericBonus(mod, 0);
	}
}

std::string ArmorClass::dump() const
{
	std::string buffer;
	AppendFormat(buffer, "Debugdump of ArmorClass of {}:\n", fmt::WideToChar{Owner->GetName()});
	AppendFormat(buffer, "TOTAL: {}\n", total);
	AppendFormat(buffer, "Natural: {}\tGeneric: {}\tDeflection: {}\n", natural, genericBonus, deflectionBonus);
	AppendFormat(buffer, "Armor: {}\tShield: {}\n", armorBonus, shieldBonus);
	AppendFormat(buffer, "Dexterity: {}\tWisdom: {}\n\n", dexterityBonus, wisdomBonus);
	Log(DEBUG, "ArmorClass", "{}", buffer);
	return buffer;
}

/*
 * Class holding the main to-hit/thac0 stat and general boni
 * NOTE: Always use it through GetCombatDetails to get the full state
 */
ToHitStats::ToHitStats()
{
	ResetAll();

	third = core->HasFeature(GFFlags::RULES_3ED);
}

void ToHitStats::SetOwner(Actor* owner)
{
	Owner = owner;
	// rerun this, so both the stats get set correctly
	SetBase(base);
}

int ToHitStats::GetTotal() const
{
	return total;
}

void ToHitStats::RefreshTotal()
{
	total = base + proficiencyBonus + armorBonus + shieldBonus + abilityBonus + weaponBonus + genericBonus + fxBonus;
	if (Owner) { // not true for a short while during init, but we make amends immediately
		Owner->Modified[IE_TOHIT] = total;
	}
}

// resets all the boni
void ToHitStats::ResetAll() {
	weaponBonus = 0;
	armorBonus = 0;
	shieldBonus = 0;
	abilityBonus = 0;
	proficiencyBonus = 0;
	genericBonus = 0;
	fxBonus = 0;
	RefreshTotal();
}

void ToHitStats::SetBase(int tohit, int /*mod*/)
{
	base = tohit;
	if (Owner) { // not true for a short while during init, but we make amends immediately
		Owner->BaseStats[IE_TOHIT] = tohit;
	}
	RefreshTotal();
}

void ToHitStats::SetProficiencyBonus(int bonus, int mod)
{
	SetBonus(proficiencyBonus, bonus, mod);
}

void ToHitStats::SetArmorBonus(int bonus, int mod)
{
	SetBonus(armorBonus, bonus, mod);
}

void ToHitStats::SetShieldBonus(int bonus, int mod)
{
	SetBonus(shieldBonus, bonus, mod);
}

void ToHitStats::SetAbilityBonus(int bonus, int mod)
{
	SetBonus(abilityBonus, bonus, mod);
}

void ToHitStats::SetWeaponBonus(int bonus, int mod)
{
	SetBonus(weaponBonus, bonus, mod);
}

void ToHitStats::SetGenericBonus(int bonus, int mod)
{
	SetBonus(genericBonus, bonus, mod);
}

void ToHitStats::SetFxBonus(int bonus, int mod)
{
	SetBonus(fxBonus, bonus, mod);
}

void ToHitStats::SetBonus(int& current, int bonus, int mod)
{
	SetBonusInternal(current, bonus, mod);
	RefreshTotal();
}

void ToHitStats::HandleFxBonus(int mod, bool permanent)
{
	if (permanent) {
		if (Actor::IsReverseToHit()) {
			SetBase(base-mod);
		} else {
			SetBase(base+mod);
		}
		return;
	}
	// this was actually aditively modifying Modified directly before
	if (Actor::IsReverseToHit()) {
		SetFxBonus(-mod, 0);
	} else {
		SetFxBonus(mod, 0);
	}
}

void ToHitStats::SetBABDecrement(int decrement) {
	babDecrement = decrement;
}

int ToHitStats::GetTotalForAttackNum(unsigned int number) const
{
	if (number <= 1) { // out of combat, we'd get 0 and that's fine
		return total;
	}
	number--;
	// compute the cascaded values
	// at low levels with poor stats, even the total can be negative
	return total-number*babDecrement;
}

std::string ToHitStats::dump() const
{
	std::string buffer;
	AppendFormat(buffer, "Debugdump of ToHit of {}:\n", fmt::WideToChar{Owner->GetName()});
	AppendFormat(buffer, "TOTAL: {}\n", total);
	AppendFormat(buffer, "Base: {:2d}\tGeneric: {}\tEffect: {}\n", base, genericBonus, fxBonus);
	AppendFormat(buffer, "Armor: {}\tShield: {}\n", armorBonus, shieldBonus);
	AppendFormat(buffer, "Weapon: {}\tProficiency: {}\tAbility: {}\n\n", weaponBonus, proficiencyBonus, abilityBonus);
	Log(DEBUG, "ToHitStats", "{}", buffer);
	return buffer;
}


}
