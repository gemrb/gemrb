// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include "Strings/StringMap.h"
#include "System/VFS.h"

namespace GemRB {

enum class PluginFlagsType {
	NORMAL,
	SKIP,
	DELAY
};

using plugin_flags_t = StringMap<PluginFlagsType>;

/**
 * Loads GemRB plugins from shared libraries or DLLs.
 *
 * It goes over all appropriately named files in PluginPath directory
 * and tries to load them one after another.
 */
void LoadPlugins(const path_t& pluginpath, const plugin_flags_t& flags);

}

#endif
