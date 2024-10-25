/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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

#ifndef PROJSERVER_H
#define PROJSERVER_H

#include "exports.h"

#include "Projectile.h"

#include <memory>

namespace GemRB {

class SymbolMgr;

//the number of resrefs in areapro.2da (before the flags field)
#define AP_RESCNT 5

//this singleton object serves the projectile objects
class GEM_EXPORT ProjectileServer {
public:
	ProjectileServer() noexcept;
	ProjectileServer(const ProjectileServer&) = delete;
	ProjectileServer& operator=(const ProjectileServer&) = delete;

	ProjectileServer(ProjectileServer&&) noexcept = default;
	ProjectileServer& operator=(ProjectileServer&&) noexcept = default;

	Projectile* GetProjectileByIndex(size_t idx);
	//it is highly unlikely we need this function
	Projectile* GetProjectileByName(const ResRef& resname);
	//returns the highest projectile id
	size_t GetHighestProjectileNumber() const;
	//creates an empty projectile on the fly
	Projectile* CreateDefaultProjectile(size_t idx);

private:
	//this represents a line of projectl.ids
	struct ProjectileEntry {
		ResRef resname;
		std::unique_ptr<Projectile> projectile;

		ProjectileEntry() noexcept = default;
		~ProjectileEntry() noexcept = default;

		ProjectileEntry(ProjectileEntry&&) noexcept = default;
		ProjectileEntry& operator=(ProjectileEntry&&) noexcept = default;

		ProjectileEntry(const ProjectileEntry&) noexcept = delete;
		ProjectileEntry& operator=(const ProjectileEntry&) noexcept = delete;
	};

	static_assert(std::is_nothrow_move_constructible<ProjectileEntry>::value, "ProjectileEntry should be noexcept MoveConstructible");

	struct ExplosionEntry {
		ResRef resources[AP_RESCNT];
		int flags = 0;
	};

	std::vector<ProjectileEntry> projectiles; //this is the list of projectiles
	std::vector<ExplosionEntry> explosions; //this is the list of explosion resources
	// internal function: what is max valid projectile id?
	size_t PrepareSymbols(const PluginHolder<SymbolMgr>& projlist) const;
	// internal function: read projectiles
	void AddSymbols(const PluginHolder<SymbolMgr>& projlist);
	//this method is used internally
	Projectile* GetProjectile(size_t idx);
	//creates a clone from the cached projectiles
	Projectile* ReturnCopy(size_t idx);
	//returns one of the resource names
	ResRef GetExplosion(size_t idx, int type);
};

}

#endif // PROJSERVER_H
