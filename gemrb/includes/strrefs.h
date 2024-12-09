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
 * @file strrefs.h
 * Defines indices of "standard" strings in strings.2da files
 * @author The GemRB Project
 */


// these symbols should match strings.2da

#ifndef IE_STRINGS_H
#define IE_STRINGS_H

#include "TableMgr.h"

namespace GemRB {

enum class HCStrings : TableMgr::index_t {
	Scattered = 0,
	WholeParty,
	DoorLocked,
	Ambush,
	ContLocked,
	Cursed,
	SpellDisrupted,
	MayNotRest,
	CantRestMonsters,
	CantSaveMonsters,
	CantSave,
	CantSaveDialog,
	CantSaveDialog2,
	CantSaveMovie,
	TargetBusy,
	DialogNoAreaMove,
	GotGold,
	LostGold,
	GotXP,
	LostXP,
	GotItem,
	LostItem,
	GotRep,
	LostRep,
	GotAbility,
	GotSpell,
	GotSong,
	NothingToSay,
	JournalChange,
	WorldmapChange,
	Paused,
	Unpaused,
	ScriptPaused,
	ApUnusable,
	ApAttacked, // this and the following are used via an offset to ApUnusable
	ApHit,
	ApWounded,
	ApDead,
	ApNotarget,
	ApEndround,
	ApEnemy,
	ApTrap,
	ApSpellcast,
	ApGeneric,
	ApReserved1,
	ApReserved2,
	ApReserved3,
	Charmed,
	DireCharmed,
	Controlled,
	Evil,
	GENeutral,
	Good,
	Lawful,
	LCNeutral,
	Chaotic,
	ActionCast,
	ActionAttack,
	MagicWeapon,
	OffhandUsed,
	TwohandedUsed,
	CantUseItem,
	CantDropItem, // not referenced, but unsure if useful
	NotInOffhand,
	NoCritical,
	Tracking,
	TrackingFailed,
	DoorNotPickable,
	ContNotpickable,
	CantSaveCombat,
	CantSaveNoCtrl,
	LockpickDone,
	LockpickFailed,
	StaticDissipate,
	LightningDissipate,
	ItemUnusable, // item has no usable ability
	ItemNeedsId, // item needs identify
	WrongItemType,
	ItemExclusion,
	PickpocketDone,
	PickpocketNone, // no items to steal
	PickpocketFail, // failed, noticed
	PickpocketEvil, // can't pick hostiles
	PickpocketArmor, // unsure if useful, add check to action?
	UsingFeat,
	StoppedFeat,
	DisarmDone,
	DisarmFail,
	DoorBashDone,
	DoorBashFail,
	ContBashDone,
	ContBashFail,
	MayNotSetTrap,
	SnareFailed,
	SnareSucceed,
	NoMoreTraps,
	DisabledMageSpells,
	SaveSuccess,
	QSaveSuccess,
	Uninjured,
	Injured1,
	Injured2,
	Injured3,
	Injured4, // near death
	Hours, // <HOUR> hours / Hours
	Hour, // <HOUR> hours / Hour
	Days, // <GAMEDAYS> days
	Day,
	Rested, // You have rested for <DURATION> / <HOUR> <DURATION>
	SummoningLimit,
	InventoryFull,
	TooFarAway,
	DamageImmunity,
	Damage1,
	Damage2,
	Damage3, // used through an offset with Damage1 and DamageDetail1
	DmgPoison, // all the following are used indirectly in dmginfo.2da
	DmgMagic,
	DmgMissile,
	DmgSlashing,
	DmgPiercing,
	DmgCrushing,
	DmgFire,
	DmgElectric,
	DmgCold,
	DmgAcid,
	DmgOther,
	GotQuestXP,
	LevelUp,
	InventoryFullItemDrop,
	ContingencyDupe,
	ContingencyFail,
	SequencerDupe,
	CriticalHit,
	CriticalMiss,
	Death,
	BackstabDamage,
	BackstabBad,
	BackstabFail,
	CasterLvlInc, // caster level bonus (wild mages)
	CasterLvlDec,
	Exported, // characters exported (iwd)
	PaladinFall,
	RangerFall,
	ResResisted,
	DeadmagicFail,
	MiscastMagic,
	WildSurge,
	FamiliarBlock,
	FamiliarProtagonistOnly,
	FamiliarNoHands,
	MagicResisted,
	CantSaveStore,
	NoSeeNoCast,
	AuraCleansed,
	IndoorFail,
	SpellFailed,
	ChaosShield,
	RapidShot,
	Hamstring,
	Arterial,
	Expertise,
	PowerAttack,
	Cleave,
	StateHeld,
	HalfSpeed,
	CantMove,
	Casts,
	WeaponIneffective,
	ConcealedMiss,
	SaveSpell,
	SaveBreath, // the next few are used with an offset from SaveSpell
	SaveDeath,
	SaveWands,
	SavePoly,
	AttackRoll,
	AttackRollLeft,
	Hit,
	Miss,
	NoRangedOffhand,
	CantRestNoControl,
	DamageDetail1,
	DamageDetail2,
	DamageDetail3,
	TravelTime,
	PickpocketInventoryFull, // pst only
	MoraleBerserk,
	MoraleRun, // used with offset from MoraleBerserk
	MoralePanic, // used with offset from MoraleBerserk
	TrapFound,
	BackstabDouble,
	Evaded1,
	Evaded2,
	HealingRest,
	HealingRestFull,

	count,
};

}

#endif //! IE_STRINGS_H
