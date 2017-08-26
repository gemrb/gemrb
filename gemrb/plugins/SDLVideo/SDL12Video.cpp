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

#include "SDL12Video.h"

#include "Game.h"
#include "Interface.h"
#include "SDLSurfaceSprite2D.h"

using namespace GemRB;

SDL12VideoDriver::SDL12VideoDriver(void)
{
	disp = NULL;
}

int SDL12VideoDriver::Init(void)
{
	int ret = SDLVideoDriver::Init();
	if (ret==GEM_OK) {
		SDL_EnableUNICODE( 1 );
		SDL_EnableKeyRepeat( 500, 50 );
#if TARGET_OS_MAC
		// Apple laptops have single buttons,
		// but actually produce more then left mouse events with that single button
		// this may limit people actually using very old single button mice, but who cares :)
		setenv("SDL_HAS3BUTTONMOUSE", "SDL_HAS3BUTTONMOUSE", 1);
#endif
	}
	return ret;
}

int SDL12VideoDriver::CreateDriverDisplay(const Size& s, int b, const char* title)
{
	bpp = b;
	screenSize = s;

	Log(MESSAGE, "SDL 1.2 Driver", "Creating display");
	ieDword flags = SDL_SWSURFACE;
	if (fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	Log(MESSAGE, "SDL 1.2 Driver", "SDL_SetVideoMode...");
	disp = SDL_SetVideoMode( s.w, s.h, bpp, flags );
	SDL_WM_SetCaption( title, 0 );
	if (disp == NULL) {
		Log(ERROR, "SDL 1.2 Driver", "%s", SDL_GetError());
		return GEM_ERROR;
	}
	Log(MESSAGE, "SDL 1.2 Driver", "Checking for HardWare Acceleration...");
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	if (!vi) {
		Log(WARNING, "SDL 1.2 Driver", "No Hardware Acceleration available.");
	}

	Log(MESSAGE, "SDL 1.2 Driver", "Creating Display Surface...");

	return GEM_OK;
}

VideoBuffer* SDL12VideoDriver::NewVideoBuffer(const Region& r, BufferFormat fmt)
{
	SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_HWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0 );
	SDL_Surface* buf = NULL;
	if (fmt == RGBA8888) {
		buf = SDL_DisplayFormatAlpha(tmp);
	} else {
		buf = SDL_DisplayFormat(tmp);
	}

	SDL_FreeSurface(tmp);
	if (fmt == YV12) {
		return new SDLOverlayVideoBuffer(buf, r.Origin(), SDL_CreateYUVOverlay(r.w, r.h, SDL_YV12_OVERLAY, disp));
	}
	return new SDLSurfaceVideoBuffer(buf, r.Origin());
}

// sets brightness and contrast
// FIXME:SetGammaRamp doesn't seem to work
// WARNING: SDL 1.2.13 crashes in SetGamma on Windows (it was fixed in SDL's #3533 Revision)
void SDL12VideoDriver::SetGamma(int brightness, int /*contrast*/)
{
	SDL_SetGamma(0.8+brightness/50.0,0.8+brightness/50.0,0.8+brightness/50.0);
}

bool SDL12VideoDriver::SetFullscreenMode(bool set)
{
	if (fullscreen != set) {
		fullscreen=set;
		// FIXME: SDL_WM_ToggleFullScreen only works on X11. use SDL_SetVideoMode()
		SDL_WM_ToggleFullScreen( disp );
		//synchronise internal variable
		core->GetDictionary()->SetAt( "Full Screen", (ieDword) fullscreen );
		return true;
	}
	return false;
}

void SDL12VideoDriver::SwapBuffers(VideoBuffers& buffers)
{
	VideoBuffers::iterator it;
	it = buffers.begin();
	for (; it != buffers.end(); ++it) {
		SDLSurfaceVideoBuffer* vb = static_cast<SDLSurfaceVideoBuffer*>(*it);
		vb->RenderOnDisplay(disp);
	}
	SDL_Flip( disp );
}

Sprite2D* SDL12VideoDriver::GetScreenshot( Region r )
{
	unsigned int Width = r.w ? r.w : screenSize.w;
	unsigned int Height = r.h ? r.h : screenSize.h;

	void* pixels = malloc( Width * Height * 3 );
	SDLSurfaceSprite2D* screenshot = new SDLSurfaceSprite2D(Width, Height, 24, pixels,
															0x00ff0000, 0x0000ff00, 0x000000ff);
	
	Video::SwapBuffers(0); // swap the buffer to update the display
	SDL_Rect src = RectFromRegion(r);
	SDL_BlitSurface( disp, (r.w && r.h) ? &src : NULL, screenshot->GetSurface(), NULL);

	return screenshot;
}

bool SDL12VideoDriver::ToggleGrabInput()
{
	if (SDL_GRAB_OFF == SDL_WM_GrabInput( SDL_GRAB_QUERY )) {
		SDL_WM_GrabInput( SDL_GRAB_ON );
		return true;
	}
	else {
		SDL_WM_GrabInput( SDL_GRAB_OFF );
		return false;
	}
}

int SDL12VideoDriver::ProcessEvent(const SDL_Event& event)
{
	if (event.type == SDL_ACTIVEEVENT) {
		if (event.active.state == SDL_APPMOUSEFOCUS) {
			// TODO: notify something (EventManager?) that we have lost focus
			// focus = event.active.gain;
		} else if (event.active.state == SDL_APPINPUTFOCUS) {
			// TODO: notify something (EventManager?) that we have lost focus
			// focus = event.active.gain;
		}
		return GEM_OK;
	}

	bool isMouseEvent = (SDL_EVENTMASK(event.type) & (SDL_MOUSEBUTTONDOWNMASK | SDL_MOUSEBUTTONUPMASK));
	int button = event.button.button;
	if (isMouseEvent && (button == SDL_BUTTON_WHEELUP || button == SDL_BUTTON_WHEELDOWN)) {
		// remap these to mousewheel events
		int speed = core->GetMouseScrollSpeed();
		speed *= (button == SDL_BUTTON_WHEELUP) ? 1 : -1;
		EventMgr::CreateMouseWheelEvent(Point(0, speed));

		return GEM_OK;
	}

	return SDLVideoDriver::ProcessEvent(event);
}

void SDL12VideoDriver::ShowSoftKeyboard()
{
	if(core->UseSoftKeyboard){
		Log(WARNING, "SDL 1.2 Driver", "SDL 1.2 doesn't support a software keyboard");
	}
}

void SDL12VideoDriver::HideSoftKeyboard()
{
	if(core->UseSoftKeyboard){
		Log(WARNING, "SDL 1.2 Driver", "SDL 1.2 doesn't support a software keyboard");
	}
}

bool SDL12VideoDriver::TouchInputEnabled()
{
    return core->MouseFeedback > 0;
}

// Private methods

bool SDL12VideoDriver::SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)
{
	return (SDL_SetAlpha( surface, SDL_SRCALPHA | SDL_RLEACCEL, alpha ) == 0);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL1 Video Driver")
PLUGIN_DRIVER(SDL12VideoDriver, "sdl")
END_PLUGIN()
