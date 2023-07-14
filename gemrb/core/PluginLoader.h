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

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <string>
#include <unordered_map>

namespace GemRB {

enum PluginFlagsType {
	PLF_NORMAL,
	PLF_SKIP,
	PLF_DELAY
};

using plugin_flags_t = std::unordered_map<std::string, PluginFlagsType>;

/**
 * Loads GemRB plugins from shared libraries or DLLs.
 *
 * It goes over all appropriately named files in PluginPath directory
 * and tries to load them one after another.
 */
void LoadPlugins(const char* pluginpath, const plugin_flags_t& flags);

}

#endif
