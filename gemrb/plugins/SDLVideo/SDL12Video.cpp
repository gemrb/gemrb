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

#include "SpriteRenderer.inl"

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

void SDL12VideoDriver::BlitSpriteBAMClipped(const Sprite2D* spr, const Sprite2D* mask,
											const Region& src, const Region& dst,
											unsigned int flags, const Color* t)
{
	Color tint(255,255,255,255);
	if (t) {
		tint = *t;
	}

	assert(spr->BAM);
	Palette* palette = spr->GetPalette();

	// global tint is handled by the callers

	// implicit flags:
	const unsigned int blit_TINTALPHA =    0x40000000U;
	const unsigned int blit_PALETTEALPHA = 0x80000000U;

	// NB: blit_TINTALPHA isn't directly used or checked, but its presence
	// affects the special case checks below
	if ((flags & BLIT_TINTED) && tint.a != 255) flags |= blit_TINTALPHA;

	if (palette->alpha) flags |= blit_PALETTEALPHA;

	// flag combinations which are often used:
	// (ignoring MIRRORX/Y since those are always resp. never handled by templ.)

	// most game sprites:
	// covered, BLIT_TINTED
	// covered, BLIT_TINTED | BLIT_TRANSSHADOW
	// covered, BLIT_TINTED | BLIT_NOSHADOW

	// area-animations?
	// BLIT_TINTED

	// (hopefully) most video overlays:
	// BLIT_HALFTRANS
	// covered, BLIT_HALFTRANS
	// covered
	// none

	// other combinations use general case


	// FIXME: our BAM blitters dont let us start at an arbitrary point in the source
	// We will compensate by tricking them by manipulating the location and size of the blit
	// then using dst as a clipping rect to achieve the effect of a partial src copy

	int x = dst.x - src.x;
	int y = dst.y - src.y;
	int w = spr->Width;
	int h = spr->Height;

	//int tx = dst.x - spr->XPos;
	//int ty = dst.y - spr->YPos;

	const Uint8* srcdata = (const Uint8*)spr->LockSprite();
	SDL_Surface* currentBuf = CurrentRenderBuffer();
	SDL_LockSurface(currentBuf);

	bool hflip = bool(spr->renderFlags&BLIT_MIRRORX);
	bool vflip = bool(spr->renderFlags&BLIT_MIRRORY);

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags & ~(BLIT_MIRRORX | BLIT_MIRRORY);
	if (remflags & BLIT_NOSHADOW) remflags &= ~BLIT_TRANSSHADOW;
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;

	// TODO: we technically only need SRBlender_Alpha when there is a mask. Could boost performance noticably to account for that.

	if (remflags == BLIT_TINTED) {

		SRShadow_Regular shadow;
		SRTinter_Tint<false, false> tinter(tint);
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(mask, hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);

	} else if (remflags == (BLIT_TINTED | BLIT_TRANSSHADOW)) {

		SRShadow_HalfTrans shadow(currentBuf->format, palette->col[1]);
		SRTinter_Tint<false, false> tinter(tint);
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(mask, hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);

	} else if (remflags == (BLIT_TINTED | BLIT_NOSHADOW)) {

		SRShadow_None shadow;
		SRTinter_Tint<false, false> tinter(tint);
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(mask, hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);

	} else if (remflags == BLIT_HALFTRANS) {

		SRShadow_HalfTrans shadow(currentBuf->format, palette->col[1]);
		SRTinter_NoTint<false> tinter;
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(mask, hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);

	} else if (remflags == 0) {

		SRShadow_Regular shadow;
		SRTinter_NoTint<false> tinter;
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(mask, hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);

	} else {
		// handling the following effects with conditionals:
		// halftrans
		// noshadow
		// transshadow
		// grey (TODO)
		// sepia (TODO)
		// glow (not yet)
		// blended (not yet)
		// vflip

		// handling the following effects by repeated calls:
		// palettealpha
		// tinted
		// covered
		// hflip

		if (!(remflags & BLIT_TINTED)) tint.a = 255;

		SRShadow_Flags shadow; // for halftrans, noshadow, transshadow
		SRBlender_Alpha blender;
		if (remflags & blit_PALETTEALPHA) {
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<true> tinter(tint);

				BlitSpritePAL_dispatch(mask, hflip,
									   currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<true> tinter;

				BlitSpritePAL_dispatch(mask, hflip,
									   currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);
			}
		} else {
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<false> tinter(tint);

				BlitSpritePAL_dispatch(mask, hflip,
									   currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<false> tinter;

				BlitSpritePAL_dispatch(mask, hflip,
									   currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, spr, remflags, shadow, tinter, blender);
			}

		}
	}

	spr->UnlockSprite();
	palette->release();
	SDL_UnlockSurface(currentBuf);
}

void SDL12VideoDriver::BlitSurfaceClipped(SDL_Surface* surf, SDL_Rect& srect, SDL_Rect& drect)
{
	// since we should already be clipped we can call SDL_LowerBlit directly
	SDL_LowerBlit(surf, &srect, CurrentRenderBuffer(), &drect);
}

