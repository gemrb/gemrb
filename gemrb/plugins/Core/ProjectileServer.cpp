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
 * $Id$
 *
 */

#include "ProjectileServer.h"
#include "Interface.h"
#include "ResourceMgr.h"
#include "SymbolMgr.h"
#include "ProjectileMgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_PROJ_IDX  1023

ProjectileServer::ProjectileServer()
{
	projectilecount = -1;
	projectiles = NULL;
	explosioncount = -1;
	explosions = NULL;
}

ProjectileServer::~ProjectileServer()
{
	if (projectiles) {
		delete[] projectiles;
	}
	if (explosions) {
		delete[] explosions;
	}
}

Projectile *ProjectileServer::CreateDefaultProjectile(unsigned int idx)
{
	Projectile *pro = new Projectile();
	//int strlength = (ieByte *) (&pro->Extension)-(ieByte *) (&pro->Type);
	//memset(&pro->Type, 0, strlength );
	int strlength = (ieByte *) (&pro->Extension)-(ieByte *) (&pro->Speed);
	memset(&pro->Speed, 0, strlength );

	//take care, this projectile is not freed up by the server
	if(idx==(unsigned int) ~0 ) {
		return pro;
	}

	projectiles[idx].projectile = pro;
	pro->SetIdentifiers(projectiles[idx].resname, idx);
	return ReturnCopy(idx);
}

//this function can return only projectiles listed in projectl.ids
Projectile *ProjectileServer::GetProjectileByName(const ieResRef resname)
{
	if (!core->IsAvailable(IE_PRO_CLASS_ID)) {
		return NULL;
	}
	unsigned int idx=GetHighestProjectileNumber();
	while(idx--) {
		if (!strnicmp(resname, projectiles[idx].resname,8) ) {
			return GetProjectile(idx);
		}
	}
	return NULL;
}

Projectile *ProjectileServer::GetProjectileByIndex(unsigned int idx)
{
	if (!core->IsAvailable(IE_PRO_CLASS_ID)) {
		return NULL;
	}
	if (idx>=GetHighestProjectileNumber()) {
		return GetProjectile(0);
	}

	return GetProjectile(idx);
}

Projectile *ProjectileServer::ReturnCopy(unsigned int idx)
{
	Projectile *pro = new Projectile();
	Projectile *old = projectiles[idx].projectile;
	//int strlength = (ieByte *) (&pro->Extension)-(ieByte *) (&pro->Type);
	//memcpy(&pro->Type, &old->Type, strlength );
	int strlength = (ieByte *) (&pro->Extension)-(ieByte *) (&pro->Speed);
	memcpy(&pro->Speed, &old->Speed, strlength );
	//FIXME: copy extension data too, or don't alter the extension
	if (old->Extension) {
		pro->Extension = old->Extension;
	}
	pro->SetIdentifiers(projectiles[idx].resname, idx);
	return pro;
}

Projectile *ProjectileServer::GetProjectile(unsigned int idx)
{
	if (projectiles[idx].projectile) {
		return ReturnCopy(idx);
	}
	DataStream* str = core->GetResourceMgr()->GetResource( projectiles[idx].resname, IE_PRO_CLASS_ID );
	ProjectileMgr* sm = ( ProjectileMgr* ) core->GetInterface( IE_PRO_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return CreateDefaultProjectile(idx);
	}
	if (!sm->Open( str, true )) {
		core->FreeInterface( sm );
		return CreateDefaultProjectile(idx);
	}
	Projectile *pro = new Projectile();
	projectiles[idx].projectile = pro;
	pro->SetIdentifiers(projectiles[idx].resname, idx);

	sm->GetProjectile( pro );
	core->FreeInterface( sm );

	pro->autofree = true;
	return ReturnCopy(idx);
}

