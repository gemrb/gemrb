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
 * Declares GUIFactory class, abstract loader for GUI windows and controls
 * @author The GemRB Project
 */

#ifndef GUIFACTORY_H
#define GUIFACTORY_H

#include "GUI/GUIScriptInterface.h"
#include "Plugin.h"

namespace GemRB {

class DataStream;
class Window;

// Abstract loader for GUI windows (and controls with them).
class GEM_EXPORT GUIFactory : public Plugin {
protected:
	ResRef winPack;
public: 
	GUIFactory() {};
	virtual ~GUIFactory() {};

	/** This function loads all available windows from the 'stream' parameter. */
	virtual bool Open(DataStream* stream) = 0;
	/** Returns the i-th window in the Previously Loaded Stream */
	virtual Window* GetWindow(ScriptingId id) const = 0;
	/** Returns the number of available windows */
	virtual unsigned int GetWindowsCount() = 0;
	/** Loads a WindowPack (CHUI file) in the Window Manager */
	virtual bool LoadWindowPack(const ResRef&) = 0;
	/** Creates a Window in the Window Manager */
	Window* CreateWindow(ScriptingId winId, const Region& rgn, Sprite2D* bg) const;

};

}

#endif
