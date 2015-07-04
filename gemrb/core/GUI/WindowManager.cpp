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

#include "GameData.h"
#include "Interface.h"
#include "ImageMgr.h"
#include "Window.h"

#include "defsounds.h"

#define WIN_IT(w) \
std::find(windows.begin(), windows.end(), w)

namespace GemRB {

void WindowManager::RedrawAll() const
{
	for (size_t i=0; i < windows.size(); i++) {
		windows[i]->MarkDirty();
	}
}

bool WindowManager::IsOpenWindow(Window* win) const
{
	WindowList::const_iterator it = WIN_IT(win);
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
	eventMgr.AddEventTap(new MethodCallback<WindowManager, const Event&, bool>(this, &WindowManager::DispatchEvent));
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
	WindowList::iterator it = WIN_IT(win);
	if (it == windows.end()) return false;

	if (it != windows.begin()) {
		if (it != windows.end()) {
			windows.erase(it);
		}
		windows.push_front(win);
	}

	win->SetDisabled(false);
	return true;
}

Window* WindowManager::MakeWindow(const Region& rgn)
{
	Window* win = new Window(rgn, *this);
	windows.push_back(win);

	if (closedWindows.size() > 1) {
		// delete all but the most recent closed window
		// FIXME: this is intended for caching (in case the last window is reopened)
		// but currently there is no way to reopen a window (other than recreating it)
		// IMPORTANT: aside from caching, the last window probably is in a callback that resulted in this new window...
		WindowList::iterator it = closedWindows.begin();
		for (; it != --closedWindows.end();) {
			Window* win = *it;
			delete win;
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
	WindowList::iterator it = WIN_IT(win);
	if (it == windows.end()) return;

	if (win == modalWin) {
		modalWin = NULL;
	}

	bool isFront = it == windows.begin();
	it = windows.erase(it);
	if (it != windows.end()) {
		// the window beneath this must get redrawn
		(*it)->MarkDirty();
		core->PlaySound(DS_WINDOW_CLOSE);
		if (isFront)
			(*it)->Focus();
	}
	closedWindows.push_back(win);

	win->DeleteScriptingRef();
	win->SetDisabled(true);
}

bool WindowManager::DispatchEvent(const Event& event)
{
#define HIT_TEST(e, w) \
((w)->Frame().PointInside(e.mouse.Pos()))

	Window* target = NULL;
	if (IsPresentingModalWindow()) {
		// modal windows absorb all events that intersect
		// but also block events from reaching other windows even
		// when they dont hit
		if (!event.isScreen || HIT_TEST(event, modalWin)) {
			target = modalWin;
		}
	} else {
		WindowList::const_iterator it = windows.begin();
		for (; it != windows.end(); ++it) {
			Window* win = *it;
			if (!win->IsDisabled() && HIT_TEST(event,win)) {
				target = win;
				break;
			}
		}
	}
	if (target) {
		Point winPos = target->ConvertPointFromScreen(event.mouse.Pos());
		switch (event.type) {
			case Event::MouseDown:
				target->DispatchMouseDown(winPos, event.mouse.button, event.mod);
				if (target != windows.front()) {
					FocusWindow(target);
				}
				break;
			case Event::MouseUp:
				target->DispatchMouseUp(winPos, event.mouse.button, event.mod);
				break;
			case Event::MouseMove:
				target->DispatchMouseOver(winPos);
				break;
			case Event::MouseScroll:
				target->DispatchMouseWheelScroll(event.mouse.x, event.mouse.y);
				break;
			default: return false;
		}
		return true;
	}
	return false;
#undef HIT_TEST
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
			video->DrawRect( screen, shieldColor );
			RedrawAll(); // wont actually have any effect until the modal window is dismissed.
			modalShield = true;
		}
		modalWin->Draw();
		return;
	}
	modalShield = false;

	bool drawFrame = true;
	const Region& frontWinFrame = windows.front()->Frame();
	// we have to draw windows from the bottom up so the front window is drawn last
	WindowList::const_reverse_iterator rit = windows.rbegin();
	for (; rit != windows.rend(); ++rit) {
		Window* win = *rit;

		if (win != windows.front() && win->NeedsDraw()) {
			const Region& frame = win->Frame();
			Region intersect = frontWinFrame.Intersect(frame);
			if (!intersect.Dimensions().IsEmpty()) {
				if (intersect == frame) {
					// this window is completely obscured by the front window
					// we dont have to bother drawing it because IE has no concept of translucent windows
					continue;
				} else {
					// only partialy obscured
					// must mark front win as dirty to redraw over the intersection
					windows.front()->MarkDirty();
				}
			}
		}

		const Region& frame = win->Frame();
		if (drawFrame && !(win->flags&WF_BORDERLESS) && (frame.w < screen.w || frame.h < screen.h)) {
			video->SetScreenClip( NULL );

			Sprite2D* edge = WinFrameEdge(0); // left
			video->BlitSprite(edge, 0, 0, true);
			edge = WinFrameEdge(1); // right
			int sideW = edge->Width;
			video->BlitSprite(edge, core->Width - sideW, 0, true);
			edge = WinFrameEdge(2); // top
			video->BlitSprite(edge, sideW, 0, true);
			edge = WinFrameEdge(3); // bottom
			video->BlitSprite(edge, sideW, core->Height - edge->Height, true);
			drawFrame = false; // only need to draw this once ever
		}
		if (win->IsDisabled()) {
			if (win->NeedsDraw()) {
				// Important to only draw if the window itself is dirty
				// controls on greyed out windows shouldnt be updating anyway
				win->Draw();
				Color fill = { 0, 0, 0, 128 };
				video->DrawRect(frame, fill);
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
	if (IsOpenWindow(win)) {
		// only a screen shot of passed win
		win->MarkDirty();
		win->Draw();
		screenshot = video->GetScreenshot( win->Frame() );
		RedrawAll();
		DrawWindows();
	} else {
		screenshot = video->GetScreenshot( screen );
	}
	return screenshot;
}

Sprite2D* WindowManager::WinFrameEdge(int edge) const
{
	std::string refstr = "STON";
	switch (screen.w) {
		case 800:
			refstr += "08";
			break;
		case 1024:
			refstr += "10";
			break;
	}
	switch (edge) {
		case 0:
			refstr += "L";
			break;
		case 1:
			refstr += "R";
			break;
		case 2:
			refstr += "T";
			break;
		case 3:
			refstr += "B";
			break;
	}

	typedef Holder<Sprite2D> FrameImage;
	static std::map<ResRef, FrameImage> frames;

	ResRef ref = refstr.c_str();
	Sprite2D* frame = NULL;
	if (frames.find(ref) != frames.end()) {
		frame = frames[ref].get();
	} else {
		ResourceHolder<ImageMgr> im(ref);
		frame = im->GetSprite2D();
		frames.insert(std::make_pair(ref, frame));
	}
	
	return frame;
}

#undef WIN_IT

}
