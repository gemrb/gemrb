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
	int strlength = (ieByte *) (&pro->Extension)-(ieByte *) (&pro->Type);
	memset(&pro->Type, 0, strlength );
	projectiles[idx].projectile = pro;
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
	int strlength = (ieByte *) (&pro->Extension)-(ieByte *) (&pro->Type);
	Projectile *old = projectiles[idx].projectile;
	memcpy(&pro->Type, &old->Type, strlength );
	//FIXME: copy extension data too, or don't alter the extension
	if (old->Extension) {
		pro->Extension = old->Extension;
	}
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
			strnuprcpy(explosions[ rows].resource1, explist->QueryField(rows, 0), 8);
			strnuprcpy(explosions[ rows].resource2, explist->QueryField(rows, 1), 8);
		}
	}
	return explosioncount;
}

unsigned int ProjectileServer::GetHighestProjectileNumber()
{
	if (projectilecount>=0) {
		return (unsigned int) projectilecount;
	}

	unsigned int resource = core->LoadSymbol("projectl");
	SymbolMgr *projlist = core->GetSymbol(resource);
	if (projlist) {
		projectilecount = 0;

		unsigned int rows = (unsigned int) projlist->GetSize();
		while(rows--) {
			unsigned int value = projlist->GetValueIndex(rows);
			if (value>MAX_PROJ_IDX) {
				value = MAX_PROJ_IDX;
				printMessage("ProjectileServer","Too high projectilenumber\n", YELLOW);
			}
			if (value>(unsigned int) projectilecount) {
				projectilecount = (unsigned int) value+1;
			}
		}
		projectiles = new ProjectileEntry[projectilecount];

		rows = (unsigned int) projlist->GetSize();
		while(rows--) {
			strnuprcpy(projectiles[ projlist->GetValueIndex(rows)].resname, projlist->GetStringIndex(rows), 8);      
		}
		core->DelSymbol(resource);
	} else {
		projectilecount=1;
		projectiles = new ProjectileEntry[projectilecount];
	}
	return (unsigned int) projectilecount;
}

ieResRef *ProjectileServer::GetExplosion(unsigned int idx, int type)
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
	switch (type) {
		case 0:
			return &explosions[idx].resource1;
		case 1:
			return &explosions[idx].resource2;
	}
	return NULL;
}
