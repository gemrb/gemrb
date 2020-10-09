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
#include "Tooltip.h"
#include "Window.h"

#include "defsounds.h"

#define WIN_IT(w) \
std::find(windows.begin(), windows.end(), w)

namespace GemRB {

int WindowManager::ToolTipDelay = 500;
unsigned long WindowManager::TooltipTime = 0;

Holder<Sprite2D> WindowManager::CursorMouseUp;
Holder<Sprite2D> WindowManager::CursorMouseDown;

void WindowManager::SetTooltipDelay(int delay)
{
	ToolTipDelay = delay;
}

WindowManager::WindowManager(Video* vid)
{
	assert(vid);

	cursorFeedback = MOUSE_ALL;

	hoverWin = NULL;
	modalWin = NULL;
	trackingWin = NULL;

	EventMgr::EventCallback cb = METHOD_CALLBACK(&WindowManager::DispatchEvent, this);
	eventMgr.RegisterEventMonitor(cb);

	cb = METHOD_CALLBACK(&WindowManager::HotKey, this);
	eventMgr.RegisterHotKeyCallback(cb, 'f', GEM_MOD_CTRL);
	eventMgr.RegisterHotKeyCallback(cb, GEM_GRAB, 0);

	screen = Region(Point(), vid->GetScreenSize());
	// FIXME: technically we should unset the current video event manager...
	vid->SetEventMgr(&eventMgr);

	gameWin = new Window(screen, *this);
	gameWin->SetFlags(Window::Borderless|View::Invisible, OP_SET);
	gameWin->SetFrame(screen);

	HUDBuf = vid->CreateBuffer(screen, Video::DISPLAY_ALPHA);

	// set the buffer that always gets cleared just in case anything
	// tries to draw
	vid->PushDrawingBuffer(HUDBuf);
	video = vid;
	// TODO: changing screen size should adjust window positions too
	// TODO: how do we get notified if the Video driver changes size?
}

WindowManager::~WindowManager()
{
	DestroyWindows(closedWindows);
	assert(closedWindows.empty());
	DestroyWindows(windows);
	assert(windows.empty());

	video.release();
	delete gameWin;
}

void WindowManager::DestroyWindows(WindowList& list)
{
	WindowList::iterator it = list.begin();
	for (; it != list.end();) {
		Window* win = *it;
		// IMPORTANT: ensure the window (a control subview) isnt executing a callback before deleting it
		if (win->InActionHandler() == false) {
			delete win;
			it = list.erase(it);
		} else {
			++it;
		}
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

/** Show a Window in Modal Mode */
bool WindowManager::PresentModalWindow(Window* win, ModalShadow shadow)
{
	if (!IsOpenWindow( win )) return false;

	OrderFront(win);
	win->SetDisabled(false);
	win->SetFlags(Window::Modal, OP_OR);
	modalWin = win;
	modalShadow = shadow;

	if (win->Flags() & Window::Borderless && !(win->Flags() & Window::NoSounds)) {
		core->PlaySound(DS_WINDOW_OPEN, SFX_CHAN_GUI);
	}

	return true;
}

WindowManager::CursorFeedback WindowManager::SetCursorFeedback(CursorFeedback feedback)
{
	std::swap(feedback, cursorFeedback);
	return feedback;
}

/** Sets a Window on the Top */
bool WindowManager::FocusWindow(Window* win)
{
	if (!IsPresentingModalWindow() && OrderFront(win)) {
		if (gameWin == win) {
			core->SetEventFlag(EF_CONTROL);
		}

		return !win->IsDisabled();
	}
	return false;
}

bool WindowManager::OrderFront(Window* win)
{
	assert(windows.size()); // win should be contained in windows
	win->SetVisible(true);
	return OrderRelativeTo(win, windows.front(), true);
}

bool WindowManager::OrderBack(Window* win)
{
	assert(windows.size()); // win should be contained in windows
	return OrderRelativeTo(win, windows.back(), false);
}

bool WindowManager::OrderRelativeTo(Window* win, Window* win2, bool front)
{
	if (win == NULL || win == win2) {
		return false;
	}
	// FIXME: this should probably account for modal windows
	// shouldnt beable to move non modals in front of modals, nor one modal to infront of another

	Window* oldFront = windows.front();
	// if we only have one window, or the 2 windows are the same it is an automatic success
	if (windows.size() > 1 && win != win2) {
		WindowList::iterator it = WIN_IT(win), it2 = WIN_IT(win2);
		if (it == windows.end() || it2 == windows.end()) return false;

		windows.erase(it);
		// it2 may have become invalid after erase
		it2 = WIN_IT(win2);
		windows.insert((front) ? it2 : ++it2, win);
	}

	Window* frontWin = windows.front();
	if ((front && frontWin == win2) || win == frontWin) {
		TooltipTime = 0;
	}
	
	if (oldFront != frontWin) {
		if (front) {
			if (trackingWin == win2) {
				trackingWin = nullptr;
			}
		} else {
			if (trackingWin == win) {
				trackingWin = nullptr;
			}
		}

		auto event = eventMgr.CreateMouseMotionEvent(eventMgr.MousePos());
		WindowList::const_iterator it = windows.begin();
		hoverWin = NextEventWindow(event, it);
		
		oldFront->FocusLost();
		frontWin->FocusGained();
	}

	return true;
}

Window* WindowManager::MakeWindow(const Region& rgn)
{
	DestroyWindows(closedWindows);

	Window* win = new Window(rgn, *this);
	windows.push_back(win);
	return win;
}

//this function won't delete the window, just queue it for deletion
//it will be deleted when another window is opened
//regardless, the window deleted is inaccessible for gui scripts and
//other high level functions from now
// this is a caching mechanisim in case the window is reopened
void WindowManager::CloseWindow(Window* win)
{
	if (win == nullptr || win == gameWin) {
		return;
	}

	WindowList::iterator it = WIN_IT(win);
	if (it == windows.end()) return;

	if (win == modalWin) {
		if (win->Flags() & Window::Borderless && !(win->Flags() & Window::NoSounds)) {
			core->PlaySound(DS_WINDOW_CLOSE, SFX_CHAN_GUI);
		}

		WindowList::iterator mit = it;
		win->SetFlags(Window::Modal, OP_NAND);
		// find the next modal window
		modalWin = NULL;
		while (++mit != windows.end()) {
			if ((*mit)->Flags() & Window::Modal) {
				modalWin = *mit;
				modalWin->FocusGained();
				break;
			}
		}
	}

	if (win == hoverWin) {
		hoverWin = NULL;
	}

	if (win == trackingWin) {
		trackingWin = NULL;
	}

	bool isFront = it == windows.begin();
	it = windows.erase(it);
	if (it != windows.end()) {
		Window* newFrontWin = *it;
		// the window beneath this must get redrawn
		newFrontWin->MarkDirty();
		if (isFront && newFrontWin->IsVisible()) {
			newFrontWin->Focus();
			// normally Focus() will call FocusGained(), but it
			// this case we must do it manually because we have erased from windows so Focus() will think we already have focus
			newFrontWin->FocusGained();
		}
	}
	closedWindows.push_back(win);

	win->SetVisible(false);
	win->SetDisabled(true);
}

void WindowManager::DestroyAllWindows()
{
	WindowList::iterator it = windows.begin();
	for (; it != windows.end(); ++it) {
		Window* win = *it;
		win->SetFlags(Window::DestroyOnClose, OP_OR); // force delete
		win->Close();
		if (windows.empty()) break;
	}
}

bool WindowManager::HotKey(const Event& event)
{
	if (event.type == Event::KeyDown && event.keyboard.repeats == 1) {
		switch (event.keyboard.keycode) {
			case 'f':
				video->ToggleFullscreenMode();
				return true;
			case GEM_GRAB:
				video->ToggleGrabInput();
				return true;
			default:
				return false;
		}
	}
	return false;
}

#define HIT_TEST(e, w) \
((w)->HitTest((w)->ConvertPointFromScreen(e.mouse.Pos())))

Window* WindowManager::NextEventWindow(const Event& event, WindowList::const_iterator& current)
{
	if (current == windows.end()) {
		// we already we through them all and returned gameWin or modalWin once. there is no target window after gameWin
		return NULL;
	}

	if (IsPresentingModalWindow()) {
		// modal win is always the target for all events no matter what
		// if the window shouldnt handle sreen events outside its bounds (ie negative coords etc)
		// then the Window class should be responsible for bounds checking

		// the NULL return is so that if this is called again after returning modalWindow there is no NextTarget
		current = windows.end(); // invalidate the iterator, no other target is possible.
		return modalWin;
	}

	while (current != windows.end()) {
		Window* win = *current++;
		if (win->IsVisible() && (!event.isScreen || HIT_TEST(event,win))) {
			// NOTE: we want to "target" the first window hit regardless of it being disabled or otherwise
			// we still need to update which window is under the mouse and block events from reaching the windows below
			return win;
		}
	}

	// we made it though with no takers...
	// send it to the game win
	return gameWin;
}

bool WindowManager::DispatchEvent(const Event& event)
{
	if (eventMgr.MouseDown() == false && eventMgr.FingerDown() == false) {
		if (event.type == Event::MouseUp || event.type == Event::TouchUp) {
			if (trackingWin) {
				if (trackingWin->IsDisabled() == false) {
					trackingWin->DispatchEvent(event);
				}

				if (event.type == Event::TouchUp) {
					trackingWin = NULL;
				}
			}

			// we don't deliver mouse up events if there isn't a corresponding mouse down (trackingWin == NULL).
			return bool(trackingWin);
		}

		if (event.type != Event::TouchGesture) {
			trackingWin = NULL;
		}
	} else if (event.isScreen && trackingWin) {
		if (trackingWin->IsDisabled() == false) {
			trackingWin->DispatchEvent(event);
		}
		return true;
	}

	if (windows.empty()) return false;

	if (event.EventMaskFromType(event.type) & Event::AllMouseMask) {
		TooltipTime = GetTickCount();
		
		// handle when mouse leaves the window
		if (hoverWin && HIT_TEST(event, hoverWin) == false) {
			hoverWin->MouseLeave(event.mouse, NULL);
			hoverWin = NULL;
		}
	// handled here instead of as a hotkey, so also gamecontrol can do its thing
	} else if (event.type == Event::KeyDown && event.keyboard.keycode == GEM_TAB) {
		if (TooltipTime + ToolTipDelay > GetTickCount()) {
			TooltipTime -= ToolTipDelay;
		}
	}

	WindowList::const_iterator it = windows.begin();
	while (Window* target = NextEventWindow(event, it)) {
		// disabled windows get no events, but should block them from going to windows below
		if (target->IsDisabled() || target->DispatchEvent(event)) {
			if (event.isScreen && target->IsVisible()) {
				hoverWin = target;
				if (event.type == Event::MouseDown || event.type == Event::TouchDown) {
					trackingWin = target;
				}
			} else if ((target->Flags()&(View::IgnoreEvents|View::Disabled)) == View::Disabled
					   && event.type == Event::KeyDown && event.keyboard.keycode == GEM_ESCAPE) {
				// force close disabled windows if they arent also ignoreing events
				target->Close();
			}
			return true;
		}
	}

	return false;
}

#undef HIT_TEST

void WindowManager::DrawMouse() const
{
	if (cursorFeedback == MOUSE_NONE)
		return;

	Point pos = eventMgr.MousePos();
	DrawCursor(pos);
	DrawTooltip(pos);
}

void WindowManager::DrawCursor(const Point& pos) const
{
	if (cursorFeedback&MOUSE_NO_CURSOR) {
		return;
	}
	// Cursor draw priority:
	// 1. gamewin cursor trumps all (we use this for drag items and some others)
	// 2. hoverwin cursor
	// 3. hoverview cursor
	// 4. WindowManager cursors

	Holder<Sprite2D> cur(gameWin->View::Cursor());
	
	if (!cur && hoverWin) {
		cur = hoverWin->Cursor();
	}

	if (!cur) {
		// no cursor override
		cur = (eventMgr.MouseDown()) ? CursorMouseDown : CursorMouseUp;
	}
	assert(cur); // must have a cursor

	if (hoverWin && hoverWin->IsDisabledCursor()) {
		// draw greayed cursor
		video->BlitGameSprite(cur.get(), pos.x, pos.y, BLIT_GREY|BLIT_BLENDED, ColorGray, NULL);
	} else {
		// draw normal cursor
		video->BlitSprite(cur.get(), pos.x, pos.y);
	}
}

void WindowManager::DrawTooltip(Point pos) const
{
	if (cursorFeedback&MOUSE_NO_TOOLTIPS) {
		return;
	}

	static Tooltip tt = core->CreateTooltip(L"");
	static unsigned long time = 0;
	static Holder<SoundHandle> tooltip_sound = NULL;
	static bool reset = false;

	if (trackingWin) // if the mouse is held down we dont want tooltips
		TooltipTime = GetTickCount();

	if (time != TooltipTime + ToolTipDelay) {
		time = TooltipTime + ToolTipDelay;
		reset = true;
	}

	if (hoverWin && TooltipTime && GetTickCount() >= time) {
		if (reset) {
			// reset the tooltip and restart the sound
			const String& text = hoverWin->TooltipText();
			tt.SetText(text);
			if (tooltip_sound) {
				tooltip_sound->Stop();
				tooltip_sound.release();
			}
			if (text.length()) {
				tooltip_sound = core->PlaySound(DS_TOOLTIP, SFX_CHAN_GUI);
			}
			reset = false;
		}

		// clamp pos so that the TT is all visible (TT draws centered at pos)
		int halfW = tt.TextSize().w/2 + 16;
		int halfH = tt.TextSize().h/2 + 11;
		pos.x = Clamp<int>(pos.x, halfW, screen.w - halfW);
		pos.y = Clamp<int>(pos.y, halfW, screen.h - halfH);

		tt.Draw(pos);
	}
}

void WindowManager::DrawWindowFrame() const
{
	// the window buffers dont have room for the frame
	// we also only need to draw the frame *once* (even if it applies to multiple windows)
	// therefore, draw the frame on its own buffer (above everything else)
	// ... I'm not 100% certain this works for all use cases.
	// if it doesnt... i think it might be better to just forget about the window frames once the game is loaded

	video->SetScreenClip( NULL );

	Holder<Sprite2D> left_edge = WinFrameEdge(0);
	if (left_edge) {
		// we assume if one fails, they all do
		Holder<Sprite2D> right_edge = WinFrameEdge(1);
		Holder<Sprite2D> top_edge = WinFrameEdge(2);
		Holder<Sprite2D> bot_edge = WinFrameEdge(3);

		int left_w = left_edge->Frame.w;
		int right_w = right_edge->Frame.w;
		int v_margin = (screen.h - left_edge->Frame.h) / 2;
		// Also assume top and bottom are the same width.
		int h_margin = (screen.w - left_w - right_w - top_edge->Frame.w) / 2;
		video->BlitSprite(left_edge, h_margin, v_margin);
		video->BlitSprite(right_edge, screen.w - right_w - h_margin, v_margin);
		video->BlitSprite(top_edge, h_margin + left_w, v_margin);
		video->BlitSprite(bot_edge, h_margin + left_w, screen.h - bot_edge->Frame.h - v_margin);
	}
}

WindowManager::HUDLock WindowManager::DrawHUD()
{
	return HUDLock(*this);
}

void WindowManager::DrawWindows() const
{
	HUDBuf->Clear();

	if (!windows.size()) {
		return;
	}

	// draw the game window now (beneath everything else); its not part of the windows collection
	gameWin->SetVisible(true); // gamewin must always be drawn
	gameWin->Draw();

	bool drawFrame = false;
	Window* frontWin = windows.front();
	const Region& frontWinFrame = frontWin->Frame();
	// we have to draw windows from the bottom up so the front window is drawn last
	WindowList::const_reverse_iterator rit = windows.rbegin();
	for (; rit != windows.rend(); ++rit) {
		Window* win = *rit;

		if (!win->IsVisible())
			continue;

		if (win == modalWin) {
			drawFrame = drawFrame || !(win->Flags()&Window::Borderless);
			continue; // will draw this later
		}

		const Region& frame = win->Frame();

		// FYI... this only checks if the front window obscures... could be covered by another window too
		if ((frontWin->Flags()&(Window::AlphaChannel|View::Invisible)) == 0 && win != frontWin && win->NeedsDraw()) {
			Region intersect = frontWinFrame.Intersect(frame);
			if (intersect == frame) {
				// this window is completely obscured by the front window
				// we dont have to bother drawing it because IE has no concept of translucent windows
				continue;
			}
		}

		if (!drawFrame && !(win->Flags()&Window::Borderless) && (frame.w < screen.w || frame.h < screen.h)) {
			// the window requires us to draw the frame border (happens later, on the cursor buffer)
			drawFrame = true;
		}

		if (win->IsDisabled() && win->NeedsDraw()) {
			// Important to only draw if the window itself is dirty
			// controls on greyed out windows shouldnt be updating anyway
			win->Draw();
			Region winrgn(Point(), win->Dimensions());
			video->DrawRect(winrgn, ColorBlack, true, BLIT_HALFTRANS|BLIT_BLENDED);
		} else {
			win->Draw();
		}
	}

	video->PushDrawingBuffer(HUDBuf);

	if (drawFrame) {
		DrawWindowFrame();
	}

	if (modalWin) {
		if (modalShadow != ShadowNone) {
			uint32_t flags = 0;

			if (modalShadow == ShadowGray) {
				flags |= BLIT_HALFTRANS;
			}
			video->DrawRect(screen, ColorBlack, true, flags|BLIT_BLENDED);
		}
		auto& modalBuffer = modalWin->DrawWithoutComposition();
		video->BlitVideoBuffer(modalBuffer, Point(), BLIT_BLENDED);
	}

	if (FadeColor.a > 0) {
		video->DrawRect(screen, FadeColor, true);
	}

	DrawMouse();

	// Be sure to reset this to nothing, else some renderer backends (metal at least) complain when we clear (swapbuffers)
	video->SetScreenClip(NULL);
}

//copies a screenshot into a sprite
Holder<Sprite2D> WindowManager::GetScreenshot(Window* win)
{
	Holder<Sprite2D> screenshot;
	if (win) { // we dont really care if we are managing the window
		// only a screen shot of passed win
		auto& winBuf = win->DrawWithoutComposition();
		screenshot = video->GetScreenshot( Region(Point(), win->Dimensions()), winBuf );
	} else {
		// redraw the windows without the mouse elements
		auto mouseState = SetCursorFeedback(MOUSE_NONE);
		DrawWindows();
		video->SwapBuffers(0);
		screenshot = video->GetScreenshot( screen );
		SetCursorFeedback(mouseState);
	}

	return screenshot;
}

Holder<Sprite2D> WindowManager::WinFrameEdge(int edge) const
{
	std::string refstr = "STON";

	// we probably need a HasResource("CSTON" + width + height) call
	// to check for a custom resource

	if (screen.w >= 800 && screen.w < 1024)
		refstr += "08";
	else if (screen.w >= 1024)
		refstr += "10";

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
	Holder<Sprite2D> frame;
	if (frames.find(ref) != frames.end()) {
		frame = frames[ref].get();
	} else {
		ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(ref);
		if (im) {
			frame = im->GetSprite2D();
		}
		frames.insert(std::make_pair(ref, frame));
	}

	return frame;
}

#undef WIN_IT

}
