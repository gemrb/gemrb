// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GUIFactory.h"

#include "GUIScriptInterface.h"
#include "WindowManager.h"

#include "Logging/Logging.h"

namespace GemRB {

Window* GUIFactory::CreateWindow(ScriptingId winId, const Region& frame) const
{
	assert(winmgr);
	Window* win = winmgr->MakeWindow(frame);
	const WindowScriptingRef* ref = RegisterScriptableWindow(win, winPack, winId);
	if (ref == nullptr) {
		Log(DEBUG, "GUIFactory", "Unable to register window {} from CHU {}", winId, winPack);
	}
	return win;
}

}
