/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef IE_FEATS_H
#define IE_FEATS_H

#include <cstdint>

namespace GemRB {

enum class Feat : uint8_t {
	AegisOfRime,
	Ambidexterity,
	AquaMortis,
	ArmourProficiency,
	ArmoredArcana,
	ArterialStrike,
	BlindFight,
	Bullheaded,
	Cleave,
	CombatCasting,
	CourteousMagocracy,
	CripplingStrike,
	Dash,
	DeflectArrows,
	DirtyFighting,
	Discipline,
	Dodge,
	EnvenomWeapon,
	ExoticBastard,
	Expertise,
	ExtraRage,
	ExtraShapeshifting,
	ExtraSmiting,
	ExtraTurning,
	Fiendslayer,
	Forester,
	GreatFortitude,
	Hamstring,
	HereticsBane,
	HeroicInspiration,
	ImprovedCritical,
	ImprovedEvasion,
	ImprovedInitiative,
	ImprovedTurning,
	IronWill,
	LightningReflexes,
	LingeringSong,
	LuckOfHeroes,
	MartialAxe,
	MartialBow,
	MartialFlail,
	MartialGreatsword,
	MartialHammer,
	MartialLargesword,
	MartialPolearm,
	MaximizedAttacks,
	MercantileBackground,
	PowerAttack,
	PreciseShot,
	RapidShot,
	ResistPoison,
	ScionOfStorms,
	ShieldProf,
	SimpleCrossbow,
	SimpleMace,
	SimpleMissile,
	SimpleQuarterstaff,
	SimpleSmallblade,
	SlipperyMind,
	SnakeBlood,
	SpellFocusEnchant,
	SpellFocusEvocation,
	SpellFocusNecromancy,
	SpellFocusTransmute,
	SpellPenetration,
	SpiritOfFlame,
	StrongBack,
	StunningFist,
	SubvocalCasting,
	Toughness,
	TwoWeaponFighting,
	WeaponFinesse,
	WildshapeBoar,
	WildshapePanther,
	WildshapeShambler,

	count = 96 // 3 * sizeof(ieDword) for the three stats that hold them
};

}

#endif
