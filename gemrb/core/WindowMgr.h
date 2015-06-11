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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file WindowMgr.h
 * Declares WindowMgr class, abstract loader for GUI windows and controls
 * @author The GemRB Project
 */

#ifndef WINDOWMGR_H
#define WINDOWMGR_H

#include "GUI/GUIScriptInterface.h"
#include "Plugin.h"

namespace GemRB {

class DataStream;
class Window;

/**
 * @class WindowMgr
 * Abstract loader for GUI windows (and controls with them).
 * Contrary to its name, it does not work as a window manager
 */

class GEM_EXPORT WindowMgr : public Plugin {
public: 
	WindowMgr();
	virtual ~WindowMgr();
	/** This function loads all available windows from the 'stream' parameter. */
	virtual bool Open(DataStream* stream) = 0;
	/** Returns the i-th window in the Previously Loaded Stream */
	virtual Window* GetWindow(ScriptingId id) = 0;
	/** Returns the number of available windows */
	virtual unsigned int GetWindowsCount() = 0;
};

}

#endif
