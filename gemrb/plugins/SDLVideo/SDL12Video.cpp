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
#include "Pixels.h"
#include "SDLSurfaceSprite2D.h"

#include "SpriteRenderer.inl"

using namespace GemRB;

SDL12VideoDriver::SDL12VideoDriver(void)
{
	disp = NULL;
	inTextInput = false;
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
		SDL_Surface* buf = NULL;
		if (fmt == RGB555) {
			buf = SDL_CreateRGBSurface(0, r.w, r.h, 16, 0x7C00, 0x03E0, 0x001F, 0);
		} else if (fmt == RGBA8888) {
			SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_HWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0 );
			buf = SDL_DisplayFormatAlpha(tmp);
			SDL_FreeSurface(tmp);
		} else if (fmt == DISPLAY_ALPHA) {
			SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_HWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0 );
			buf = SDL_DisplayFormatAlpha(tmp);
			SDL_FreeSurface(tmp);
		} else {
			SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_HWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0 );
			buf = SDL_DisplayFormat(tmp);
			SDL_FreeSurface(tmp);
		}

		return new SDLSurfaceVideoBuffer(buf, r.Origin());
	}
}

void SDL12VideoDriver::BlitSpriteBAMClipped(const Sprite2D* spr, const Region& src, const Region& dst,
											uint32_t flags, const Color* t)
{
	Color tint(255,255,255,255);
	if (t) {
		tint = *t;
	}

	assert(spr->BAM);
	Palette* palette = spr->GetPalette();

	// global tint is handled by the callers

	// flag combinations which are often used:
	// (ignoring MIRRORX/Y since those are always resp. never handled by templ.)

	// most game sprites:
	// covered, BLIT_TINTED

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
	int w = spr->Frame.w;
	int h = spr->Frame.h;

	//int tx = dst.x - spr->Frame.x;
	//int ty = dst.y - spr->Frame.y;

	const Uint8* srcdata = (const Uint8*)spr->LockSprite();
	SDL_Surface* currentBuf = CurrentRenderBuffer();
	SDL_LockSurface(currentBuf);

	bool hflip = bool(spr->renderFlags&BLIT_MIRRORX);
	bool vflip = bool(spr->renderFlags&BLIT_MIRRORY);

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags & ~(BLIT_MIRRORX | BLIT_MIRRORY | BLIT_BLENDED);
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;

	SDL_Surface* mask = nullptr;
	if (remflags&BLIT_STENCIL_MASK) {
		mask = CurrentStencilBuffer();
	}

	// TODO: we technically only need SRBlender_Alpha when there is a mask. Could boost performance noticably to account for that.

	if (remflags == BLIT_TINTED && tint.a == 255) {
		SRTinter_Tint<true, true> tinter(tint);
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);

	} else if (remflags == BLIT_HALFTRANS) {
		SRTinter_NoTint<false> tinter;
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);

	} else if (remflags == 0) {
		SRTinter_NoTint<false> tinter;
		SRBlender_Alpha blender;

		BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);

	} else {
		// handling the following effects with conditionals:
		// halftrans
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

		SRBlender_Alpha blender;
		if (palette->alpha) {
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<true> tinter(tint);

				BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<true> tinter;

				BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);
			}
		} else {
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<false> tinter(tint);

				BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<false> tinter;

				BlitSpritePAL_dispatch(hflip, currentBuf, srcdata, palette->col, x, y, w, h, vflip, dst, (Uint8)spr->GetColorKey(), mask, remflags, tinter, blender);
			}

		}
	}

	spr->UnlockSprite();
	palette->release();
	SDL_UnlockSurface(currentBuf);
}

