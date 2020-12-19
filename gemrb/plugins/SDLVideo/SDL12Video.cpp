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
#include "SDLPixelIterator.h"
#include "SDLSurfaceSprite2D.h"
#include "SDL12GamepadMappings.h"

#include "SpriteRenderer.inl"

using namespace GemRB;

SDL12VideoDriver::SDL12VideoDriver(void)
{
	disp = NULL;
	inTextInput = false;
}

SDL12VideoDriver::~SDL12VideoDriver()
{
	if (gameController != nullptr) {
 		SDL_JoystickClose(gameController);
	}
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
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
			Log(ERROR, "SDLJoystick", "InitSubSystem failed: %s", SDL_GetError());
		} else {
			if (SDL_NumJoysticks() > 0) {
				gameController = SDL_JoystickOpen(0);
			}
		}
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
		Log(ERROR, "SDLJoystick", "InitSubSystem failed: %s", SDL_GetError());
	} else {
		if (SDL_NumJoysticks() > 0) {
			gameController = SDL_JoystickOpen(0);
		}
	}

	return ret;
}

int SDL12VideoDriver::CreateSDLDisplay(const char* title)
{
	Log(MESSAGE, "SDL 1.2 Driver", "Creating display");
	ieDword flags = SDL_SWSURFACE;

	Log(MESSAGE, "SDL 1.2 Driver", "SDL_SetVideoMode...");
	disp = SDL_SetVideoMode(screenSize.w, screenSize.h, bpp, flags );
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
			buf = SDL_CreateRGBSurface(0, r.w, r.h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
		} else if (fmt == DISPLAY_ALPHA) {
			SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0);
			buf = SDL_DisplayFormatAlpha(tmp);
			SDL_FreeSurface(tmp);
		} else {
			SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, r.w, r.h, bpp, 0, 0, 0, 0);
			buf = SDL_DisplayFormat(tmp);
			SDL_FreeSurface(tmp);
		}

		return new SDLSurfaceVideoBuffer(buf, r.Origin());
	}
}

IAlphaIterator* SDL12VideoDriver::StencilIterator(uint32_t flags, SDL_Rect maskclip) const
{
	struct SurfaceAlphaIterator : RGBAChannelIterator {
		SDLPixelIterator pixit;
		
		SurfaceAlphaIterator(SDL_Surface* surface, const SDL_Rect& clip, Uint32 mask, Uint8 shift,
							 IPixelIterator::Direction x, IPixelIterator::Direction y)
		: RGBAChannelIterator(&pixit, mask, shift), pixit(surface, x, y, clip) {}
	} *maskit = nullptr;

	if (flags&BLIT_STENCIL_MASK) {
		SDL_Surface* maskSurf = CurrentStencilBuffer();
		SDL_PixelFormat* fmt = maskSurf->format;

		Uint32 mask = 0;
		Uint8 shift = 0;
		if (flags&BLIT_STENCIL_RED) {
			mask = fmt->Rmask;
			shift = fmt->Rshift;
		} else if (flags&BLIT_STENCIL_GREEN) {
			mask = fmt->Gmask;
			shift = fmt->Gshift;
		} else if (flags&BLIT_STENCIL_BLUE) {
			mask = fmt->Bmask;
			shift = fmt->Bshift;
		} else {
			mask = fmt->Amask;
			shift = fmt->Ashift;
		}
		
		const Point& stencilOrigin = stencilBuffer->Origin();
		maskclip.x -= stencilOrigin.x;
		maskclip.y -= stencilOrigin.y;
		IPixelIterator::Direction xdir = (flags&BLIT_MIRRORX) ? IPixelIterator::Reverse : IPixelIterator::Forward;
		IPixelIterator::Direction ydir = (flags&BLIT_MIRRORY) ? IPixelIterator::Reverse : IPixelIterator::Forward;
		maskit = new SurfaceAlphaIterator(maskSurf, maskclip, mask, shift, xdir, ydir);
	}
	
	return maskit;
}

