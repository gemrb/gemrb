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

#include "Logging/Logging.h"

namespace GemRB {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_PROJ_IDX 0x1fff

ProjectileServer::ProjectileServer() noexcept
{
	// built-in gemrb projectiles and game/mod-provided projectiles
	unsigned int gemresource = core->LoadSymbol("gemprjtl");
	auto gemprojlist = core->GetSymbol(gemresource);
	unsigned int resource = core->LoadSymbol("projectl");
	auto projlist = core->GetSymbol(resource);
	size_t projectilecount = 0;
	// first, we must calculate how many projectiles we have
	if (gemprojlist) {
		projectilecount = PrepareSymbols(gemprojlist) + 1;
	}
	if (projlist) {
		size_t temp = PrepareSymbols(projlist) + 1;
		if (temp > projectilecount)
			projectilecount = temp;
	}

	// then, allocate space for them all
	if (projectilecount == 0) {
		// no valid projectiles files..
		projectilecount = 1;
	}
	projectiles.resize(projectilecount);

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

	AutoTable explist = gamedata->LoadTable("areapro");
	if (explist) {
		TableMgr::index_t rows = explist->GetRowCount();
		//cannot handle 0xff and it is easier to set up the fields
		//without areapro.2da anyway. So this isn't a restriction
		if (rows > 254) {
			rows = 254;
		}
		explosions.resize(rows);
		while (rows) {
			--rows;
			for (int i = 0; i < AP_RESCNT; i++) {
				explosions[rows].resources[i] = explist->QueryField(rows, i);
			}
			explosions[rows].flags = explist->QueryFieldSigned<int>(rows, AP_RESCNT);
		}
	}
}

Projectile* ProjectileServer::CreateDefaultProjectile(size_t idx)
{
	Projectile* pro = new Projectile();

	//take care, this projectile is not freed up by the server
	if (idx == (unsigned int) ~0) {
		return pro;
	}

	pro->SetIdentifiers(projectiles[idx].resname, idx);
	projectiles[idx].projectile = std::make_unique<Projectile>(*pro);

	return pro;
}

//this function can return only projectiles listed in projectl.ids
Projectile* ProjectileServer::GetProjectileByName(const ResRef& resname)
{
	if (!core->IsAvailable(IE_PRO_CLASS_ID)) {
		return NULL;
	}
	size_t idx = GetHighestProjectileNumber();
	while (idx--) {
		if (resname == projectiles[idx].resname) {
			return GetProjectile(idx);
		}
	}
	return NULL;
}

Projectile* ProjectileServer::GetProjectileByIndex(size_t idx)
{
	if (!core->IsAvailable(IE_PRO_CLASS_ID)) {
		return NULL;
	}
	if (idx >= GetHighestProjectileNumber()) {
		return GetProjectile(0);
	}

	return GetProjectile(idx);
}

Projectile* ProjectileServer::ReturnCopy(size_t idx)
{
	Projectile* pro = new Projectile(*projectiles[idx].projectile);
	pro->SetIdentifiers(projectiles[idx].resname, idx);
	return pro;
}

Projectile* ProjectileServer::GetProjectile(size_t idx)
{
	if (projectiles[idx].projectile) {
		return ReturnCopy(idx);
	}
	DataStream* str = gamedata->GetResourceStream(projectiles[idx].resname, IE_PRO_CLASS_ID);
	PluginHolder<ProjectileMgr> sm = MakePluginHolder<ProjectileMgr>(IE_PRO_CLASS_ID);
	if (!sm) {
		delete str;
		return CreateDefaultProjectile(idx);
	}
	if (!sm->Open(str)) {
		return CreateDefaultProjectile(idx);
	}
	Projectile* pro = new Projectile();
	pro->SetIdentifiers(projectiles[idx].resname, idx);

	sm->GetProjectile(pro);
	int Type = 0xff;

	if (pro->Extension) {
		Type = pro->Extension->ExplType;
	}
	if (Type < 0xff) {
		ResRef res;

		//fill the spread field according to the hardcoded explosion type
		res = GetExplosion(Type, 0);
		if (res) {
			pro->Extension->Spread = res;
		}

		//if the hardcoded explosion type has a center animation
		//override the VVC animation field with it
		res = GetExplosion(Type, 1);
		if (res) {
			pro->Extension->AFlags |= PAF_VVC;
			pro->Extension->VVCRes = res;
		}

		//fill the secondary spread field out
		res = GetExplosion(Type, 2);
		if (res) {
			pro->Extension->Secondary = res;
		}

		//the explosion sound, used for the first explosion
		//(overrides an original field)
		res = GetExplosion(Type, 3);
		if (res) {
			pro->Extension->SoundRes = res;
		}

		//the area sound (used for subsequent explosions)
		res = GetExplosion(Type, 4);
		if (res) {
			pro->Extension->AreaSound = res;
		}

		//fill the explosion/spread animation flags
		if (Type < static_cast<int>(explosions.size())) {
			pro->Extension->APFlags = explosions[Type].flags;
		}
	}

	projectiles[idx].projectile = std::make_unique<Projectile>(*pro);
	return pro;
}

size_t ProjectileServer::PrepareSymbols(const PluginHolder<SymbolMgr>& projlist) const
{
	size_t count = 0;

	size_t rows = projlist->GetSize();
	while (rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value > MAX_PROJ_IDX) {
			Log(WARNING, "ProjectileServer", "Too high projectilenumber");
			continue; // ignore
		}
		if (value > count) {
			count = value;
		}
	}

	return count;
}

void ProjectileServer::AddSymbols(const PluginHolder<SymbolMgr>& projlist)
{
	size_t rows = projlist->GetSize();
	while (rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value > MAX_PROJ_IDX) {
			continue;
		}
		projectiles[value].resname = ResRef(projlist->GetStringIndex(rows));
	}
}

size_t ProjectileServer::GetHighestProjectileNumber() const
{
	return projectiles.size();
}

ResRef ProjectileServer::GetExplosion(size_t idx, int type)
{
	if (idx >= explosions.size()) {
		return ResRef();
	}
	ResRef const ret = explosions[idx].resources[type];
	if (ret.IsEmpty() || IsStar(ret)) return ResRef();

	return ret;
}

}
