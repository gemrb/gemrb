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

#ifndef CHUIMPORTER_H
#define CHUIMPORTER_H

#include "GUI/GUIFactory.h"

#include "Streams/DataStream.h"

namespace GemRB {

/**CHU File Importer Class
  *@author GemRB Development Team
  */

class CHUImporter : public GUIFactory {
	ieDword WindowCount = 0;
	ieDword CTOffset = 0;
	ieDword WEOffset = 0;
public:
	/** Returns the number of available windows */
	unsigned int GetWindowsCount() const override;
	bool LoadWindowPack(const ScriptingGroup_t&) override;
	/** Returns the i-th window in the Previously Loaded Stream */
	Window* GetWindow(ScriptingId) const override;
	/** This function loads all available windows from the 'stream' parameter. */
	bool Import(DataStream* stream) override;
};

}

#endif
