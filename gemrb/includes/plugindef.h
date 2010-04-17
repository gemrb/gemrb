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
 *
 *
 */

/**
 * @file plugindef.h
 * Macros for defining plugins.
 * @author The GemRB Project
 */

#ifndef PLUGINDEF_H
#define PLUGINDEF_H

#include "exports.h"
#include "../plugins/Core/PluginMgr.h"

template <typename T>
Plugin* CreatePlugin()
{
	return new T();
}

template <typename Res>
Resource* CreateResource(DataStream *str)
{
	Res *res = new Res();
	if (res->Open(str)) {
		return res;
	} else {
		delete res;
		return NULL;
	}
}

#ifdef WIN32
#include <windows.h>

BOOL APIENTRY DllMain(HANDLE /*hModule*/, DWORD  /*ul_reason_for_call*/,
	LPVOID /*lpReserved*/)
{
	return TRUE;
}

#endif

GEM_EXPORT_DLL const char* GemRBPlugin_Version()
{
	return VERSION_GEMRB;
}

#define GEMRB_PLUGIN(id, desc)					\
	GEM_EXPORT_DLL PluginID GemRBPlugin_ID() {		\
		return id;					\
	}							\
	GEM_EXPORT_DLL const char* GemRBPlugin_Description() {	\
		return desc;					\
	}							\
	GEM_EXPORT_DLL bool GemRBPlugin_Register(PluginMgr *mgr) {
#define PLUGIN_CLASS(id, cls)					\
		if (!mgr->RegisterPlugin(id, CreatePlugin<cls>))	\
			return false;
#define PLUGIN_RESOURCE(id, cls, ext)				\
		mgr->RegisterResource(id, &CreateResource<cls>, ext);
#define PLUGIN_IE_RESOURCE(id, cls, ext, ie_id)			\
		mgr->RegisterResource(id, &CreateResource<cls>, ext, ie_id);
#define PLUGIN_CLEANUP(func)					\
		core->RegisterCleanup(func);
#define END_PLUGIN()						\
		/* mgr is not null (this makes mgr used) */	\
		return mgr;					\
	}


#endif
