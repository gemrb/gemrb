/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2012 The GemRB Project
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

#include "SDL20Video.h"

#include "TileRenderer.inl"

#include "AnimationFactory.h"
#include "Audio.h"
#include "Game.h" // for GetGlobalTint
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Polygon.h"
#include "SpriteCover.h"
#include "GUI/Console.h"
#include "GUI/GameControl.h" // for TargetMode (contextual information for touch inputs)
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#include <cmath>
#include <cassert>
#include <cstdio>

#if TARGET_OS_IPHONE
extern "C" {
	#include "SDL_sysvideo.h"
	#include <SDL/uikit/SDL_uikitkeyboard.h>
}
#endif
#ifdef ANDROID
#include "SDL_screenkeyboard.h"
#endif

using namespace GemRB;

//touch gestures
#define MIN_GESTURE_DELTA_PIXELS 10
#define TOUCH_RC_NUM_TICKS 500

SDL20VideoDriver::SDL20VideoDriver(void)
	: rightMouseDownEvent(), rightMouseUpEvent()
{
	assert( core->NumFingScroll > 1 && core->NumFingKboard > 1 && core->NumFingInfo > 1);
	assert( core->NumFingScroll < 5 && core->NumFingKboard < 5 && core->NumFingInfo < 5);
	assert( core->NumFingScroll != core->NumFingKboard );

	renderer = NULL;
	window = NULL;
	videoPlayer = NULL;

	// touch input
	ignoreNextMouseUp = false;
	numFingers = 0;
	formationRotation = false;

	touchHold = false;
	touchHoldTime = 0;

	rightMouseDownEvent.type = SDL_MOUSEBUTTONDOWN;
	rightMouseDownEvent.button = SDL_BUTTON_RIGHT;
	rightMouseDownEvent.state = SDL_PRESSED;

	rightMouseUpEvent.type = SDL_MOUSEBUTTONUP;
	rightMouseUpEvent.button = SDL_BUTTON_RIGHT;
	rightMouseUpEvent.state = SDL_RELEASED;
	ignoreNextFingerUp = false;
	firstFingerDown = SDL_TouchFingerEvent();
}

SDL20VideoDriver::~SDL20VideoDriver(void)
{
	SDL_DestroyTexture(videoPlayer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

int SDL20VideoDriver::CreateDisplay(int w, int h, int b, bool fs, const char* title)
{
	bpp=b;
	fullscreen=fs;
	Log(MESSAGE, "SDL 2 Driver", "Creating display");
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
#if TARGET_OS_IPHONE || ANDROID
	// this allows the user to flip the device upsidedown if they wish and have the game rotate.
	// it also for some unknown reason is required for retina displays
	winFlags |= SDL_WINDOW_RESIZABLE;
	// this hint is set in the wrapper for iPad at a higher priority. set it here for iPhone
	// don't know if Android makes use of this.
	SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight LandscapeLeft", SDL_HINT_DEFAULT);
#endif
	if (fullscreen) {
		winFlags |= SDL_WINDOW_FULLSCREEN;
		//This is needed to remove the status bar on Android/iOS.
		//since we are in fullscreen this has no effect outside Android/iOS
		winFlags |= SDL_WINDOW_BORDERLESS;
	}
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, winFlags);
	renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create renderer:%s", SDL_GetError());
		return GEM_ERROR;
	}

	Viewport.x = Viewport.y = 0;
	width = window->w;
	height = window->h;
	Viewport.w = width;
	Viewport.h = height;

	Log(MESSAGE, "SDL 2 Driver", "Creating Main Surface...");
	SDL_Surface* tmp = SDL_CreateRGBSurface( 0, width, height,
											bpp, 0, 0, 0, 0 );

	backBuf = SDL_ConvertSurfaceFormat(tmp, SDL_GetWindowPixelFormat(window), 0);
	disp = backBuf;

	SDL_FreeSurface( tmp );
	return GEM_OK;
}