void SDL12VideoDriver::BlitSpriteNativeClipped(const sprite_t* spr, const SDL_Rect& srect, const SDL_Rect& drect, uint32_t flags, const SDL_Color* tint)
{
	const SDLSurfaceSprite2D* sdlspr = static_cast<const SDLSurfaceSprite2D*>(spr);
	SDL_Surface* surf = sdlspr->GetSurface();

	Color c;
	if (tint && (flags&BLIT_TINTED)){
		c = Color(tint->r, tint->g, tint->b, tint->unused);
	}

	if (surf->format->BytesPerPixel == 1) {
		c.a = SDL_ALPHA_OPAQUE; // FIXME: this is probably actually contigent on something else...

		const unsigned int shaderflags = (BLIT_TINTED|BLIT_GREY|BLIT_SEPIA);
		uint32_t version = flags&shaderflags;
		if (flags&BLIT_TINTED) {
			RenderSpriteVersion(sdlspr, version, &c);
		} else {
			RenderSpriteVersion(sdlspr, version);
		}
		
		// since the "shading" has been done we clear the flags
		flags &= ~shaderflags;
	}

	BlitSpriteNativeClipped(surf, srect, drect, flags, c);
}

void SDL12VideoDriver::BlitSpriteNativeClipped(SDL_Surface* surf, const SDL_Rect& srect, const SDL_Rect& drect, uint32_t flags, Color tint)
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
	// palettealpha

	//		print("Unoptimized blit: %04X", flags);

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags;
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;

	SDL_Surface* currentBuf = CurrentRenderBuffer();

	SDL_Surface* stencilsurf = nullptr;
	if (remflags&BLIT_STENCIL_MASK) {
		stencilsurf = CurrentStencilBuffer();
	}

	bool halftrans = remflags & BLIT_HALFTRANS;
	if (halftrans && (remflags ^ BLIT_HALFTRANS)) { // other flags are set too
		// handle halftrans with 50% alpha tinting
		if (!(remflags & BLIT_TINTED)) {
			tint.r = tint.g = tint.b = tint.a = 255;
			remflags |= BLIT_TINTED;
		}
		tint.a >>= 1;
	}

	// FIXME: this always assumes BLIT_BLENDED if any "shader" flags are set
	// we don't currently have a need for non blended sprites (we do for primitives, which is handled elsewhere)
	// however, it could make things faster if we handled it

	if (remflags&BLIT_TINTED) {
		if (remflags&BLIT_GREY) {
			RGBBlendingPipeline<GREYSCALE, true> blender(tint);
			BlitBlendedRect(surf, currentBuf, srect, drect, blender, remflags, stencilsurf);
		} else if (remflags&BLIT_SEPIA) {
			RGBBlendingPipeline<SEPIA, true> blender(tint);
			BlitBlendedRect(surf, currentBuf, srect, drect, blender, remflags, stencilsurf);
		} else {
			RGBBlendingPipeline<TINT, true> blender(tint);
			BlitBlendedRect(surf, currentBuf, srect, drect, blender, remflags, stencilsurf);
		}
	} else if (remflags&BLIT_GREY) {
		RGBBlendingPipeline<GREYSCALE, true> blender;
		BlitBlendedRect(surf, currentBuf, srect, drect, blender, remflags, stencilsurf);
	} else if (remflags&BLIT_SEPIA) {
		RGBBlendingPipeline<SEPIA, true> blender;
		BlitBlendedRect(surf, currentBuf, srect, drect, blender, remflags, stencilsurf);
	} else if (stencilsurf || (remflags&(BLIT_MIRRORX|BLIT_MIRRORY)) || ((surf->flags & SDL_SRCCOLORKEY) == 0 && remflags&BLIT_BLENDED)) {
		RGBBlendingPipeline<NONE, true> blender;
		BlitBlendedRect(surf, currentBuf, srect, drect, blender, remflags, stencilsurf);
	} else {
		// must be checked afer palette versioning is done
		
		// the gamewin is an RGB surface (no alpha)
		// RGBA->RGB with SDL_SRCALPHA
		//     The source is alpha-blended with the destination, using the alpha channel. SDL_SRCCOLORKEY and the per-surface alpha are ignored.
		// RGBA->RGB without SDL_SRCALPHA
		//     The RGB data is copied from the source. The source alpha channel and the per-surface alpha value are ignored.
		if (halftrans) {
			// we can only use SDL_SetAlpha if we dont need our general purpose blitter for another reason
			SDL_SetAlpha(surf, SDL_SRCALPHA, 128);
		} else if (remflags&BLIT_BLENDED) {
			SDL_SetAlpha(surf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
		} else {
			SDL_SetAlpha(surf, 0, SDL_ALPHA_OPAQUE);
		}

		SDL_Rect s = srect;
		SDL_Rect d = drect;
		SDL_LowerBlit(surf, &s, currentBuf, &d);
	}
}

