// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Declares GUIFactory class, abstract loader for GUI windows and controls
 * @author The GemRB Project
 */

#ifndef GUIFACTORY_H
#define GUIFACTORY_H

#include "Plugin.h"
#include "ScriptEngine.h"

#include "GUI/Window.h"

namespace GemRB {

class Window;
class WindowManager;

// Abstract loader for GUI windows (and controls with them).
class GEM_EXPORT GUIFactory : public ImporterBase {
protected:
	ResRef winPack;
	WindowManager* winmgr = nullptr;

public:
	void SetWindowManager(WindowManager& mgr) { winmgr = &mgr; }

	/** Returns the i-th window in the Previously Loaded Stream */
	virtual Window* GetWindow(ScriptingId id) const = 0;
	/** Returns the number of available windows */
	virtual unsigned int GetWindowsCount() const = 0;
	/** Loads a WindowPack (CHUI file) in the Window Manager */
	virtual bool LoadWindowPack(const ScriptingGroup_t&) = 0;
	/** Creates a Window in the Window Manager */
	Window* CreateWindow(ScriptingId winId, const Region& rgn) const;
};

}

#endif
