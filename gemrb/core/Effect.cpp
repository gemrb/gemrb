/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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

#include "Effect.h"
#include "EffectQueue.h" // this is only needed for a hack in Effect::Persistent
#include "Interface.h"

namespace GemRB {

//don't modify position in case it was already set
void Effect::SetPosition(const Point &p)
{
	if (Pos.IsInvalid()) {
		Pos = p;
	}
}
void Effect::SetSourcePosition(const Point &p)
{
	if (Source.IsInvalid()) {
		Source = p;
	}
}

Effect::Effect(const Effect& rhs) noexcept
{
	operator=(rhs);
}

Effect& Effect::operator=(const Effect& rhs) noexcept
{
	if (this != &rhs) {
		Opcode = rhs.Opcode;
		Target = rhs.Target;
		Power = rhs.Power;
		Parameter1 = rhs.Parameter1;
		Parameter2 = rhs.Parameter2;
		TimingMode = rhs.TimingMode;
		// skip unknown2
		Resistance = rhs.Resistance;
		Duration = rhs.Duration;
		ProbabilityRangeMax = rhs.ProbabilityRangeMax;
		ProbabilityRangeMin = rhs.ProbabilityRangeMin;
		DiceThrown = rhs.DiceThrown;
		DiceSides = rhs.DiceSides;
		SavingThrowType = rhs.SavingThrowType;
		SavingThrowBonus = rhs.SavingThrowBonus;
		IsVariable = rhs.IsVariable;
		IsSaveForHalfDamage = rhs.IsSaveForHalfDamage;
		PrimaryType = rhs.PrimaryType;
		MinAffectedLevel = rhs.MinAffectedLevel;
		MaxAffectedLevel = rhs.MaxAffectedLevel;
		Parameter3 = rhs.Parameter3;
		Parameter4 = rhs.Parameter4;
		Parameter5 = rhs.Parameter5;
		Parameter6 = rhs.Parameter6;
		Source = rhs.Source;
		Pos = rhs.Pos;
		SourceType = rhs.SourceType;
		SourceRef = rhs.SourceRef;
		SourceFlags = rhs.SourceFlags;
		Projectile = rhs.Projectile;
		InventorySlot = rhs.InventorySlot;
		CasterLevel = rhs.CasterLevel;
		FirstApply = rhs.FirstApply;
		SecondaryType = rhs.SecondaryType;
		SecondaryDelay = rhs.SecondaryDelay;
		CasterID = rhs.CasterID;
		RandomValue = rhs.RandomValue;
		SpellLevel = rhs.SpellLevel;

		IsVariable = rhs.IsVariable;
		if (IsVariable) {
			VariableName = rhs.VariableName;
		} else {
			resources = rhs.resources;
		}
	}
	return *this;
}

bool Effect::operator==(const Effect& rhs) const noexcept
{
	if (this == &rhs) return true;
	
	if (Opcode != rhs.Opcode) return false;
	if (Target != rhs.Target) return false;
	if (Power != rhs.Power) return false;
	if (Parameter1 != rhs.Parameter1) return false;
	if (Parameter2 != rhs.Parameter2) return false;
	if (TimingMode != rhs.TimingMode) return false;
	// skip unknown2
	if (Resistance != rhs.Resistance) return false;
	if (Duration != rhs.Duration) return false;
	if (ProbabilityRangeMax != rhs.ProbabilityRangeMax) return false;
	if (ProbabilityRangeMin != rhs.ProbabilityRangeMin) return false;
	if (DiceThrown != rhs.DiceThrown) return false;
	if (DiceSides != rhs.DiceSides) return false;
	if (SavingThrowType != rhs.SavingThrowType) return false;
	if (SavingThrowBonus != rhs.SavingThrowBonus) return false;
	if (IsVariable != rhs.IsVariable) return false;
	if (IsSaveForHalfDamage != rhs.IsSaveForHalfDamage) return false;
	if (PrimaryType != rhs.PrimaryType) return false;
	if (MinAffectedLevel != rhs.MinAffectedLevel) return false;
	if (MaxAffectedLevel != rhs.MaxAffectedLevel) return false;
	if (Parameter3 != rhs.Parameter3) return false;
	if (Parameter4 != rhs.Parameter4) return false;
	if (Parameter5 != rhs.Parameter5) return false;
	if (Parameter6 != rhs.Parameter6) return false;
	if (Source != rhs.Source) return false;
	if (Pos != rhs.Pos) return false;
	if (SourceType != rhs.SourceType) return false;
	if (SourceRef != rhs.SourceRef) return false;
	if (SourceFlags != rhs.SourceFlags) return false;
	if (Projectile != rhs.Projectile) return false;
	if (InventorySlot != rhs.InventorySlot) return false;
	if (CasterLevel != rhs.CasterLevel) return false;
	if (FirstApply != rhs.FirstApply) return false;
	if (SecondaryType != rhs.SecondaryType) return false;
	if (SecondaryDelay != rhs.SecondaryDelay) return false;
	if (CasterID != rhs.CasterID) return false;
	if (RandomValue != rhs.RandomValue) return false;
	if (SpellLevel != rhs.SpellLevel) return false;

	if (IsVariable && VariableName != rhs.VariableName) return false;
	else return Resource == rhs.Resource && Resource2 == rhs.Resource2 && Resource3 == rhs.Resource3 && Resource4 == rhs.Resource4;
	
}

//returns true if the effect supports simplified duration
bool Effect::HasDuration() const
{
	switch(TimingMode) {
	case FX_DURATION_INSTANT_LIMITED: //simple duration
	case FX_DURATION_DELAY_LIMITED:   //delayed duration
	case FX_DURATION_DELAY_PERMANENT: //simple delayed
	// not supporting FX_DURATION_INSTANT_LIMITED_TICKS, since it's in ticks
		return true;
	}
	return false;
}

//returns true if the effect must be saved
bool Effect::Persistent() const
{
	// local variable effects self-destruct if they were processed already
	// but if they weren't processed, e.g. in a global actor, we must save them
	// TODO: do we really need to special-case this? leaving it for now - fuzzie
	static EffectRef fx_variable_ref = { "Variable:StoreLocalVariable", -1 };
	if (Opcode == (ieDword)EffectQueue::ResolveEffect(fx_variable_ref)) {
		return true;
	}

	switch (TimingMode) {
		//normal equipping fx of items
		case FX_DURATION_INSTANT_WHILE_EQUIPPED:
		//delayed effect not saved
		case FX_DURATION_DELAY_UNSAVED:
		//permanent effect not saved
		case FX_DURATION_PERMANENT_UNSAVED:
		//just expired effect
		case FX_DURATION_JUST_EXPIRED:
			return false;
	}
	return true;
}

// we add +1 so we can handle effects with 0 duration (apply once only)
void Effect::PrepareDuration(ieDword gameTime)
{
	Duration = (Duration ? Duration * core->Time.defaultTicksPerSec : 1) + gameTime;
}

} // namespace GemRB