void SDL20VideoDriver::InitMovieScreen(int &w, int &h, bool yuv)
{
	w = window->w;
	h = window->h;

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	if (videoPlayer) SDL_DestroyTexture(videoPlayer);
	if (yuv) {
		videoPlayer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, w, h);
	} else {
		videoPlayer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	}

	//setting the subtitle region to the bottom 1/4th of the screen
	subtitleregion.w = w;
	subtitleregion.h = h/4;
	subtitleregion.x = 0;
	subtitleregion.y = h-h/4;
	//same for SDL
	subtitleregion_sdl.w = w;
	subtitleregion_sdl.h = h/4;
	subtitleregion_sdl.x = 0;
	subtitleregion_sdl.y = h-h/4;
}

void SDL20VideoDriver::showFrame(unsigned char* buf, unsigned int bufw,
							   unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
							   unsigned int h, unsigned int dstx, unsigned int dsty,
							   int g_truecolor, unsigned char *pal, ieDword titleref)
{
	assert( bufw == w && bufh == h );

	SDL_Rect srcRect = {sx, sy, w, h};
	SDL_Rect destRect = {dstx, dsty, w, h};
	// temporary hack. for some reason our view port is centered even when our "window" is not.
	// on iOS it is currently impossible to not anchor the view to the top left of the display (SDL bug)
	destRect.x = 0;
	destRect.y = 0 ;

	Uint8 *src;
	Uint32 *dst;
	unsigned int row, col;
	void *pixels;
	int pitch;
	SDL_Color* color;

	SDL_LockTexture(videoPlayer, &destRect, &pixels, &pitch);
	src = buf;
	if (g_truecolor) {
		for (row = 0; row < bufh; ++row) {
			dst = (Uint32*)((Uint16*)pixels + row * pitch);
			for (col = 0; col < bufw; ++col) {
				color->r = ((*src & 0xF8) << 3) | ((*src & 0xF8) >> 2);
				color->g = ((*src & 0x7E0) << 2) | ((*src & 0x7E0) >> 4);
				color->b = ((*src & 0x1F) << 3) | ((*src & 0x1F) >> 2);
				color->unused = 0;
				// video player texture is of ARGB format. buf is RGB565
				*dst++ = (0xFF000000|(color->r << 16)|(color->g << 8)|(color->b));
				src++;
			}
		}
	} else {
		SDL_Palette* palette;
		palette = SDL_AllocPalette(256);
		for (int i = 0; i < 256; i++) {
			palette->colors[i].r = ( *pal++ ) << 2;
			palette->colors[i].g = ( *pal++ ) << 2;
			palette->colors[i].b = ( *pal++ ) << 2;
			palette->colors[i].unused = 0;
		}
		for (row = 0; row < bufh; ++row) {
			dst = (Uint32*)((Uint8*)pixels + row * pitch);
			for (col = 0; col < bufw; ++col) {
				color = &palette->colors[*src++];
				// video player texture is of ARGB format
				*dst++ = (0xFF000000|(color->r << 16)|(color->g << 8)|(color->b));
			}
		}
		SDL_FreePalette(palette);
	}
	SDL_UnlockTexture(videoPlayer);

	SDL_RenderFillRect(renderer, &subtitleregion_sdl);
	SDL_RenderCopy(renderer, videoPlayer, &srcRect, &destRect);

	if (titleref>0)
		DrawMovieSubtitle( titleref );

	SDL_RenderPresent(renderer);
}

void SDL20VideoDriver::showYUVFrame(unsigned char** buf, unsigned int */*strides*/,
				  unsigned int bufw, unsigned int bufh,
				  unsigned int w, unsigned int h,
				  unsigned int dstx, unsigned int dsty,
				  ieDword titleref)
{
	showFrame(*buf, bufw, bufh, 0, 0, w, h, dstx, dsty, true, NULL, titleref);
}

