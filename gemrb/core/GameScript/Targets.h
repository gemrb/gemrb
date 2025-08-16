/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2024 The GemRB Project
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

#ifndef TARGETS_H
#define TARGETS_H

#include "exports-core.h"

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
	Targets() noexcept {};

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