void SDL12VideoDriver::BlitSpriteBAMClipped(const Holder<Sprite2D> spr, const Region& src, const Region& dst,
											uint32_t flags, const Color* t)
{
	Color tint(255,255,255,255);
	if (t) {
		tint = *t;
	}

	// global tint is handled by the callers

	// flag combinations which are often used:
	// (ignoring MIRRORX/Y since those are always resp. never handled by templ.)

	// most game sprites:
	// covered, BLIT_COLOR_MOD

	// area-animations?
	// BLIT_COLOR_MOD

	// (hopefully) most video overlays:
	// BLIT_HALFTRANS
	// covered, BLIT_HALFTRANS
	// covered
	// none

	// other combinations use general case

	PaletteHolder palette = spr->GetPalette();
	SDL_Surface* currentBuf = CurrentRenderBuffer();

	SDL_Rect drect = RectFromRegion(dst);
	IAlphaIterator* maskit = StencilIterator(flags, drect);

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags & ~(BLIT_BLENDED);
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;

	// TODO: we technically only need SRBlender_Alpha when there is a mask. Could boost performance noticably to account for that.

	if (remflags == BLIT_COLOR_MOD && tint.a == 255) {
		SRTinter_Tint<true, true> tinter(tint);
		BlitSpriteRLE<SRBlender_Alpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
	} else if (remflags == BLIT_HALFTRANS) {
		SRTinter_NoTint<false> tinter;
		BlitSpriteRLE<SRBlender_HalfAlpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
	} else if (remflags == 0 && palette->HasAlpha() == false) {
		SRTinter_NoTint<false> tinter;
		BlitSpriteRLE<SRBlender_Alpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
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

		if (!(remflags & BLIT_COLOR_MOD)) tint.a = 255;

		if (palette->HasAlpha()) {
			if (remflags & BLIT_COLOR_MOD) {
				SRTinter_Flags<true> tinter(tint);
				BlitSpriteRLE<SRBlender_Alpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
			} else {
				SRTinter_FlagsNoTint<true> tinter;
				BlitSpriteRLE<SRBlender_Alpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
			}
		} else {
			if (remflags & BLIT_COLOR_MOD) {
				SRTinter_Flags<false> tinter(tint);
				BlitSpriteRLE<SRBlender_Alpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
			} else {
				SRTinter_FlagsNoTint<false> tinter;
				BlitSpriteRLE<SRBlender_Alpha>(spr, src, currentBuf, dst, maskit, flags, tinter);
			}
		}
	}

	spr->UnlockSprite();
	delete maskit;
}

