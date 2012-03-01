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

#include "ProjectileServer.h"

#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "ProjectileMgr.h"
#include "SymbolMgr.h"

namespace GemRB {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_PROJ_IDX  0x1fff

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
	DataStream* str = gamedata->GetResource( projectiles[idx].resname, IE_PRO_CLASS_ID );
	PluginHolder<ProjectileMgr> sm(IE_PRO_CLASS_ID);
	if (!sm) {
		delete ( str );
		return CreateDefaultProjectile(idx);
	}
	if (!sm->Open(str)) {
		return CreateDefaultProjectile(idx);
	}
	Projectile *pro = new Projectile();
	projectiles[idx].projectile = pro;
	pro->SetIdentifiers(projectiles[idx].resname, idx);

	sm->GetProjectile( pro );
	int Type = 0xff;

	if(pro->Extension) {
		Type = pro->Extension->ExplType;
	}
	if(Type<0xff) {
		ieResRef const *res;

		//fill the spread field according to the hardcoded explosion type
		res = GetExplosion(Type,0);
		if(res) {
			strnuprcpy(pro->Extension->Spread,*res,sizeof(ieResRef)-1);
		}
	
		//if the hardcoded explosion type has a center animation
		//override the VVC animation field with it
		res = GetExplosion(Type,1);
		if(res) {
			pro->Extension->AFlags|=PAF_VVC;
			strnuprcpy(pro->Extension->VVCRes,*res,sizeof(ieResRef)-1);
		}

		//fill the secondary spread field out
		res = GetExplosion(Type,2);
		if(res) {
			strnuprcpy(pro->Extension->Secondary,*res,sizeof(ieResRef)-1);
		}

		//the explosion sound, used for the first explosion
		//(overrides an original field)
		res = GetExplosion(Type,3);
		if(res) {
			strnuprcpy(pro->Extension->SoundRes,*res,sizeof(ieResRef)-1);
		}

		//the area sound (used for subsequent explosions)
		res = GetExplosion(Type,4);
		if(res) {
			strnuprcpy(pro->Extension->AreaSound,*res,sizeof(ieResRef)-1);
		}

		//fill the explosion/spread animation flags
		pro->Extension->APFlags = GetExplosionFlags(Type);
	}

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
		//cannot handle 0xff and it is easier to set up the fields
		//without areapro.2da anyway. So this isn't a restriction
		if(rows>254) {
			rows=254;
		}
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

unsigned int ProjectileServer::PrepareSymbols(Holder<SymbolMgr> projlist) {
	unsigned int count = 0;

	unsigned int rows = (unsigned int) projlist->GetSize();
	while(rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value>MAX_PROJ_IDX) {
			//value = MAX_PROJ_IDX;
			Log(WARNING, "ProjectileServer", "Too high projectilenumber");
			continue; // ignore
		}
		if (value>(unsigned int) count) {
			count = (unsigned int) value;
		}
	}

	return count;
}

void ProjectileServer::AddSymbols(Holder<SymbolMgr> projlist) {
	unsigned int rows = (unsigned int) projlist->GetSize();
	while(rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value>MAX_PROJ_IDX) {
			continue;
		}
		if (value >= (unsigned int)projectilecount) {
			// this should never happen!
			error("ProjectileServer", "Too high projectilenumber while adding projectiles\n");
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
	Holder<SymbolMgr> gemprojlist = core->GetSymbol(gemresource);
	unsigned int resource = core->LoadSymbol("projectl");
	Holder<SymbolMgr> projlist = core->GetSymbol(resource);

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
			Log(ERROR, "ProjectileServer", "Problem with explosions");
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
			Log(ERROR, "ProjectileServer", "Problem with explosions");
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

}
