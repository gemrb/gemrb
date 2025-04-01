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

#include "defsounds.h"

#include "Debug.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Tooltip.h"
#include "Window.h"

#include "GUI/GUIFactory.h"
#include "GUI/GUIScriptInterface.h"
#include "GUI/GameControl.h"

#define WIN_IT(w) \
	std::find(windows.begin(), windows.end(), w)

namespace GemRB {

tick_t WindowManager::ToolTipDelay = 500;
tick_t WindowManager::TooltipTime = 0;

Holder<Sprite2D> WindowManager::CursorMouseUp;
Holder<Sprite2D> WindowManager::CursorMouseDown;

void WindowManager::SetTooltipDelay(int delay)
{
	ToolTipDelay = delay;
}

WindowManager::WindowManager(PluginHolder<Video> vid, std::shared_ptr<GUIFactory> fact)
	: guifact(std::move(fact)), video(std::move(vid)), tooltip(core->CreateTooltip())
{
	assert(video);
	assert(guifact);

	guifact->SetWindowManager(*this);

	cursorFeedback = MOUSE_ALL;

	EventMgr::EventCallback cb = METHOD_CALLBACK(&WindowManager::DispatchEvent, this);
	EventMgr::RegisterEventMonitor(cb);

	cb = METHOD_CALLBACK(&WindowManager::HotKey, this);
	EventMgr::RegisterHotKeyCallback(cb, 'f', GEM_MOD_CTRL);
	EventMgr::RegisterHotKeyCallback(cb, GEM_GRAB, 0);

	screen = Region(Point(), video->GetScreenSize());
	// FIXME: technically we should unset the current video event manager...
	video->SetEventMgr(&eventMgr);

	gameWin = new Window(screen, *this);
	gameWin->SetFlags(Window::Borderless | View::Invisible, BitOp::SET);
	gameWin->SetFrame(screen);

	HUDBuf = video->CreateBuffer(screen, Video::BufferFormat::DISPLAY_ALPHA);

	// set the buffer that always gets cleared just in case anything
	// tries to draw
	video->PushDrawingBuffer(HUDBuf);
	// TODO: changing screen size should adjust window positions too
	// TODO: how do we get notified if the Video driver changes size?
}

void WindowManager::MarkAllDirty() const
{
	for (auto& window : windows) {
		window->MarkDirty();
	}
}

WindowManager::~WindowManager()
{
	CloseAllWindows();

	DestroyClosedWindows();
	assert(closedWindows.empty());

	delete gameWin;
}

Window* WindowManager::LoadWindow(ScriptingId WindowID, const ScriptingGroup_t& ref, Window::WindowPosition pos)
{
	guifact->LoadWindowPack(ref);

	Window* win = GetWindow(WindowID, ref);
	if (!win) {
		win = guifact->GetWindow(WindowID);
	}
	if (win) {
		assert(win->GetScriptingRef());
		win->SetPosition(pos);
		FocusWindow(win);
	}
	return win;
}

Window* WindowManager::CreateWindow(ScriptingId WindowID, const Region& frame)
{
	// FIXME: this will create a window under the current "window pack"
	// obviously its possible the id can conflict with an existing window
	return guifact->CreateWindow(WindowID, frame);
}

void WindowManager::DestroyClosedWindows()
{
	WindowList::iterator it = closedWindows.begin();
	while (it != closedWindows.end()) {
		Window* win = *it;
		// IMPORTANT: ensure the window (a control subview) isn't executing a callback before deleting it
		if (win->InActionHandler() == false) {
			delete win;
			it = closedWindows.erase(it);
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

Window* WindowManager::ModalWindow() const
{
	Window* fwin = windows.front();
	return fwin->Flags() & Window::Modal ? fwin : nullptr;
}

/** Show a Window in Modal Mode */
bool WindowManager::PresentModalWindow(Window* win)
{
	if (!IsOpenWindow(win)) return false;

	OrderFront(win);
	win->SetDisabled(false);
	win->SetFlags(Window::Modal, BitOp::OR);

	if (win->Flags() & Window::Borderless && !(win->Flags() & Window::NoSounds)) {
		core->GetAudioPlayback().PlayDefaultSound(DS_WINDOW_OPEN, SFXChannel::GUI);
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
	// we can only focus the window if it is above the topmost modal window
	const Window* modalWin = ModalWindow();
	if (modalWin && win != modalWin) {
		auto modalIt = WIN_IT(modalWin);
		auto winIt = std::find(modalIt, windows.end(), win);
		if (winIt == windows.end()) {
			return false;
		}
	}

	if (OrderFront(win)) {
		if (gameWin == win) {
			core->SetEventFlag(EF_CONTROL);
		}

		return !win->IsDisabled();
	}
	return false;
}

bool WindowManager::OrderFront(Window* win)
{
	assert(!windows.empty()); // win should be contained in windows
	win->SetVisible(true);
	return OrderRelativeTo(win, windows.front(), true);
}

bool WindowManager::OrderBack(Window* win)
{
	assert(!windows.empty()); // win should be contained in windows
	return OrderRelativeTo(win, windows.back(), false);
}

bool WindowManager::OrderRelativeTo(Window* win, Window* win2, bool front)
{
	if (win == NULL || win == win2) {
		return false;
	}
	// FIXME: this should probably account for modal windows
	// shouldn't be able to move non modals in front of modals, nor one modal to in front of another

	Window* oldFront = windows.front();
	// if we only have one window, or the 2 windows are the same it is an automatic success
	if (windows.size() > 1 && win != win2) {
		WindowList::iterator it = WIN_IT(win);
		WindowList::iterator it2 = WIN_IT(win2);
		if (it == windows.end() || it2 == windows.end()) return false;

		windows.erase(it);
		// it2 may have become invalid after erase
		it2 = WIN_IT(win2);
		windows.insert(front ? it2 : ++it2, win);
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

		auto event = EventMgr::CreateMouseMotionEvent(EventMgr::MousePos());
		WindowList::const_iterator it = windows.begin();
		hoverWin = NextEventWindow(event, it);

		oldFront->FocusLost();
		frontWin->FocusGained();
	}

	return true;
}

Window* WindowManager::MakeWindow(const Region& rgn)
{
	DestroyClosedWindows();

	Window* win = new Window(rgn, *this);
	windows.push_back(win);
	return win;
}

// this function won't delete the window, just queue it for deletion
// it will be deleted when another window is opened
// regardless, the closed window is inaccessible for gui scripts and
// other high level functions from now
// this is a necessity to prevent crashing when a control callback results
// in the parent window closing which would delete the control before
// returning from the callback
void WindowManager::CloseWindow(Window* win)
{
	if (win == nullptr || win == gameWin) {
		return;
	}

	WindowList::iterator it = WIN_IT(win);
	if (it == windows.end()) return;

	if (win == ModalWindow()) {
		if (win->Flags() & Window::Borderless && !(win->Flags() & Window::NoSounds)) {
			core->GetAudioPlayback().PlayDefaultSound(DS_WINDOW_CLOSE, SFXChannel::GUI);
		}

		win->SetFlags(Window::Modal, BitOp::NAND);
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

void WindowManager::CloseAllWindows() const
{
	for (Window* win : WindowList(windows)) {
		win->SetFlags(Window::DestroyOnClose, BitOp::OR); // force delete
		win->Close();
	}
	assert(windows.empty());
}

bool WindowManager::HotKey(const Event& event) const
{
	if (event.type == Event::KeyDown && event.keyboard.repeats == 1) {
		auto& vars = core->GetDictionary();
		bool fullscreen;
		switch (event.keyboard.keycode) {
			case 'f':
				video->ToggleFullscreenMode();
				fullscreen = (bool) vars.Get("Full Screen", 0);
				vars.Set("Full Screen", !fullscreen);
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

Window* WindowManager::GetFocusWindow() const
{
	if (Window* mwin = ModalWindow()) {
		return mwin;
	}

	for (Window* win : windows) {
		if ((win->Flags() & (View::IgnoreEvents | View::Invisible)) == 0) {
			return win;
		}
	}

	// for all intents and purposes there must always be a window considered to be the focus
	// gameWin is the "root" window so it will be considered the focus if no eligible windows are
	return gameWin;
}

#define HIT_TEST(e, w) \
	((w)->HitTest((w)->ConvertPointFromScreen(e.mouse.Pos())))

Window* WindowManager::NextEventWindow(const Event& event, WindowList::const_iterator& current)
{
	if (current == windows.end()) {
		// we already went through them all and returned gameWin or modalWin once. There is no target window after gameWin
		return NULL;
	}

	if (Window* mwin = ModalWindow()) {
		// modal win is always the target for all events no matter what
		// if the window shouldn't handle screen events outside its bounds (ie. negative coords)
		// then the Window class should be responsible for bounds checking

		// the NULL return is so that if this is called again after returning modalWindow there is no NextTarget
		current = windows.end(); // invalidate the iterator, no other target is possible.
		return mwin;
	}

	if (event.isScreen) {
		while (current != windows.end()) {
			Window* win = *current++;
			if (win->IsVisible() && HIT_TEST(event, win)) {
				// NOTE: we want to "target" the first window hit regardless of it being disabled or otherwise
				// we still need to update which window is under the mouse and block events from reaching the windows below
				return win;
			}
		}
	} else {
		current = windows.end(); // invalidate the iterator, no other target is possible.
		return GetFocusWindow();
	}

	// we made it though with no takers...
	// send it to the game win
	return gameWin;
}

bool WindowManager::DispatchEvent(const Event& event)
{
	if (event.type == Event::EventType::RedrawRequest) {
		MarkAllDirty();
		return true;
	}

	if (EventMgr::MouseDown() == false && EventMgr::FingerDown() == false) {
		if (event.type == Event::MouseUp || event.type == Event::TouchUp) {
			// we don't deliver mouse up events if there isn't a corresponding mouse down (no trackingWin).
			if (!trackingWin) return false;

			if (!trackingWin->IsDisabled() && trackingWin->IsVisible()) {
				trackingWin->DispatchEvent(event);
			}
			trackingWin = nullptr;
			return false;
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

	if (Event::EventMaskFromType(event.type) & Event::AllMouseMask) {
		TooltipTime = GetMilliseconds();

		// handle when mouse leaves the window
		if (hoverWin && HIT_TEST(event, hoverWin) == false) {
			hoverWin->MouseLeave(event.mouse, NULL);
			hoverWin = NULL;
		}
		// handled here instead of as a hotkey, so also gamecontrol can do its thing
	} else if (event.type == Event::KeyDown && event.keyboard.keycode == GEM_TAB) {
		if (TooltipTime + ToolTipDelay > GetMilliseconds()) {
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
			} else if ((target->Flags() & (View::IgnoreEvents | View::Disabled)) == View::Disabled && event.type == Event::KeyDown && event.keyboard.keycode == GEM_ESCAPE) {
				// force close disabled windows if they aren't also ignoring events
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

	Point cursorPos = EventMgr::MousePos();
	Point tooltipPos = cursorPos;

	// pst displays actor name tooltips overhead, not at the mouse position
	const GameControl* gc = core->GetGameControl();
	if (core->HasFeature(GFFlags::ONSCREEN_TEXT) && gc) {
		tooltipPos.y -= gc->GetOverheadOffset();
	}

	if (tooltip.tt.TextSize().IsZero() || core->HasFeature(GFFlags::ONSCREEN_TEXT)) {
		DrawCursor(cursorPos);
	}
	DrawTooltip(tooltipPos);
}

void WindowManager::DrawCursor(const Point& pos) const
{
	if (cursorFeedback & MOUSE_NO_CURSOR) {
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
		cur = (EventMgr::MouseDown()) ? CursorMouseDown : CursorMouseUp;
	}
	assert(cur); // must have a cursor

	if (hoverWin && hoverWin->IsDisabledCursor()) {
		// draw greyed cursor
		video->BlitGameSprite(cur, pos, BlitFlags::GREY | BlitFlags::BLENDED, ColorGray);
	} else {
		// draw normal cursor
		video->BlitSprite(cur, pos);
	}
}

void WindowManager::DrawTooltip(Point pos) const
{
	if (cursorFeedback & MOUSE_NO_TOOLTIPS) {
		return;
	}

	if (trackingWin) // if the mouse is held down we don't want tooltips
		TooltipTime = GetMilliseconds();

	if (tooltip.time != TooltipTime + ToolTipDelay) {
		tooltip.time = TooltipTime + ToolTipDelay;
		tooltip.reset = true;
	}

	if (hoverWin && TooltipTime && GetMilliseconds() >= tooltip.time) {
		if (tooltip.reset) {
			// reset the tooltip and restart the sound
			const String& text = hoverWin->TooltipText();
			tooltip.tt.SetText(text);
			if (tooltip.tooltip_sound) {
				tooltip.tooltip_sound->Stop();
				tooltip.tooltip_sound.reset();
			}
			if (text.length()) {
				tooltip.tooltip_sound = core->GetAudioPlayback().PlayDefaultSound(DS_TOOLTIP, SFXChannel::GUI);
			}
			tooltip.reset = false;
		}

		// clamp pos so that the TT is all visible (TT draws centered at pos)
		int halfW = tooltip.tt.TextSize().w / 2 + 16;
		int halfH = tooltip.tt.TextSize().h / 2 + 11;
		pos.x = Clamp<int>(pos.x, halfW, screen.w - halfW);
		pos.y = Clamp<int>(pos.y, halfW, screen.h - halfH);

		tooltip.tt.Draw(pos);
	} else {
		tooltip.tt.SetText(u"");
	}
}

void WindowManager::DrawWindowFrame(BlitFlags flags) const
{
	// the window buffers don't have room for the frame
	// we also only need to draw the frame *once* (even if it applies to multiple windows)
	// therefore, draw the frame on its own buffer (above everything else)
	// ... I'm not 100% certain this works for all use cases.
	// if it doesn't... i think it might be better to just forget about the window frames once the game is loaded

	video->SetScreenClip(NULL);

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

		const static Color dummy;
		video->BlitGameSprite(left_edge, Point(h_margin, v_margin), flags, dummy);
		video->BlitGameSprite(right_edge, Point(screen.w - right_w - h_margin, v_margin), flags, dummy);
		video->BlitGameSprite(top_edge, Point(h_margin + left_w, v_margin), flags, dummy);
		video->BlitGameSprite(bot_edge, Point(h_margin + left_w, screen.h - bot_edge->Frame.h - v_margin), flags, dummy);
	}
}

WindowManager::HUDLock WindowManager::DrawHUD() const
{
	return HUDLock(*this);
}

void WindowManager::DrawWindows() const
{
	TRACY(ZoneScoped);
	HUDBuf->Clear();

	if (windows.empty()) {
		return;
	}

	// draw the game window now (beneath everything else); it's not part of the windows collection
	if (gameWin->IsVisible()) {
		gameWin->Draw();

		if (FadeColor.a > 0) {
			video->DrawRect(screen, FadeColor, true, BlitFlags::BLENDED);
		}
	} else {
		// something must get drawn or else we get smearing
		// this is kind of a hacky way to clear it, but it works
		auto& buffer = gameWin->DrawWithoutComposition();
		buffer->Clear();
		video->PushDrawingBuffer(buffer);
	}

	bool drawFrame = false;
	const Window* frontWin = windows.front();
	Window* modalWin = ModalWindow();
	const Region& frontWinFrame = frontWin->Frame();
	// we have to draw windows from the bottom up so the front window is drawn last
	WindowList::const_reverse_iterator rit = windows.rbegin();
	for (; rit != windows.rend(); ++rit) {
		Window* win = *rit;

		if (!win->IsVisible())
			continue;

		if (win == modalWin) {
			drawFrame = drawFrame || !(win->Flags() & Window::Borderless);
			continue; // will draw this later
		}

		const Region& frame = win->Frame();

		// FYI... this only checks if the front window obscures... could be covered by another window too
		if ((frontWin->Flags() & (Window::AlphaChannel | View::Invisible)) == 0 && win != frontWin && win->NeedsDraw()) {
			Region intersect = frontWinFrame.Intersect(frame);
			if (intersect == frame) {
				// this window is completely obscured by the front window
				// we dont have to bother drawing it because IE has no concept of translucent windows
				continue;
			}
		}

		if (!drawFrame && !(win->Flags() & Window::Borderless) && (frame.w < screen.w || frame.h < screen.h)) {
			// the window requires us to draw the frame border (happens later, on the cursor buffer)
			drawFrame = true;
		}

		win->Draw();
	}

	video->PushDrawingBuffer(HUDBuf);

	BlitFlags frame_flags = BlitFlags::NONE;
	if (modalWin) {
		if (modalWin->modalShadow != Window::ModalShadow::None) {
			if (modalWin->modalShadow == Window::ModalShadow::Gray) {
				frame_flags |= BlitFlags::HALFTRANS;
			}
			video->DrawRect(screen, ColorBlack, true, frame_flags);
		}
		auto& modalBuffer = modalWin->DrawWithoutComposition();
		video->BlitVideoBuffer(modalBuffer, Point(), BlitFlags::BLENDED);
	}

	if (drawFrame) {
		DrawWindowFrame(frame_flags);
	}

	if (InDebugMode(DebugMode::WINDOWS)) {
		// ensure this is drawing over the window frames
		if (trackingWin) {
			Region r = trackingWin->Frame();
			r.ExpandAllSides(5);
			video->DrawRect(r, ColorRed, false);
		}

		if (hoverWin) {
			Region r = hoverWin->Frame();
			r.ExpandAllSides(10);
			video->DrawRect(r, ColorWhite, false);
		}
	}

	DrawMouse();

	// Be sure to reset this to nothing, else some renderer backends (metal at least) complain when we clear (swapbuffers)
	video->SetScreenClip(NULL);
}

//copies a screenshot into a sprite
Holder<Sprite2D> WindowManager::GetScreenshot(Window* win)
{
	Holder<Sprite2D> screenshot;
	if (win) { // we don't really care if we are managing the window
		// only a screen shot of passed win
		auto& winBuf = win->DrawWithoutComposition();
		screenshot = video->GetScreenshot(Region(Point(), win->Dimensions()), winBuf);
	} else {
		// redraw the windows without the mouse elements
		auto mouseState = SetCursorFeedback(MOUSE_NONE);
		DrawWindows();
		video->SwapBuffers(0);
		screenshot = video->GetScreenshot(screen);
		SetCursorFeedback(mouseState);
	}

	return screenshot;
}

Holder<Sprite2D> WindowManager::WinFrameEdge(int edge) const
{
	ResRef refstr = "STON";

	// we probably need a HasResource("CSTON" + width + height) call
	// to check for a custom resource

	if (screen.w >= 800 && screen.w < 1024)
		refstr.Append("08");
	else if (screen.w >= 1024)
		refstr.Append("10");

	switch (edge) {
		case 0:
			refstr.Append("L");
			break;
		case 1:
			refstr.Append("R");
			break;
		case 2:
			refstr.Append("T");
			break;
		case 3:
			refstr.Append("B");
			break;
	}

	Holder<Sprite2D> frame;
	if (winframes.find(refstr) != winframes.end()) {
		frame = winframes[refstr];
	} else {
		auto im = gamedata->GetResourceHolder<ImageMgr>(refstr);
		if (im) {
			frame = im->GetSprite2D();
		}
		winframes.emplace(refstr, frame);
	}

	return frame;
}

#undef WIN_IT

}