void SDL12VideoDriver::BlitSpriteNativeClipped(const sprite_t* spr, const SDL_Rect& srect, const SDL_Rect& drect, uint32_t flags, const SDL_Color* tint)
{
	const SDLSurfaceSprite2D *sdlspr = static_cast<const SDLSurfaceSprite2D*>(spr);
	SDL_Surface* surf = sdlspr->GetSurface();

	Color c;
	if (tint && (flags&BLIT_COLOR_MOD)){
		c = Color(tint->r, tint->g, tint->b, tint->unused);
	}

	if (surf->format->BytesPerPixel == 1) {
		c.a = SDL_ALPHA_OPAQUE; // FIXME: this is probably actually contigent on something else...

		if (flags&BLIT_COLOR_MOD) {
			RenderSpriteVersion(sdlspr, flags, &c);
		} else {
			RenderSpriteVersion(sdlspr, flags);
		}
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
	if (flags & BLIT_GREY) flags &= ~BLIT_SEPIA;

	SDL_Surface* currentBuf = CurrentRenderBuffer();
	IAlphaIterator* maskIt = StencilIterator(flags, drect);
	
	bool nativeBlit = (flags & ~(BLIT_HALFTRANS | BLIT_ALPHA_MOD | BLIT_BLENDED)) == 0
						&& maskIt == nullptr && ((surf->flags & SDL_SRCCOLORKEY) != 0
						|| (flags & BLIT_BLENDED) == 0);
	if (nativeBlit) {
		// must be checked afer palette versioning is done
		
		// the gamewin is an RGB surface (no alpha)
		// RGBA->RGB with SDL_SRCALPHA
		//     The source is alpha-blended with the destination, using the alpha channel. SDL_SRCCOLORKEY and the per-surface alpha are ignored.
		// RGBA->RGB without SDL_SRCALPHA
		//     The RGB data is copied from the source. The source alpha channel and the per-surface alpha value are ignored.
		
		Uint8 alpha = SDL_ALPHA_OPAQUE;
		if (flags & BLIT_ALPHA_MOD) {
			alpha = tint.a;
		}
		
		if (flags & BLIT_HALFTRANS) {
			alpha /= 2;
		}
		
		if (flags & BLIT_BLENDED) {
			SDL_SetAlpha(surf, SDL_SRCALPHA, alpha);
		} else {
			SDL_SetAlpha(surf, 0, alpha);
		}

		SDL_Rect s = srect;
		SDL_Rect d = drect;
		SDL_LowerBlit(surf, &s, currentBuf, &d);
	} else {
		bool halftrans = flags & BLIT_HALFTRANS;
		if (halftrans && (flags ^ BLIT_HALFTRANS)) { // other flags are set too
			// handle halftrans with 50% alpha tinting
			// force use of RGBBlendingPipeline with tint parameter if we aren't already
			if (!(flags & (BLIT_COLOR_MOD | BLIT_ALPHA_MOD))) {
				tint = ColorWhite;
				flags |= BLIT_COLOR_MOD;
			}
			tint.a /= 2;
		}

		// FIXME: this always assumes BLIT_BLENDED if any "shader" flags are set
		// we don't currently have a need for non blended sprites (we do for primitives, which is handled elsewhere)
		// however, it could make things faster if we handled it

		if (flags & (BLIT_COLOR_MOD | BLIT_ALPHA_MOD)) {
			if (flags&BLIT_GREY) {
				RGBBlendingPipeline<GREYSCALE, true> blender(tint);
				BlitBlendedRect(surf, currentBuf, srect, drect, blender, flags, maskIt);
			} else if (flags&BLIT_SEPIA) {
				RGBBlendingPipeline<SEPIA, true> blender(tint);
				BlitBlendedRect(surf, currentBuf, srect, drect, blender, flags, maskIt);
			} else {
				RGBBlendingPipeline<TINT, true> blender(tint);
				BlitBlendedRect(surf, currentBuf, srect, drect, blender, flags, maskIt);
			}
		} else if (flags&BLIT_GREY) {
			RGBBlendingPipeline<GREYSCALE, true> blender;
			BlitBlendedRect(surf, currentBuf, srect, drect, blender, flags, maskIt);
		} else if (flags&BLIT_SEPIA) {
			RGBBlendingPipeline<SEPIA, true> blender;
			BlitBlendedRect(surf, currentBuf, srect, drect, blender, flags, maskIt);
		} else {
			RGBBlendingPipeline<NONE, true> blender;
			BlitBlendedRect(surf, currentBuf, srect, drect, blender, flags, maskIt);
		}
	}
	
	delete maskIt;
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
		DrawPointSurface<BLEND>(CurrentRenderBuffer(), p, CurrentRenderClip(), color);
	} else if (flags & BLIT_MULTIPLY) {
		DrawPointSurface<TINT>(CurrentRenderBuffer(), p, CurrentRenderClip(), color);
	} else {
		DrawPointSurface<NONE>(CurrentRenderBuffer(), p, CurrentRenderClip(), color);
	}
}

void SDL12VideoDriver::DrawPointsImp(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawPointsSurface<BLEND>(CurrentRenderBuffer(), points, CurrentRenderClip(), color);
	} else if (flags & BLIT_MULTIPLY) {
		DrawPointsSurface<TINT>(CurrentRenderBuffer(), points, CurrentRenderClip(), color);
	} else {
		DrawPointsSurface<NONE>(CurrentRenderBuffer(), points, CurrentRenderClip(), color);
	}
}

