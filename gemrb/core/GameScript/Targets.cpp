// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later


#include "Targets.h"

#include "GameScript/GSUtils.h"
#include "Strings/StringConversion.h"

namespace GemRB {

size_t Targets::Count() const
{
	return objects.size();
}

targettype* Targets::RemoveTargetAt(targetlist::iterator& m)
{
	m = objects.erase(m);
	if (m != objects.end()) {
		return &(*m);
	}
	return nullptr;
}

const targettype* Targets::GetLastTarget(ScriptableType type)
{
	targetlist::const_iterator m = objects.end();
	while (m-- != objects.begin()) {
		if (type == ST_ANY || (*m).actor->Type == type) {
			return &(*m);
		}
	}
	return nullptr;
}

const targettype* Targets::GetFirstTarget(targetlist::iterator& m, ScriptableType type)
{
	m = objects.begin();
	while (m != objects.end()) {
		if (type != ST_ANY && (*m).actor->Type != type) {
			m++;
			continue;
		}
		return &(*m);
	}
	return nullptr;
}

const targettype* Targets::GetNextTarget(targetlist::iterator& m, ScriptableType type)
{
	m++;
	while (m != objects.end()) {
		if (type != ST_ANY && (*m).actor->Type != type) {
			m++;
			continue;
		}
		return &(*m);
	}
	return nullptr;
}

Scriptable* Targets::GetTarget(unsigned int index, ScriptableType type)
{
	targetlist::iterator m = objects.begin();
	while (m != objects.end()) {
		if (type == ST_ANY || (*m).actor->Type == type) {
			if (!index) {
				return (*m).actor;
			}
			index--;
		}
		++m;
	}
	return nullptr;
}

void Targets::AddTarget(Scriptable* target, unsigned int distance, int flags)
{
	if (!target) {
		return;
	}

	switch (target->Type) {
		case ST_ACTOR:
			// I don't know if unselectable actors are targetable by script
			// if yes, then remove GA_SELECT
			if (flags && !Scriptable::As<Actor>(target)->ValidTarget(flags)) {
				return;
			}
			break;
		case ST_GLOBAL:
		case ST_ANY:
			// this doesn't seem a good idea to allow
			return;
		default:
			break;
	}

	targettype newTarget = { target, distance };
	for (auto m = objects.begin(); m != objects.end(); ++m) {
		if ((*m).distance > distance) {
			objects.insert(m, newTarget);
			return;
		}
	}
	objects.push_back(newTarget);
}

void Targets::Clear()
{
	objects.clear();
}

void Targets::dump() const
{
	Log(DEBUG, "GameScript", "Target dump (actors only):");
	for (const auto& object : objects) {
		if (object.actor->Type == ST_ACTOR) {
			Log(DEBUG, "GameScript", "{}", fmt::WideToChar { object.actor->GetName() });
		}
	}
}

void Targets::FilterObjectRect(const Object* oC)
{
	// can't match anything if the second pair of coordinates (or all of them) are unset
	if (oC->objectRect.w <= 0 || oC->objectRect.h <= 0) return;

	for (auto m = objects.begin(); m != objects.end();) {
		if (!IsInObjectRect((*m).actor->Pos, oC->objectRect)) {
			m = objects.erase(m);
		} else {
			++m;
		}
	}
}

}
