/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2013 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef __GemRB__InterfaceConfig__
#define __GemRB__InterfaceConfig__

#include <string>
#include <unordered_map>

#include "exports.h"

namespace GemRB {

using InterfaceConfig = std::unordered_map<std::string, std::string>;

GEM_EXPORT InterfaceConfig LoadFromArgs(int argc, char *argv[]);
GEM_EXPORT InterfaceConfig LoadFromCFG(const char* file);

}

#endif /* defined(__GemRB__InterfaceConfig__) */
