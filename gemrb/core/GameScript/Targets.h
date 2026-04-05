// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TARGETS_H
#define TARGETS_H

#include "exports.h"

#include "Scriptable/Scriptable.h"

namespace GemRB {

class Object;

struct targettype {
	Scriptable* actor; // could be door
	unsigned int distance;
};

using targetlist = std::list<targettype>;

class GEM_EXPORT Targets {
	targetlist objects;

public:
	Targets() noexcept = default;

	size_t Count() const;
	void Pop() { objects.pop_front(); };
	targettype* RemoveTargetAt(targetlist::iterator& m);
	const targettype* GetNextTarget(targetlist::iterator& m, ScriptableType type);
	const targettype* GetLastTarget(ScriptableType type);
	const targettype* GetFirstTarget(targetlist::iterator& m, ScriptableType type);
	Scriptable* GetTarget(unsigned int index, ScriptableType type);
	void AddTarget(Scriptable* target, unsigned int distance, int flags);
	void Clear();
	void FilterObjectRect(const Object* oC);
	void dump() const;
};

}

#endif