int ProjectileServer::InitExplosion()
{
	if (explosioncount>=0) {
		return explosioncount;
	}

	AutoTable explist("areapro");
	if (explist) {
		explosioncount = 0;

		unsigned int rows = (unsigned int) explist->GetRowCount();
		explosioncount = rows;
		explosions = new ExplosionEntry[rows];

		while(rows--) {
			int i;

			for(i=0;i<AP_RESCNT;i++) {
				strnuprcpy(explosions[rows].resources[i], explist->QueryField(rows, i), 8);
			}
			//using i so the flags field will always be after the resources
			explosions[rows].flags = atoi(explist->QueryField(rows,i));
		}
	}
	return explosioncount;
}

unsigned int ProjectileServer::PrepareSymbols(SymbolMgr *projlist) {
	unsigned int count = 0;

	unsigned int rows = (unsigned int) projlist->GetSize();
	while(rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value>MAX_PROJ_IDX) {
			//value = MAX_PROJ_IDX;
			printMessage("ProjectileServer","Too high projectilenumber\n", YELLOW);
			continue; // ignore
		}
		if (value>(unsigned int) count) {
			count = (unsigned int) value;
		}
	}

	return count;
}

void ProjectileServer::AddSymbols(SymbolMgr *projlist) {
	unsigned int rows = (unsigned int) projlist->GetSize();
	while(rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value>MAX_PROJ_IDX) {
			continue;
		}
		if (value >= (unsigned int)projectilecount) {
			// this should never happen!
			printMessage("ProjectileServer","Too high projectilenumber while adding projectiles\n", RED);
			abort();
		}
		strnuprcpy(projectiles[value].resname, projlist->GetStringIndex(rows), 8);
	}
}

unsigned int ProjectileServer::GetHighestProjectileNumber()
{
	if (projectilecount>=0) {
		// already read the projectiles
		return (unsigned int) projectilecount;
	}

	// built-in gemrb projectiles and game/mod-provided projectiles
	unsigned int gemresource = core->LoadSymbol("gemprjtl");
	SymbolMgr *gemprojlist = core->GetSymbol(gemresource);
	unsigned int resource = core->LoadSymbol("projectl");
	SymbolMgr *projlist = core->GetSymbol(resource);

	// first, we must calculate how many projectiles we have
	if (gemprojlist) {
		projectilecount = PrepareSymbols(gemprojlist) + 1;
	}
	if (projlist) {
		unsigned int temp = PrepareSymbols(projlist) + 1;
		if (projectilecount == -1 || temp > (unsigned int)projectilecount)
			projectilecount = temp;
	}

	// then, allocate space for them all
	if (projectilecount == -1) {
		// no valid projectiles files..
		projectilecount = 1;
	}	
	projectiles = new ProjectileEntry[projectilecount];
	
	// finally, we must read the projectile resrefs
	if (projlist) {
		AddSymbols(projlist);
		core->DelSymbol(resource);
	}
	// gemprojlist is second because it always overrides game/mod-supplied projectiles
	if (gemprojlist) {
		AddSymbols(gemprojlist);
		core->DelSymbol(gemresource);
	}

	return (unsigned int) projectilecount;
}

//return various flags for the explosion type
int ProjectileServer::GetExplosionFlags(unsigned int idx)
{
	if (explosioncount==-1) {
		if (InitExplosion()<0) {
			printMessage("ProjectileServer","Problem with explosions\n", RED);
			explosioncount=0;
		}
	}
	if (idx>=(unsigned int) explosioncount) {
		return 0;
	}

	return explosions[idx].flags;
}

ieResRef const *ProjectileServer::GetExplosion(unsigned int idx, int type)
{
	if (explosioncount==-1) {
		if (InitExplosion()<0) {
			printMessage("ProjectileServer","Problem with explosions\n", RED);
			explosioncount=0;
		}
	}
	if (idx>=(unsigned int) explosioncount) {
		return NULL;
	}
	ieResRef const *ret = NULL;

	ret = &explosions[idx].resources[type];
	if (ret && *ret[0]=='*') ret = NULL;

	return ret;
}
