/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 */

#include "WindowManager.h"

#include "Interface.h"
#include "Window.h"

#include "defsounds.h"

namespace GemRB {

void WindowManager::RedrawAll() const
{
	for (size_t i=0; i < windows.size(); i++) {
		windows[i]->MarkDirty();
	}
}

bool WindowManager::IsOpenWindow(Window* win) const
{
	WindowList::const_iterator it = std::find(windows.begin(), windows.end(), win);
	return it != windows.end();
}

bool WindowManager::IsPresentingModalWindow() const
{
	return modalWin != NULL;
}

WindowManager::WindowManager(Video* vid)
{
	modalWin = NULL;
	modalShadow = ShadowNone;
	SetVideoDriver(vid);
}

/** Show a Window in Modal Mode */
bool WindowManager::MakeModal(Window* win, ModalShadow Shadow)
{
	if (!IsOpenWindow(win) || IsPresentingModalWindow()) return false;

	FocusWindow( win );
	modalWin = win;

	core->PlaySound(DS_WINDOW_OPEN);
	modalShadow = Shadow;
	return true;
}

/** Sets a Window on the Top */
bool WindowManager::FocusWindow(Window* win)
{
	if (!IsOpenWindow(win)) return false;

	WindowList::iterator it = std::find(windows.begin(), windows.end(), win);
	if (it != windows.begin()) {
		if (it != windows.end()) {
			windows.erase(it);
		}
		windows.push_front(win);
	}

	win->SetDisabled(false);
	eventMgr.AddWindow( win );
	eventMgr.SetFocused( win, NULL );
	return true;
}

Window* WindowManager::MakeWindow(const Region& rgn)
{
	Window* win = new Window(rgn, *this);
	windows.push_back(win);

	if (closedWindows.size() > 0) { // change this to 1 when we can reopen closed windows
		// delete all but the most recent closed window
		// FIXME: this is intended for caching (in case the last window is reopened)
		// but currently there is no way to reopen a window (other than recreating it)
		WindowList::iterator it = closedWindows.begin();
		for (; it != --closedWindows.end(); ++it) {
			delete (*it);
			it = closedWindows.erase(it);
		}
	}

	return win;
}

//this function won't delete the window, just queue it for deletion
//it will be deleted when another window is opened
//regardless, the window deleted is inaccessible for gui scripts and
//other high level functions from now
// this is a caching mechanisim in case the window is reopened
void WindowManager::CloseWindow(Window* win)
{
	if (!IsOpenWindow(win)) return;

	if (win == modalWin) {
		modalWin = NULL;
	}

	WindowList::iterator it = windows.begin();
	it = std::find(it, windows.end(), win);
	bool isFront = it == windows.begin();
	// since the window is "deleted" it should be sent to the back
	// just in case it is undeleted later
	it = windows.erase(it);
	if (it != windows.end()) {
		// the window beneath this must get redrawn
		(*it)->MarkDirty();
		core->PlaySound(DS_WINDOW_CLOSE);
		if (isFront)
			(*it)->Focus();
	}
	closedWindows.push_back(win);

	win->SetDisabled(true);
	eventMgr.DelWindow(win);
}

void WindowManager::DrawWindows() const
{
	if (!windows.size()) {
		return;
	}

	static bool modalShield = false;
	if (modalWin) {
		if (!modalShield) {
			// only draw the shield layer once
			Color shieldColor = Color(); // clear
			if (modalShadow == ShadowGray) {
				shieldColor.a = 128;
			} else if (modalShadow == ShadowBlack) {
				shieldColor.a = 0xff;
			}
			video->DrawRect( Region(Point(), screen), shieldColor );
			RedrawAll(); // wont actually have any effect until the modal window is dismissed.
			modalShield = true;
		}
		modalWin->Draw();
		return;
	}
	modalShield = false;

	const Region& frontWinFrame = windows.front()->Frame();
	// we have to draw windows from the bottom up so the front window is drawn last
	WindowList::const_reverse_iterator rit = windows.rbegin();
	for (; rit != windows.rend(); ++rit) {
		Window* win = *rit;

		if (win != windows.front()) {
			const Region& frame = win->Frame();
			Region intersect = frontWinFrame.Intersect(frame);
			if (intersect == frame) {
				// this window is completely obscured by the front window
				// we dont have to bother drawing it because IE has no concept of translucent windows
				continue;
			}
		}

		if (win->IsDisabled()) {
			if (win->NeedsDraw()) {
				// Important to only draw if the window itself is dirty
				// controls on greyed out windows shouldnt be updating anyway
				win->Draw();
				Color fill = { 0, 0, 0, 128 };
				video->DrawRect(win->Frame(), fill);
			}
		} else {
			win->Draw();
		}
	}
}

void WindowManager::SetVideoDriver(Video* vid)
{
	if (vid == video) return;

	RedrawAll();

	// FIXME: technically we should unset the current video event manager...
	if (vid) {
		screen.w = vid->GetWidth();
		screen.h = vid->GetHeight();
		vid->SetEventMgr(&eventMgr);
	}
	video = vid;
	// TODO: changing screen size should adjust window positions too
}

//copies a screenshot into a sprite
Sprite2D* WindowManager::GetScreenshot(Window* win) const
{
	Sprite2D* screenshot = NULL;
	if (win && IsOpenWindow(win)) {
		// only a screen shot of passed win
		win->MarkDirty();
		win->Draw();
		screenshot = video->GetScreenshot( win->Frame() );
		RedrawAll();
		DrawWindows();
	} else {
		screenshot = video->GetScreenshot( Region(Point(), screen) );
	}
	return screenshot;
}

}
