// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