void SDL12VideoDriver::BlitSpriteNativeClipped(const Sprite2D* spr, const Sprite2D* mask,
											   const SDL_Rect& srect, const SDL_Rect& drect,
											   unsigned int flags, const SDL_Color* tint)
{
	// non-BAM Blitting

	// handling the following effects with conditionals:
	// halftrans
	// grey
	// sepia
	// glow (not yet)
	// blended (not yet)
	// yflip

	// handling the following effects by repeated inclusion:
	// tinted
	// covered
	// xflip

	// not handling the following effects at all:
	// noshadow
	// transshadow
	// palettealpha

	//		print("Unoptimized blit: %04X", flags);

	Color c;
	if (tint){
		c = Color(tint->r, tint->g, tint->b, tint->unused);
	}

	bool hflip = bool(flags&BLIT_MIRRORX);
	bool vflip = bool(flags&BLIT_MIRRORY);

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags & ~(BLIT_MIRRORX | BLIT_MIRRORY);
	if (remflags & BLIT_NOSHADOW) remflags &= ~BLIT_TRANSSHADOW;
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;

	if (remflags & BLIT_HALFTRANS) {
		// handle halftrans with 50% alpha tinting
		if (!(remflags & BLIT_TINTED)) {
			c.r = c.g = c.b = c.a = 255;
			remflags |= BLIT_TINTED;
		}
		c.a >>= 1;
	}

	const SDL_Surface* surf = ((SDLSurfaceSprite2D*)spr)->GetSurface();
	SDL_Surface* currentBuf = CurrentRenderBuffer();
	const Region finalclip = Region(drect.x, drect.y, drect.w, drect.h);

	int x = drect.x - srect.x;
	int y = drect.y - srect.y;
	int w = spr->Width;
	int h = spr->Height;

	if (surf->format->BytesPerPixel == 1) {
		const Color* pal = spr->GetPaletteColors();

		if (remflags & BLIT_TINTED)
			c.a = 255;

		SRBlender_Alpha blender;
		SRShadow_NOP shadow;
		if (remflags & BLIT_TINTED) {
			const Uint8 *data = (const Uint8*)spr->LockSprite();
			SRTinter_Flags<false> tinter(c);

			BlitSpritePAL_dispatch(mask, hflip,
								   currentBuf, data, pal, x, y, w, h, vflip, finalclip, -1, mask, spr, remflags, shadow, tinter, blender);
			spr->UnlockSprite();
		} else {
			// no blending/tinting
			SDL_Surface* surf = ((SDLSurfaceSprite2D*)spr)->GetSurface();
			SDL_Rect s = srect;
			SDL_Rect d = drect;
			BlitSurfaceClipped(surf, s, d);
		}

	} else {
		SRBlender_Alpha blender;
		if (remflags & BLIT_TINTED) {
			const Uint32 *data = (const Uint32*)spr->LockSprite();
			SRTinter_Flags<true> tinter(c);

			BlitSpriteRGB_dispatch(mask, hflip,
								   currentBuf, data, x, y, w, h, vflip, finalclip, mask, spr, remflags, tinter, blender);
			spr->UnlockSprite();
		} else {
			// no blending/tinting
			SDL_Surface* surf = ((SDLSurfaceSprite2D*)spr)->GetSurface();
			SDL_Rect s = srect;
			SDL_Rect d = drect;
			BlitSurfaceClipped(surf, s, d);
		}

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

	short min = screenClip.x;
	short max = min + screenClip.w;
	x1 = Clamp(x1, min, max);
	x2 = Clamp(x2, min, max);

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

	short min = screenClip.y;
	short max = min + screenClip.h;
	y1 = Clamp(y1, min, max);
	y2 = Clamp(y2, min, max);

	std::vector<SDL_Point> points;
	for (; y1 <= y2 ; y1++ )
		points.push_back(Point(x, y1));

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}

void SDL12VideoDriver::DrawLine(const Point& start, const Point& end, const Color& color)
{
	if (start.y == end.y) return DrawHLine(start.x, start.y, end.x, color);
	if (start.x == end.x) return DrawVLine(start.x, start.y, end.y, color);

	// clamp the points to screenClip
	Point min = screenClip.Origin();
	Point max = min + Point(screenClip.w, screenClip.h);
	Point p1 = Clamp(start, min, max);
	Point p2 = Clamp(end, min, max);
	
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

			assert(rgn.w > 0 && rgn.h > 0);
			SDL_Rect srect = {0, 0, (unsigned short)rgn.w, (unsigned short)rgn.h};
			SDL_Rect drect = RectFromRegion(ClippedDrawingRect(rgn));
			BlitSurfaceClipped(rectsurf, srect, drect);
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
		Uint32 flags = disp->flags;
		flags ^= SDL_FULLSCREEN;
		disp = SDL_SetVideoMode(disp->w, disp->h, disp->format->BitsPerPixel, flags);

		fullscreen=set;
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

	SDLSurfaceSprite2D* screenshot = new SDLSurfaceSprite2D(Width, Height, 24,
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
