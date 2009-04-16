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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#ifndef PROJSERVER_H
#define PROJSERVER_H

#include "PluginMgr.h"
#include "Projectile.h"

//this represents a line of projectl.ids
class ProjectileEntry
{
public:
	ProjectileEntry()
	{
		memset(this,0,sizeof(ProjectileEntry));
	}
	~ProjectileEntry()
	{
		if (projectile)
			delete projectile;
	}

	ieResRef resname;
	Projectile *projectile;
};

class ExplosionEntry
{
public:
	ExplosionEntry()
	{
		memset(this,0,sizeof(ExplosionEntry));
	}
	ieResRef resource1;
	ieResRef resource2;
};

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//this singleton object serves the projectile objects
class GEM_EXPORT ProjectileServer
{
public:
	ProjectileServer();
	~ProjectileServer();

	Projectile *GetProjectileByIndex(unsigned int idx);
	//it is highly unlikely we need this function
	Projectile *GetProjectileByName(const ieResRef resname);
	//returns the highest projectile id
	unsigned int GetHighestProjectileNumber();
	int InitExplosion();
	ieResRef *GetExplosion(unsigned int idx, int type);
private:
	ProjectileEntry *projectiles; //this is the list of projectiles
	int projectilecount;
	ExplosionEntry *explosions;   //this is the list of explosion resources
	int explosioncount;
	//this method is used internally
	Projectile *GetProjectile(unsigned int idx);
	Projectile *CreateDefaultProjectile(unsigned int idx);
	//creates a clone from the cached projectiles
	Projectile *ReturnCopy(unsigned int idx);
	//returns one of the resource names
};

#endif // PROJSERVER_H

