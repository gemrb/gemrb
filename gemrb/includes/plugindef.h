/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**
 * @file plugindef.h
 * Macros for defining plugins.
 * @author The GemRB Project
 *
 * This file should be included once in each plugin. This file defines several
 * entry points to the plugin and a set of macros to describe the contents
 * of the plugin.
 *
 * A typical use is
 * @code
 * #include "plugindef.h"
 *
 * GEMRB_PLUGIN(0xD768B1, "BMP File Reader")
 * PLUGIN_IE_RESOURCE(BMPImporter, "bmp", IE_BMP_CLASS_ID)
 * END_PLUGIN()
 * @endcode
 *
 * The plugin description block should start with a call to GEMRB_PLUGIN and
 * end with a call to END_PLUGIN, and have only calls to PLUGIN_* macros
 * defined here in between.
 *
 * @def GEMRB_PLUGIN
 * Starts a plugin declaration block
 * @param[in] id Arbitrary unique to distinguish loadable modules.
 * @param[in] desc Description of loadable module.
 *
 * PluginMgr will not load multiple plugins with the same id.
 *
 * @def PLUGIN_CLASS
 * Register a class to be accessed through PluginMgr::GetPlugin
 * @param[in] id Identifier to refer to this class.
 * @param[in] cls Class to register. Must be a descendent of Plugin.
 *
 * PluginMgr will not register multiple classes with the same id, but
 * will report an error and unload the module.
 *
 * @def PLUGIN_DRIVER
 * Register a class to be accessed through PluginMgr::GetDriver.
 * @param[in] cls Class to register. Must be a descendent of Plugin.
 * @param[in] name
 *
 * PluginMgr will not register multiple classes with the same name.
 *
 * @def PLUGIN_RESOURCE
 * Registers a resource through ResourceManager.
 * @param[in] cls Class to register.
 * @param[in] ext Extension of resource files.
 *
 * The class given must derive from a subclass of Resource that
 * contains a static member ID of type TypeID. Any number of class
 * extension pairs can be registerd. They will be tried in turn when a
 * resource of the given subclass is requested.
 *
 * If the resource exists in bif files, then \ref{PLUGIN_IE_RESOURCE}
 * should be used instead.
 *
 * @def PLUGIN_IE_RESOURCE
 * Registers a resource through ResourceManager.
 * @param[in] ie_id Type id that appears in BIF files.
 *
 * See \ref{PLUGIN_RESOURCE} for details. The ie_id will be used when
 * searching chitin.key.
 *
 * @def PLUGIN_INITIALIZER
 * Registers a function to do global intialization.
 * @param[in] func Function to call at startup.
 *
 * This function is called during Interface initialization.
 *
 * @def PLUGIN_CLEANUP
 * Registers a function to do global cleanup
 * @param[in] func Function to call at shutdown.
 *
 * This function is called during Interface cleanup.
 *
 * @def END_PLUGIN
 * End a plugin declaration block.
 */

#ifndef PLUGINDEF_H
#define PLUGINDEF_H

#include "exports.h"
#include "PluginMgr.h"

namespace GemRB {

template <typename T>
struct CreatePlugin {
	static Plugin *func()
	{
		return new T();
	}
};

template <typename Res>
struct CreateResource {
	static Resource* func(DataStream *str)
	{
		Res *res = new Res();
		if (res->Open(str)) {
			return res;
		} else {
			delete res;
			return NULL;
		}
	}
};

}

#ifndef STATIC_LINK

#ifdef WIN32
#include <windows.h>

BOOL APIENTRY DllMain(HANDLE /*hModule*/, DWORD  /*ul_reason_for_call*/,
	LPVOID /*lpReserved*/)
{
	return true;
}

#endif

GEM_EXPORT_DLL const char* GemRBPlugin_Version()
{
	return VERSION_GEMRB;
}

#define GEMRB_PLUGIN(id, desc)					\
	namespace { using namespace GemRB;			\
	GEM_EXPORT_DLL PluginID GemRBPlugin_ID() {		\
		return id;					\
	}							\
	GEM_EXPORT_DLL const char* GemRBPlugin_Description() {	\
		return desc;					\
	}							\
	GEM_EXPORT_DLL bool GemRBPlugin_Register(PluginMgr *mgr) {

#define PLUGIN_CLASS(id, cls)					\
	if (!mgr->RegisterPlugin(id, &CreatePlugin<cls>::func ))\
		return false;

#define PLUGIN_DRIVER(cls, name)					\
	mgr->RegisterDriver(&cls::ID, name, &CreatePlugin<cls>::func ); \

#define PLUGIN_RESOURCE(cls, ext)				\
	mgr->RegisterResource(&cls::ID, &CreateResource<cls>::func, ext);

#define PLUGIN_IE_RESOURCE(cls, ext, ie_id)			\
	mgr->RegisterResource(&cls::ID, &CreateResource<cls>::func, ext, ie_id);

#define PLUGIN_INITIALIZER(func)				\
		mgr->RegisterInitializer(func);
#define PLUGIN_CLEANUP(func)					\
		mgr->RegisterCleanup(func);

		/* mgr is not null (this makes mgr used) */
#define END_PLUGIN()						\
		return mgr!=0;					\
	}							\
	}

#else /* STATIC_LINK */

#define GEMRB_PLUGIN(id, desc)	\
	namespace { using namespace GemRB;			\
		bool doRegisterPlugin = (

#define PLUGIN_CLASS(id, cls)					\
		PluginMgr::Get()->RegisterPlugin(id, &CreatePlugin<cls>::func ),

#define PLUGIN_DRIVER(cls, name)					\
		PluginMgr::Get()->RegisterDriver(&cls::ID, name, &CreatePlugin<cls>::func ),

#define PLUGIN_RESOURCE(cls, ext)				\
		PluginMgr::Get()->RegisterResource(&cls::ID, &CreateResource<cls>::func, ext),

#define PLUGIN_IE_RESOURCE(cls, ext, ie_id)			\
		PluginMgr::Get()->RegisterResource(&cls::ID, &CreateResource<cls>::func, ext, ie_id),

#define PLUGIN_INITIALIZER(func)				\
		PluginMgr::Get()->RegisterInitializer(func),
#define PLUGIN_CLEANUP(func)					\
		PluginMgr::Get()->RegisterCleanup(func),

#define END_PLUGIN()						\
		true); }

#endif /* STATIC_LINK */

#endif