int SDL20VideoDriver::SwapBuffers(void)
{
	int ret = SDLVideoDriver::SwapBuffers();

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, backBuf);

	if (fadeColor.a) {
		SDL_Rect dst = {
			Viewport.x, Viewport.y, Viewport.w, Viewport.h
		};
		SDL_SetRenderDrawColor(renderer, fadeColor.r, fadeColor.g, fadeColor.b, fadeColor.a);
		SDL_RenderFillRect(renderer, &dst);
	}

	SDL_RenderCopy(renderer, tex, NULL, NULL);

	SDL_RenderPresent( renderer );
	SDL_DestroyTexture(tex);
	return ret;
}

int SDL20VideoDriver::PollEvents()
{
	if (touchHold && (SDL_GetTicks() - touchHoldTime) >= TOUCH_RC_NUM_TICKS) {
		SDL_Event evtDown = SDL_Event();

		evtDown.type = SDL_MOUSEBUTTONDOWN;
		evtDown.button = rightMouseDownEvent;

		SDL_PushEvent(&evtDown);

		GameControl* gc = core->GetGameControl();
		if (EvntManager->GetMouseFocusedControlType() == IE_GUI_GAMECONTROL && gc && gc->GetTargetMode() == TARGET_MODE_NONE) {
			// formation rotation
			touchHold = false;
			formationRotation = true;
		} else {
			SDL_Event evtUp = SDL_Event();
			evtUp.type = SDL_MOUSEBUTTONUP;
			evtUp.button = rightMouseUpEvent;
			SDL_PushEvent(&evtUp);
		}
		ignoreNextMouseUp = true;
		touchHoldTime = 0;
	}
	if (formationRotation) {
		ignoreNextMouseUp = true;
	}
	return SDLVideoDriver::PollEvents();
}

void SDL20VideoDriver::ProcessFirstTouch( int mouseButton )
{
	if (firstFingerDown.fingerId) {
		// no need to scale these coordinates. they were scaled previously for us.
		EvntManager->MouseDown( firstFingerDown.x, firstFingerDown.y,
								mouseButton, GetModState(SDL_GetModState()) );
		firstFingerDown = SDL_TouchFingerEvent();
		ignoreNextFingerUp = false;
	}
}