void SDL12VideoDriver::DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.unused < 0xff) {
		DrawPointsSurface<BLEND>(CurrentRenderBuffer(), points, CurrentRenderClip(), reinterpret_cast<const Color&>(color));
	} else if (flags & BLIT_MULTIPLY) {
		DrawPointsSurface<TINT>(CurrentRenderBuffer(), points, CurrentRenderClip(), reinterpret_cast<const Color&>(color));
	} else {
		DrawPointsSurface<NONE>(CurrentRenderBuffer(), points, CurrentRenderClip(), reinterpret_cast<const Color&>(color));
	}
}

void SDL12VideoDriver::DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawPolygonSurface<BLEND>(CurrentRenderBuffer(), poly, origin, CurrentRenderClip(), color, fill);
	} else if (flags & BLIT_MULTIPLY) {
		DrawPolygonSurface<TINT>(CurrentRenderBuffer(), poly, origin, CurrentRenderClip(), color, fill);
	} else {
		DrawPolygonSurface<NONE>(CurrentRenderBuffer(), poly, origin, CurrentRenderClip(), color, fill);
	}
}

void SDL12VideoDriver::DrawLineImp(const Point& start, const Point& end, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawLineSurface<BLEND>(CurrentRenderBuffer(), start, end, CurrentRenderClip(), color);
	} else if (flags & BLIT_MULTIPLY) {
		DrawLineSurface<TINT>(CurrentRenderBuffer(), start, end, CurrentRenderClip(), color);
	} else {
		DrawLineSurface<NONE>(CurrentRenderBuffer(), start, end, CurrentRenderClip(), color);
	}
}

