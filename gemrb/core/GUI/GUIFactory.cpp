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

#include "GUIFactory.h"

namespace GemRB {

Window* GUIFactory::CreateWindow(ScriptingId winId, const Region& frame, Sprite2D* bg) const
{
	assert(winmgr);
	Window* win = winmgr->MakeWindow(frame);
	win->GetScriptingRef(winId);
	win->SetBackground(bg);
	//strcpy( win->WindowPack, winPack );
	return win;
}

}