int SDL20VideoDriver::ProcessEvent(const SDL_Event & event)
{
	/*
	 the digitizer could have a higher resolution then the screen.
	 we need to get the scale factor to convert digitizer touch coordinates to screen pixel coordinates
	 */
	SDL_Touch *state = SDL_GetTouch(event.tfinger.touchId);
	float xScaleFactor = 1.0;
	float yScaleFactor = 1.0;
	if(state){
		xScaleFactor = state->xres / window->w;
		yScaleFactor = state->yres / window->h;
	}

	touchHoldTime = 0;

	bool ConsolePopped = core->ConsolePopped;

	switch (event.type) {
		//!!!!!!!!!!!!
		// !!!: currently SDL brodcasts both mouse and touch events on touch
		//  there is no API to prevent the mouse events so I have hacked the mouse events.
		//  watch future SDL 1.3 releases to see if/when disabling mouse events from the touchscreen is available
		//!!!!!!!!!!!!
		case SDL_MOUSEMOTION:
			if (numFingers > 1) break;
			MouseMovement(event.motion.x, event.motion.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if ((MouseFlags & MOUSE_DISABLED) || !EvntManager)
				break;
			ignoreNextMouseUp = false;
			lastMouseDownTime = EvntManager->GetRKDelay();
			if (lastMouseDownTime != (unsigned long) ~0) {
				lastMouseDownTime += lastMouseDownTime + lastTime;
			}
			if (CursorIndex != VID_CUR_DRAG)
				CursorIndex = VID_CUR_DOWN;
			CursorPos.x = event.button.x; // - mouseAdjustX[CursorIndex];
			CursorPos.y = event.button.y; // - mouseAdjustY[CursorIndex];
			if (!ConsolePopped)
				EvntManager->MouseDown( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );
			break;
		case SDL_MOUSEBUTTONUP:
			if ((MouseFlags & MOUSE_DISABLED) || !EvntManager || ignoreNextMouseUp)
				break;
			ignoreNextMouseUp = true;
			if (CursorIndex != VID_CUR_DRAG)
				CursorIndex = VID_CUR_UP;
			CursorPos.x = event.button.x;
			CursorPos.y = event.button.y;
			if (!ConsolePopped)
				EvntManager->MouseUp( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );
			break;
		case SDL_MOUSEWHEEL:
			/*
			 TODO: need a preference for inverting these
			 */
			short scrollX;
			scrollX= event.wheel.x * -1;
			short scrollY;
			scrollY= event.wheel.y * -1;
			EvntManager->MouseWheelScroll( scrollX, scrollY );
			break;
		case SDL_FINGERMOTION://SDL 1.3+
			//For swipes. gestures needing pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
			touchHold = false;
			if (EvntManager) {
				if (numFingers == core->NumFingScroll || (numFingers != core->NumFingKboard && EvntManager->GetMouseFocusedControlType() == IE_GUI_TEXTAREA)) {
					//any # of fingers != NumFingKBoard will scroll a text area
					if (EvntManager->GetMouseFocusedControlType() != IE_GUI_TEXTAREA) {
						// if focus is IE_GUI_TEXTAREA we need mouseup to clear scrollbar flags so this scrolling doesnt break after interactind with the slider
						ignoreNextMouseUp = true;
					}else {
						// if we are scrolling a text area we dont want the keyboard in the way
						HideSoftKeyboard();
					}
					//invert the coordinates such that dragging down scrolls up etc.
					int scrollX = (event.tfinger.dx / xScaleFactor) * -1;
					int scrollY = (event.tfinger.dy / yScaleFactor) * -1;

					EvntManager->MouseWheelScroll( scrollX, scrollY );
				} else if (numFingers == core->NumFingKboard) {
					if ((event.tfinger.dy / yScaleFactor) * -1 >= MIN_GESTURE_DELTA_PIXELS){
						// if the keyboard is already up interpret this gesture as console pop
						if(softKeyboardShowing && !ConsolePopped && !ignoreNextMouseUp) core->PopupConsole();
						else ShowSoftKeyboard();
					} else if((event.tfinger.dy / yScaleFactor) * -1 <= -MIN_GESTURE_DELTA_PIXELS){
						HideSoftKeyboard();
					}
					ignoreNextMouseUp = true;
				}
			}
			break;
		case SDL_FINGERDOWN://SDL 1.3+
			touchHold = false;
			if (++numFingers == 1) {
				rightMouseDownEvent.x = event.tfinger.x / xScaleFactor;
				rightMouseDownEvent.y = event.tfinger.y / yScaleFactor;
				rightMouseUpEvent.x = event.tfinger.x / xScaleFactor;
				rightMouseUpEvent.y = event.tfinger.y / yScaleFactor;

				touchHoldTime = SDL_GetTicks();
				touchHold = true;
			} else if (EvntManager && numFingers == core->NumFingInfo) {
				EvntManager->OnSpecialKeyPress( GEM_TAB );
				EvntManager->OnSpecialKeyPress( GEM_ALT );
			}
			break;
		case SDL_FINGERUP://SDL 1.3+
			touchHold = false;//even if there are still fingers in contact
			if (numFingers) numFingers--;
			if (formationRotation) {
				EvntManager->MouseUp( event.tfinger.x, event.tfinger.y, GEM_MB_MENU, GetModState(SDL_GetModState()) );
				formationRotation = false;
				ignoreNextMouseUp = false;
			}
			if (EvntManager && numFingers != core->NumFingInfo) {
				EvntManager->KeyRelease( GEM_ALT, 0 );
			}
			break;
			//multitouch gestures
		case SDL_MULTIGESTURE://SDL 1.3+
			// use this for pinch or rotate gestures. see also SDL_DOLLARGESTURE
			numFingers = event.mgesture.numFingers;
			/*
			 // perhaps formation rotation should be implemented here as a rotate gesture.
			 if (Evnt->GetMouseFocusedControlType() == IE_GUI_GAMECONTROL && numFingers == 2) {
			 }
			 */
			break;
		/* not user input event */
		case SDL_TEXTINPUT:
			for (size_t i=0; i < strlen(event.text.text); i++) {
				if (core->ConsolePopped)
					core->console->OnKeyPress( event.text.text[i], GetModState(event.key.keysym.mod));
				else
					EvntManager->KeyPress( event.text.text[i], GetModState(event.key.keysym.mod));
			}
			break;
		/* not user input events */
		case SDL_WINDOWEVENT://SDL 1.2
			switch (event.window.event) {
				case SDL_WINDOWEVENT_MINIMIZED://SDL 1.3
					// We pause the game and audio when the window is minimized.
					// on iOS/Android this happens when leaving the application or when play is interrupted (ex phone call)
					// if win/mac/linux has a problem with this behavior we can work something out.
					core->GetAudioDrv()->Pause();//this is for ANDROID mostly
					core->SetPause(PAUSE_ON);
					break;
				case SDL_WINDOWEVENT_RESTORED: //SDL 1.3
					/*
					 reset all static variables as if no events have happened yet
					 restoring from "minimized state" should be a clean slate.
					 */
					numFingers = 0;
					touchHoldTime = 0;
					touchHold = false;
					ignoreNextMouseUp = false;
#if TARGET_OS_IPHONE
					// FIXME: this is essentially a hack.
					// I believe there to be a bug in SDL 1.3 that is causeing the surface to be invalidated on a restore event for iOS
					SDL_Window* window;
					window = SDL_GetFocusWindow();
					window->surface_valid = SDL_TRUE;//private attribute!!!
					// FIXME:
					// sleep for a short while to avoid some unknown Apple threading issue with OpenAL threads being suspended
					// even using Apple examples of how to properly suspend an OpenAL context and resume on iOS are falling flat
					// it could be this bug affects only the simulator.
					sleep(1);
#endif
					core->GetAudioDrv()->Resume();//this is for ANDROID mostly
					break;
					/*
				case SDL_WINDOWEVENT_RESIZED: //SDL 1.2
					// this event exists in SDL 1.2, but this handler is only getting compiled under 1.3+
					Log(WARNING, "SDL 2 Driver",  "Window resized so your window surface is now invalid.");
					break;
					 */
			}
			break;
		default:
			return SDLVideoDriver::ProcessEvent(event);
	}
	return GEM_OK;
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::HideSoftKeyboard()
{
	if(core->UseSoftKeyboard){
#if TARGET_OS_IPHONE
		SDL_iPhoneKeyboardHide(window);
#endif
#ifdef ANDROID
		SDL_ANDROID_SetScreenKeyboardShown(0);
#endif
		softKeyboardShowing = false;
		if(core->ConsolePopped) core->PopupConsole();
	}
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::ShowSoftKeyboard()
{
	if(core->UseSoftKeyboard){
#if TARGET_OS_IPHONE
		SDL_iPhoneKeyboardShow(SDL_GetFocusWindow());
#endif
#ifdef ANDROID
		SDL_ANDROID_SetScreenKeyboardShown(1);
#endif
		softKeyboardShowing = true;
	}
}

/* no idea how elaborate this should be*/
void SDL20VideoDriver::MoveMouse(unsigned int x, unsigned int y)
{
	SDL_WarpMouseInWindow(window, x, y);
}

void SDL20VideoDriver::SetGamma(int /*brightness*/, int /*contrast*/)
{

}

bool SDL20VideoDriver::SetFullscreenMode(bool set)
{
	return (SDL_SetWindowFullscreen(window, (SDL_bool)set) == 0);
}

bool SDL20VideoDriver::ToggleGrabInput()
{
	bool isGrabbed = SDL_GetWindowGrab(window);
	SDL_SetWindowGrab(window, (SDL_bool)!isGrabbed);
	return (isGrabbed != SDL_GetWindowGrab(window));
}

// Private methods

bool SDL20VideoDriver::SetSurfacePalette(SDL_Surface* surface, SDL_Color* colors, int ncolors)
{
	return (SDL_SetPaletteColors((surface)->format->palette, colors, 0, ncolors) == 0);
}

bool SDL20VideoDriver::SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)
{
	return (SDL_SetSurfaceAlphaMod(surface, alpha) == 0);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()
