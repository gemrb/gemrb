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
 *
 *
 */

/**
 * @file PluginMgr.h
 * Declares PluginMgr, loader for GemRB plugins
 * @author The GemRB Project
 */

#ifndef PLUGINMGR_H
#define PLUGINMGR_H

#include "SClassID.h" // For PluginID
#include "exports.h"
#include "globals.h"
#include "Plugin.h"
#include "Plugins/ImporterPlugin.h"
#include "ResourceDesc.h"

#include <cstring>
#include <list>
#include <map>
#include <vector>

namespace GemRB {

class TypeID;
struct CoreSettings;

/**
 * @class PluginMgr
 * Class for loading GemRB plugins from shared libraries or DLLs.
 * It goes over all appropriately named files in PluginPath directory
 * and tries to load them one after another.
 */

class GEM_EXPORT PluginMgr {
public:
	using ResourceFunc = ResourceHolder<Resource> (*)(DataStream*);
	using PluginFunc = PluginHolder<Plugin> (*)();
public:
	/** Return global instance of PluginMgr */
	static PluginMgr* Get();
private:
	PluginMgr() noexcept {};
private:
	std::map< SClass_ID, PluginFunc> plugins;
	std::map< const TypeID*, std::vector<ResourceDesc> > resources;
	/** Array of initializer functions */
	std::vector<void (*)(const CoreSettings&)> initializerFunctions;
	/** Array of cleanup functions */
	std::vector<void (*)(void)> cleanupFunctions;
	using driver_map = std::map<std::string, PluginFunc>;
	std::map<const TypeID*, driver_map> drivers;
public:
	size_t GetPluginCount() const { return plugins.size(); }
	bool IsAvailable(SClass_ID plugintype) const;
	PluginHolder<Plugin> GetPlugin(SClass_ID plugintype) const;


	/**
	 * Register class plugin.
	 *
	 * @param[in] id ID used to access plugin.
	 * @param[in] create Function to create instance of plugin.
	 */
	bool RegisterPlugin(SClass_ID id, PluginFunc create);
	/**
	 * Register resource.
	 *
	 * @param[in] type Base class for resource.
	 * @param[in] create Function to create resource from a stream.
	 * @param[in] ext Extension used for resource files.
	 * @param[in] keyType \iespecific Type identifier used in key/biff files.
	 */
	void RegisterResource(const TypeID* type, ResourceFunc create, const path_t& ext, ieWord keyType = 0);

	const std::vector<ResourceDesc>& GetResourceDesc(const TypeID*);

	/**
	 * Registers a static initializer.
	 *
	 * @param[in] init Function to call on startup.
	 */
	void RegisterInitializer(void (*init)(const CoreSettings&));
	/**
	 * Registers a static cleanup.
	 *
	 * @param[in] cleanup Function to call on shutdown.
	 */
	void RegisterCleanup(void (*cleanup)(void));

	/** Run initializer functions. */
	void RunInitializers(const CoreSettings&) const;
	/** Run cleanup functions */
	void RunCleanup() const;

	/**
	 * Registers a driver plugin
	 *
	 * @param[in] type Base class for driver.
	 * @param[in] name Name of driver.
	 * @param[in] create Function to create instance of plugin.
	 */
	bool RegisterDriver(const TypeID* type, const std::string& name, PluginFunc create);

	/**
	 * Gets driver of specified type.
	 *
	 * @param[in] type Base class for driver.
	 * @param[in] name Name of driver.
	 *
	 * Tries to get driver associated to name, or falls back to a random one.
	 */
	PluginHolder<Plugin> GetDriver(const TypeID* type, const std::string& name);
};

template <typename T>
PluginHolder<T> MakePluginHolder(PluginID id) {
	return std::static_pointer_cast<T>(PluginMgr::Get()->GetPlugin(id));
}

template <typename T>
PluginHolder<ImporterPlugin<T>> MakeImporterPluginHolder(PluginID id) {
	return MakePluginHolder<ImporterPlugin<T>>(id);
}

template <typename T>
PluginHolder<T> GetImporter(PluginID id) {
	auto plugin = MakeImporterPluginHolder<T>(id);
	if (plugin) {
		return plugin->GetImporter();
	}
	return nullptr;
}

template <typename T>
PluginHolder<T> GetImporter(PluginID id, DataStream* str) {
	auto plugin = MakeImporterPluginHolder<T>(id);
	if (plugin) {
		return plugin->GetImporter(str);
	}
	delete str;
	return nullptr;
}

}

#endif