void SDL12VideoDriver::DrawLinesImp(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawLinesSurface<BLEND>(CurrentRenderBuffer(), points, CurrentRenderClip(), color);
	} else if (flags & BLIT_MULTIPLY) {
		DrawLinesSurface<TINT>(CurrentRenderBuffer(), points, CurrentRenderClip(), color);
	} else {
		DrawLinesSurface<NONE>(CurrentRenderBuffer(), points, CurrentRenderClip(), color);
	}
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDL12VideoDriver::DrawRectImp(const Region& rgn, const Color& color, bool fill, uint32_t flags)
{
	SDL_Surface* currentBuf = CurrentRenderBuffer();

	if (fill) {
		if (flags&BLIT_BLENDED && color.a < 0xff) {
			assert(rgn.w > 0 && rgn.h > 0);
			
			const static OneMinusSrcA<false, false> blender;
			
			Region clippedrgn = ClippedDrawingRect(rgn);
			SDLPixelIterator dstit(currentBuf, RectFromRegion(clippedrgn));
			SDLPixelIterator dstend = SDLPixelIterator::end(dstit);
			ColorFill(color, dstit, dstend, blender);
		} else {
			Uint32 val = SDL_MapRGBA( currentBuf->format, color.r, color.g, color.b, color.a );
			SDL_Rect drect = RectFromRegion(ClippedDrawingRect(rgn));
			SDL_FillRect( currentBuf, &drect, val );
		}
	} else if (flags&BLIT_BLENDED && color.a < 0xff) {
		DrawHLineSurface<BLEND>(currentBuf, rgn.Origin(), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<BLEND>(currentBuf, rgn.Origin(), rgn.y + rgn.h - 1, screenClip, color);
		DrawHLineSurface<BLEND>(currentBuf, Point(rgn.x, rgn.y + rgn.h - 1), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<BLEND>(currentBuf, Point(rgn.x + rgn.w - 1, rgn.y), rgn.y + rgn.h - 1, screenClip, color);
	} else if (flags & BLIT_MULTIPLY) {
		DrawHLineSurface<TINT>(currentBuf, rgn.Origin(), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<TINT>(currentBuf, rgn.Origin(), rgn.y + rgn.h - 1, screenClip, color);
		DrawHLineSurface<TINT>(currentBuf, Point(rgn.x, rgn.y + rgn.h - 1), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<TINT>(currentBuf, Point(rgn.x + rgn.w - 1, rgn.y), rgn.y + rgn.h - 1, screenClip, color);
	} else {
		DrawHLineSurface<NONE>(currentBuf, rgn.Origin(), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<NONE>(currentBuf, rgn.Origin(), rgn.y + rgn.h - 1, screenClip, color);
		DrawHLineSurface<NONE>(currentBuf, Point(rgn.x, rgn.y + rgn.h - 1), rgn.x + rgn.w - 1, screenClip, color);
		DrawVLineSurface<NONE>(currentBuf, Point(rgn.x + rgn.w - 1, rgn.y), rgn.y + rgn.h - 1, screenClip, color);
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
		disp = SDL_SetVideoMode(disp->w, disp->h, disp->format->BitsPerPixel, flags | SDL_SWSURFACE | SDL_ANYFORMAT);

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

SDL12VideoDriver::vid_buf_t* SDL12VideoDriver::ScratchBuffer() const
{
	assert(scratchBuffer);
	return std::static_pointer_cast<SDLSurfaceVideoBuffer>(scratchBuffer)->Surface();
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

Holder<Sprite2D> SDL12VideoDriver::GetScreenshot(Region r,  const VideoBufferPtr& buf)
{
	unsigned int Width = r.w ? r.w : screenSize.w;
	unsigned int Height = r.h ? r.h : screenSize.h;

	SDLSurfaceSprite2D *screenshot = new SDLSurfaceSprite2D(Region(0,0, Width, Height), 24,
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

	if ((SDL_EVENTMASK(event.type) & (SDL_MOUSEBUTTONDOWNMASK))
		&& (event.button.button == SDL_BUTTON_WHEELUP || event.button.button == SDL_BUTTON_WHEELDOWN)) {
		// remap these to mousewheel events
		int speed = core->GetMouseScrollSpeed();
		speed *= (event.button.button == SDL_BUTTON_WHEELUP) ? 1 : -1;
		Event e;
		if (SDL_GetModState() & KMOD_SHIFT) {
			e = EventMgr::CreateMouseWheelEvent(Point(speed, 0));
		} else{
			e = EventMgr::CreateMouseWheelEvent(Point(0, speed));
		}
		EvntManager->DispatchEvent(e);
		return GEM_OK;
	}
	
	if (InTextInput()) {
		if (event.type == SDL_KEYDOWN) {
			int modstate = GetModState(SDL_GetModState());
			Uint16 chr = event.key.keysym.unicode;

			if (isprint(chr) && modstate <= GEM_MOD_SHIFT) {
				char text[2] = { (char)chr, '\0' };
				Event e = EventMgr::CreateTextEvent(text);
				EvntManager->DispatchEvent(e);
				return GEM_OK;
			}
		} else if (event.type == SDL_JOYBUTTONDOWN) {
			Event bsp = EventMgr::CreateKeyEvent(GEM_BACKSP, true);
			
			switch(event.jbutton.button) {
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				dPadSoftKeyboard.RemoveCharacter();
				EvntManager->DispatchEvent(bsp);
				return GEM_OK;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				dPadSoftKeyboard.AddCharacter();
				EvntManager->DispatchEvent(dPadSoftKeyboard.GetTextEvent());
				return GEM_OK;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				dPadSoftKeyboard.NextCharacter();
				EvntManager->DispatchEvent(bsp);
				EvntManager->DispatchEvent(dPadSoftKeyboard.GetTextEvent());
				return GEM_OK;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				dPadSoftKeyboard.PreviousCharacter();
				EvntManager->DispatchEvent(bsp);
				EvntManager->DispatchEvent(dPadSoftKeyboard.GetTextEvent());
				return GEM_OK;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				dPadSoftKeyboard.ToggleUppercase();
				EvntManager->DispatchEvent(bsp);
				EvntManager->DispatchEvent(dPadSoftKeyboard.GetTextEvent());
				return GEM_OK;
			}
		}
	}

	return SDLVideoDriver::ProcessEvent(event);
}

void SDL12VideoDriver::StartTextInput()
{
	inTextInput = true;
	dPadSoftKeyboard.StartInput();
}

void SDL12VideoDriver::StopTextInput()
{
	inTextInput = false;
	dPadSoftKeyboard.StopInput();
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
