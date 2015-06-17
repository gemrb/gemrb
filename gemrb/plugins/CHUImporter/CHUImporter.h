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

#include "System/DataStream.h"

namespace GemRB {

/**CHU File Importer Class
  *@author GemRB Developement Team
  */

class CHUImporter : public GUIFactory {
private:
	DataStream* str;
//	bool autoFree;
	ieDword WindowCount, CTOffset, WEOffset;
public: 
	CHUImporter();
	~CHUImporter();
	/** Returns the number of available windows */
	unsigned int GetWindowsCount();
	bool LoadWindowPack(const ResRef&);
	/** Returns the i-th window in the Previously Loaded Stream */
	Window* GetWindow(ScriptingId) const;
	/** This function loads all available windows from the 'stream' parameter. */
	bool Open(DataStream* stream);
};

}

#endif
