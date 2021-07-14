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

namespace GemRB {

class SymbolMgr;

//the number of resrefs in areapro.2da (before the flags field)
#define AP_RESCNT 5

//this represents a line of projectl.ids
struct ProjectileEntry
{
	~ProjectileEntry()
	{
		delete projectile;
	}

	ResRef resname;
	Projectile *projectile = nullptr;
};

struct ExplosionEntry
{
	ResRef resources[AP_RESCNT];
	int flags = 0;
};

//this singleton object serves the projectile objects
class GEM_EXPORT ProjectileServer
{
public:
	ProjectileServer();
	~ProjectileServer();

	Projectile *GetProjectileByIndex(unsigned int idx);
	//it is highly unlikely we need this function
	Projectile *GetProjectileByName(const ResRef &resname);
	//returns the highest projectile id
	unsigned int GetHighestProjectileNumber();
	int InitExplosion();
	int GetExplosionFlags(unsigned int idx);
	ResRef GetExplosion(unsigned int idx, int type);
	//creates an empty projectile on the fly
	Projectile *CreateDefaultProjectile(unsigned int idx);
private:
	ProjectileEntry *projectiles; //this is the list of projectiles
	int projectilecount;
	ExplosionEntry *explosions;   //this is the list of explosion resources
	int explosioncount;
	// internal function: what is max valid projectile id?
	unsigned int PrepareSymbols(const Holder<SymbolMgr>& projlist);
	// internal function: read projectiles
	void AddSymbols(const Holder<SymbolMgr>& projlist);
	//this method is used internally
	Projectile *GetProjectile(unsigned int idx);
	//creates a clone from the cached projectiles
	Projectile *ReturnCopy(unsigned int idx);
	//returns one of the resource names
};

#endif // PROJSERVER_H
}