void SDL12VideoDriver::BlitVideoBuffer( const VideoBufferPtr& buf, const Point& p, uint32_t flags, const Color* tint, const Region* clip)
{
	auto surface = static_cast<SDLSurfaceVideoBuffer&>(*buf).Surface();
	const Region& r = buf->Rect();
	Point origin = r.Origin() + p;

	Color c;
	if (tint) {
		c = *tint;
	}

	if (clip) {
		SDL_Rect drect = {origin.x, origin.y, Uint16(clip->w), Uint16(clip->h)};
		BlitSpriteNativeClipped(surface, RectFromRegion(*clip), drect, flags, c);
	} else {
		SDL_Rect srect = {0, 0, Uint16(r.w), Uint16(r.h)};
		SDL_Rect drect = {origin.x, origin.y, Uint16(r.w), Uint16(r.h)};
		BlitSpriteNativeClipped(surface, srect, drect, flags, c);
	}
}

void SDL12VideoDriver::DrawPointImp(const Point& p, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawPointSurface<true>(CurrentRenderBuffer(), p, screenClip, color);
	} else {
		DrawPointSurface<false>(CurrentRenderBuffer(), p, screenClip, color);
	}
}

void SDL12VideoDriver::DrawPointsImp(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawPointsSurface<true>(CurrentRenderBuffer(), points, screenClip, color);
	} else {
		DrawPointsSurface<false>(CurrentRenderBuffer(), points, screenClip, color);
	}
}

void SDL12VideoDriver::DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.unused < 0xff) {
		DrawPointsSurface<true>(CurrentRenderBuffer(), points, screenClip, reinterpret_cast<const Color&>(color));
	} else {
		DrawPointsSurface<false>(CurrentRenderBuffer(), points, screenClip, reinterpret_cast<const Color&>(color));
	}
}

void SDL12VideoDriver::DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawPolygonSurface<true>(CurrentRenderBuffer(), poly, origin, screenClip, color, fill);
	} else {
		DrawPolygonSurface<false>(CurrentRenderBuffer(), poly, origin, screenClip, color, fill);
	}
}

void SDL12VideoDriver::DrawLineImp(const Point& start, const Point& end, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawLineSurface<true>(CurrentRenderBuffer(), start, end, screenClip, color);
	} else {
		DrawLineSurface<false>(CurrentRenderBuffer(), start, end, screenClip, color);
	}
}

