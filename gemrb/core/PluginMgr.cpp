// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PluginMgr.h"

namespace GemRB {

PluginMgr* PluginMgr::Get()
{
	static PluginMgr mgr;
	return &mgr;
}

bool PluginMgr::IsAvailable(SClass_ID plugintype) const
{
	return plugins.find(plugintype) != plugins.end();
}

PluginHolder<Plugin> PluginMgr::GetPlugin(SClass_ID plugintype) const
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

void PluginMgr::RegisterResource(const TypeID* type, ResourceFunc create, const path_t& ext, ieWord keyType)
{
	resources[type].emplace_back(type, create, ext, keyType);
}

void PluginMgr::RegisterInitializer(void (*func)(const CoreSettings&))
{
	initializerFunctions.push_back(func);
}

void PluginMgr::RegisterCleanup(void (*func)(void))
{
	cleanupFunctions.push_back(func);
}

void PluginMgr::RunInitializers(const CoreSettings& config) const
{
	for (const auto initializerFunction : initializerFunctions) {
		initializerFunction(config);
	}
}

void PluginMgr::RunCleanup() const
{
	for (const auto cleanupFunction : cleanupFunctions) {
		cleanupFunction();
	}
}

bool PluginMgr::RegisterDriver(const TypeID* type, const std::string& name, PluginFunc create)
{
	driver_map& map = drivers[type];
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return false;
	map[name] = create;
	return true;
}

PluginHolder<Plugin> PluginMgr::GetDriver(const TypeID* type, const std::string& name)
{
	driver_map& map = drivers[type];
	if (map.begin() == map.end())
		return NULL;
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return iter->second();
	return map.begin()->second();
}

}
