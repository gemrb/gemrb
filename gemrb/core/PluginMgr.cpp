/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 */

#include "PluginMgr.h"

#include "win32def.h"

#include "Plugin.h"
#include "ResourceDesc.h"

namespace GemRB {

PluginMgr *PluginMgr::Get()
{
	static PluginMgr mgr;
	return &mgr;
}

PluginMgr::PluginMgr()
{
}

PluginMgr::~PluginMgr()
{
}


bool PluginMgr::IsAvailable(SClass_ID plugintype) const
{
	return plugins.find(plugintype) != plugins.end();
}

Plugin* PluginMgr::GetPlugin(SClass_ID plugintype) const
{
	std::map<SClass_ID, PluginFunc>::const_iterator iter = plugins.find(plugintype);
	if (iter != plugins.end())
		return iter->second();
	return NULL;
}

const std::vector<ResourceDesc>& PluginMgr::GetResourceDesc(const TypeID* type)
{
	return resources[type];
}

bool PluginMgr::RegisterPlugin(SClass_ID id, PluginFunc create)
{
	if (plugins.find(id) != plugins.end())
		return false;
	plugins[id] = create;
	return true;
}

void PluginMgr::RegisterResource(const TypeID* type, ResourceFunc create, const char *ext, ieWord keyType)
{
	resources[type].push_back(ResourceDesc(type,create,ext,keyType));
}

void PluginMgr::RegisterInitializer(void (*func)(void))
{
	intializerFunctions.push_back(func);
}

void PluginMgr::RegisterCleanup(void (*func)(void))
{
	cleanupFunctions.push_back(func);
}

void PluginMgr::RunInitializers() const
{
	for (size_t i = 0; i < intializerFunctions.size(); i++)
		intializerFunctions[i]();
}

void PluginMgr::RunCleanup() const
{
	for (size_t i = 0; i < cleanupFunctions.size(); i++)
		cleanupFunctions[i]();
}

bool PluginMgr::RegisterDriver(const TypeID* type, const char* name, PluginFunc create)
{
	driver_map &map = drivers[type];
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return false;
	map[name] = create;
	return true;
}

Plugin* PluginMgr::GetDriver(const TypeID* type, const char* name)
{
	driver_map &map = drivers[type];
	if (map.begin() == map.end())
		return NULL;
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return iter->second();
	return map.begin()->second();
}

}