void SDL12VideoDriver::DrawLinesImp(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawLinesSurface<true>(CurrentRenderBuffer(), points, screenClip, color);
	} else {
		DrawLinesSurface<false>(CurrentRenderBuffer(), points, screenClip, color);
	}
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDL12VideoDriver::DrawRectImp(const Region& rgn, const Color& color, bool fill, uint32_t flags)
{
	SDL_Surface* currentBuf = CurrentRenderBuffer();

	if (fill) {
		if (flags&BLIT_BLENDED && color.a < 0xff) {
			SDL_Surface* rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
			SDL_Color c = {color.r, color.g, color.b, color.a};
			SetSurfacePalette(rectsurf, &c, 1);

			assert(rectsurf->format->palette->colors[0].unused == color.a);
			assert(rgn.w > 0 && rgn.h > 0);
			Region clippedrgn = ClippedDrawingRect(rgn);
			SDL_Rect srect = {0, 0, (unsigned short)clippedrgn.w, (unsigned short)clippedrgn.h};
			SDL_Rect drect = RectFromRegion(clippedrgn);

			// use our RGBBlendingPipeline because SDL 1.2 apprently doesnt support blending 8bit surface + SDL_SRCALPHA to 32bit RGBA (seems like a bug)
			RGBBlendingPipeline<NONE, false> blender;
			BlitBlendedRect(rectsurf, currentBuf, srect, drect, blender, 0, NULL);

			SDL_FreeSurface( rectsurf );
		} else {
			Uint32 val = SDL_MapRGBA( currentBuf->format, color.r, color.g, color.b, color.a );
			SDL_Rect drect = RectFromRegion(ClippedDrawingRect(rgn));
			SDL_FillRect( currentBuf, &drect, val );
		}
	} else if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawHLineSurface<true>(currentBuf, rgn.Origin(), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<true>(currentBuf, rgn.Origin(), rgn.y + rgn.h - 1, screenClip, color);
		DrawHLineSurface<true>(currentBuf, Point(rgn.x, rgn.y + rgn.h - 1), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<true>(currentBuf, Point(rgn.x + rgn.w - 1, rgn.y), rgn.y + rgn.h - 1, screenClip, color);
	} else {
		DrawHLineSurface<false>(currentBuf, rgn.Origin(), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<false>(currentBuf, rgn.Origin(), rgn.y + rgn.h - 1, screenClip, color);
		DrawHLineSurface<false>(currentBuf, Point(rgn.x, rgn.y + rgn.h - 1), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<false>(currentBuf, Point(rgn.x + rgn.w - 1, rgn.y), rgn.y + rgn.h - 1, screenClip, color);
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

SDL12VideoDriver::vid_buf_t* SDL12VideoDriver::CurrentRenderBuffer() const
{
	assert(drawingBuffer);
	return static_cast<SDLSurfaceVideoBuffer*>(drawingBuffer)->Surface();
}

SDLVideoDriver::vid_buf_t* SDL12VideoDriver::CurrentStencilBuffer() const
{
	assert(stencilBuffer);
	return std::static_pointer_cast<SDLSurfaceVideoBuffer>(stencilBuffer)->Surface();
}

Sprite2D* SDL12VideoDriver::GetScreenshot(Region r,  const VideoBufferPtr& buf)
{
	unsigned int Width = r.w ? r.w : screenSize.w;
	unsigned int Height = r.h ? r.h : screenSize.h;

	SDLSurfaceSprite2D* screenshot = new SDLSurfaceSprite2D(Region(0,0, Width, Height), 24,
															0x00ff0000, 0x0000ff00, 0x000000ff, 0);
	SDL_Rect src = RectFromRegion(r);
	if (buf) {
		auto surface = static_cast<SDLSurfaceVideoBuffer&>(*buf).Surface();
		SDL_BlitSurface( surface, (r.w && r.h) ? &src : NULL, screenshot->GetSurface(), NULL);
	} else {
		SDL_BlitSurface( disp, (r.w && r.h) ? &src : NULL, screenshot->GetSurface(), NULL);
	}

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

	if (event.type == SDL_KEYDOWN && InTextInput()) {
		int modstate = GetModState(SDL_GetModState());
		Uint16 chr = event.key.keysym.unicode;

		if (isprint(chr) && modstate <= GEM_MOD_SHIFT) {
			char text[2] = { (char)chr, '\0' };
			Event e = EventMgr::CreateTextEvent(text);
			EvntManager->DispatchEvent(e);
			return GEM_OK;
		}
	}

	return SDLVideoDriver::ProcessEvent(event);
}

void SDL12VideoDriver::StartTextInput()
{
	inTextInput = true;
}

void SDL12VideoDriver::StopTextInput()
{
	inTextInput = false;
}

bool SDL12VideoDriver::InTextInput()
{
	return inTextInput;
}

bool SDL12VideoDriver::TouchInputEnabled()
{
    return false;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL1 Video Driver")
PLUGIN_DRIVER(SDL12VideoDriver, "sdl")
END_PLUGIN()
