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
	if (fmt == YV12) {
		return new SDLOverlayVideoBuffer(r.Origin(), SDL_CreateYUVOverlay(r.w, r.h, SDL_YV12_OVERLAY, disp));
	} else {
		SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_HWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0 );
		SDL_Surface* buf = NULL;
		if (fmt == RGBA8888) {
			buf = SDL_DisplayFormatAlpha(tmp);
		} else {
			buf = SDL_DisplayFormat(tmp);
		}
		
		SDL_FreeSurface(tmp);
		return new SDLSurfaceVideoBuffer(buf, r.Origin());
	}
}

void SDL12VideoDriver::DrawPoint(const Point& p, const Color& color)
{
	if (!screenClip.PointInside(p)) {
		return;
	}

	std::vector<SDL_Point> points;
	points.push_back(p);
	SDLVideoDriver::SetSurfacePixels(CurrentRenderBuffer(), points, color);
}

void SDL12VideoDriver::DrawPoints(const std::vector<SDL_Point>& points, const SDL_Color& color)
{
	SDLVideoDriver::SetSurfacePixels(CurrentRenderBuffer(), points, reinterpret_cast<const Color&>(color));
}

void SDL12VideoDriver::DrawLines(const std::vector<SDL_Point>& points, const SDL_Color& color)
{
	std::vector<SDL_Point>::const_iterator it = points.begin();
	while (it != points.end()) {
		Point p1 = *it++;
		if (it == points.end()) {
			Log(WARNING, "SDL12Video", "DrawLines was passed an odd number of points. Last point discarded.");
			break;
		}
		Point p2 = *it++;
		DrawLine(p1, p2, reinterpret_cast<const Color&>(color));
	}
}

void SDL12VideoDriver::DrawHLine(short x1, short y, short x2, const Color& color)
{
	if (x1 > x2) {
		short tmpx = x1;
		x1 = x2;
		x2 = tmpx;
	}

	std::vector<SDL_Point> points;
	for (; x1 <= x2 ; x1++ )
		points.push_back(Point(x1, y));

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}

void SDL12VideoDriver::DrawVLine(short x, short y1, short y2, const Color& color)
{
	if (y1 > y2) {
		short tmpy = y1;
		y1 = y2;
		y2 = tmpy;
	}

	std::vector<SDL_Point> points;
	for (; y1 <= y2 ; y1++ )
		points.push_back(Point(x, y1));

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}

void SDL12VideoDriver::DrawLine(const Point& p, const Point& p2, const Color& color)
{
	Point p1 = p;
	bool yLonger = false;
	int shortLen = p2.y - p1.y;
	int longLen = p2.x - p1.x;
	if (abs( shortLen ) > abs( longLen )) {
		int swap = shortLen;
		shortLen = longLen;
		longLen = swap;
		yLonger = true;
	}
	int decInc;
	if (longLen == 0) {
		decInc = 0;
	} else {
		decInc = ( shortLen << 16 ) / longLen;
	}

	std::vector<SDL_Point> points;

	do { // TODO: rewrite without loop
		if (yLonger) {
			if (longLen > 0) {
				longLen += p1.y;
				for (int j = 0x8000 + ( p1.x << 16 ); p1.y <= longLen; ++p1.y) {
					points.push_back(Point( j >> 16, p1.y ));
					j += decInc;
				}
				break;
			}
			longLen += p1.y;
			for (int j = 0x8000 + ( p1.x << 16 ); p1.y >= longLen; --p1.y) {
				points.push_back(Point( j >> 16, p1.y ));
				j -= decInc;
			}
			break;
		}

		if (longLen > 0) {
			longLen += p1.x;
			for (int j = 0x8000 + ( p1.y << 16 ); p1.x <= longLen; ++p1.x) {
				points.push_back(Point( p1.x, j >> 16 ));
				j += decInc;
			}
			break;
		}
		longLen += p1.x;
		for (int j = 0x8000 + ( p1.y << 16 ); p1.x >= longLen; --p1.x) {
			points.push_back(Point( p1.x, j >> 16 ));
			j -= decInc;
		}
	} while (false);

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDL12VideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill)
{
	if (fill) {
		SDL_Surface* currentBuf = CurrentRenderBuffer();
		if ( SDL_ALPHA_TRANSPARENT == color.a ) {
			return;
		} else if ( SDL_ALPHA_OPAQUE == color.a || currentBuf->format->Amask) {

			Uint32 val = SDL_MapRGBA( currentBuf->format, color.r, color.g, color.b, color.a );
			SDL_Rect drect = RectFromRegion(ClippedDrawingRect(rgn));
			SDL_FillRect( currentBuf, &drect, val );
		} else {
			SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
			SDL_Color c = {color.r, color.g, color.b, color.a};
			SetSurfacePalette(rectsurf, &c, 1);
			SetSurfaceAlpha(rectsurf, color.a);
			BlitSurfaceClipped(rectsurf, Region(0, 0, rgn.w, rgn.h), rgn);
			SDL_FreeSurface( rectsurf );
		}
	} else {
		DrawHLine( rgn.x, rgn.y, rgn.x + rgn.w - 1, color );
		DrawVLine( rgn.x, rgn.y, rgn.y + rgn.h - 1, color );
		DrawHLine( rgn.x, rgn.y + rgn.h - 1, rgn.x + rgn.w - 1, color );
		DrawVLine( rgn.x + rgn.w - 1, rgn.y, rgn.y + rgn.h - 1, color );
	}
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
	bool flip = false;
	for (; it != buffers.end(); ++it) {
		flip = (*it)->RenderOnDisplay(disp) || flip;
	}
	
	if (flip) SDL_Flip( disp );
}

SDL12VideoDriver::vid_buf_t* SDL12VideoDriver::CurrentRenderBuffer()
{
	assert(drawingBuffer);
	return static_cast<SDLSurfaceVideoBuffer*>(drawingBuffer)->Surface();
}

Sprite2D* SDL12VideoDriver::GetScreenshot( Region r )
{
	unsigned int Width = r.w ? r.w : screenSize.w;
	unsigned int Height = r.h ? r.h : screenSize.h;

	void* pixels = malloc( Width * Height * 3 );
	SDLSurfaceSprite2D* screenshot = new SDLSurfaceSprite2D(Width, Height, 24, pixels,
															0x00ff0000, 0x0000ff00, 0x000000ff, 0);
	SDL_Surface* screenshotSurface = SDL_DisplayFormat(disp);
	SDL_Surface* tmp = disp;
	disp = screenshotSurface;
	Video::SwapBuffers(0);

	SDL_Rect src = RectFromRegion(r);
	SDL_BlitSurface( screenshotSurface, (r.w && r.h) ? &src : NULL, screenshot->GetSurface(), NULL);

	disp = tmp;
	SDL_FreeSurface(screenshotSurface);

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
		Event e;
		if (SDL_GetModState() & KMOD_SHIFT) {
			e = EventMgr::CreateMouseWheelEvent(Point(speed, 0));
		} else{
			e = EventMgr::CreateMouseWheelEvent(Point(0, speed));
		}
		EvntManager->DispatchEvent(e);
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
