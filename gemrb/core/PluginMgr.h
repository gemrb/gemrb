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

#include "exports.h"
#include "win32def.h"
#include "globals.h"
#include "SClassID.h" // For PluginID
#include <vector>
#include <list>
#include <cstring>
#include <map>
#include "ResourceDesc.h"
#include "iless.h"

#ifdef WIN32
typedef HINSTANCE LibHandle;
#else
typedef void *LibHandle;
#endif

class Resource;
class TypeID;
class Plugin;

/**
 * @class PluginMgr
 * Class for loading GemRB plugins from shared libraries or DLLs.
 * It goes over all appropriately named files in PluginPath directory
 * and tries to load them one after another.
 */

class GEM_EXPORT PluginMgr {
public:
	typedef Resource* (*ResourceFunc)(DataStream*);
	typedef Plugin* (*PluginFunc)();
public:
	/** Return global instance of PluginMgr */
	static PluginMgr* Get();
	void LoadPlugins(char* pluginpath);
private:
	PluginMgr();
public: // HACK: MSVC6 is buggy.
	~PluginMgr(void);
private:
	struct PluginDesc {
		LibHandle handle;
		PluginID ID;
		const char *Description;
		bool (*Register)(PluginMgr*);
	};
	std::map< PluginID, PluginDesc> libs;
	std::map< SClass_ID, PluginFunc> plugins;
	std::map< const TypeID*, std::vector<ResourceDesc> > resources;
	/** Array of initializer functions */
	std::vector<void (*)(void)> intializerFunctions;
	/** Array of cleanup functions */
	std::vector<void (*)(void)> cleanupFunctions;
	typedef std::map<const char*, PluginFunc, iless> driver_map;
	std::map<const TypeID*, driver_map> drivers;
public:
	/** Return names of all *.so or *.dll files in the given directory */
	bool FindFiles( char* path, std::list< char* > &files);
	bool IsAvailable(SClass_ID plugintype) const;
	Plugin* GetPlugin(SClass_ID plugintype) const;

	size_t GetPluginCount() const { return plugins.size(); }

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
	void RegisterResource(const TypeID* type, ResourceFunc create, const char *ext, ieWord keyType = 0);

	const std::vector<ResourceDesc>& GetResourceDesc(const TypeID*);

	/**
	 * Registers a static intializer.
	 *
	 * @param[in] init Function to call on startup.
	 */
	void RegisterInitializer(void (*init)(void));
	/**
	 * Registers a static cleanup.
	 *
	 * @param[in] cleanup Function to call on shutdown.
	 */
	void RegisterCleanup(void (*cleanup)(void));

	/** Run intializer functions. */
	void RunInitializers();
	/** Run cleanup functions */
	void RunCleanup();

	/**
	 * Registers a driver plugin
	 *
	 * @param[in] type Base class for driver.
	 * @param[in] name Name of driver.
	 * @param[in] create Function to create instance of plugin.
	 */
	bool RegisterDriver(const TypeID* type, const char* name, PluginFunc create);

	/**
	 * Gets driver of specified type.
	 *
	 * @param[in] type Base class for driver.
	 * @param[in] name Name of driver.
	 *
	 * Tries to get driver associated to name, or falls back to a random one.
	 */
	Plugin* GetDriver(const TypeID* type, const char* name);
};

#endif
