/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2006 The GemRB Project
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

#include "SDLVideo.h"

#include "Interface.h"
#include "Palette.h"
#include "Pixels.h"


#if defined(__sgi)
#  include <math.h>
#  ifdef __cplusplus
extern "C" double round(double);
#  endif
#else
#  include <cmath>
#endif

using namespace GemRB;

SDLVideoDriver::SDLVideoDriver(void)
{
	lastTime = 0;
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	SDL_Quit();
}

int SDLVideoDriver::Init(void)
{
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1) {
		return GEM_ERROR;
	}
	SDL_ShowCursor(SDL_DISABLE);
	return GEM_OK;
}

int SDLVideoDriver::PollEvents()
{
	int ret = GEM_OK;
	SDL_Event currentEvent;

	while (ret != GEM_ERROR && SDL_PollEvent(&currentEvent)) {
		ret = ProcessEvent(currentEvent);
	}

	return ret;
}

int SDLVideoDriver::ProcessEvent(const SDL_Event & event)
{
	if (!EvntManager)
		return GEM_ERROR;

	// FIXME: technically event.key.keysym.mod should be the mod,
	// but for the mod keys themselves this is 0 and therefore not what GemRB expects
	// int modstate = GetModState(event.key.keysym.mod);
	int modstate = GetModState(SDL_GetModState());
	SDLKey sym = event.key.keysym.sym;
	SDL_Keycode key = sym;
	Event e;

	/* Loop until there are no events left on the queue */
	switch (event.type) {
			/* Process the appropriate event type */
		case SDL_QUIT:
			/* Quit event originated from outside GemRB so ask the user if we should exit */
			core->AskAndExit();
			return GEM_OK;
		case SDL_KEYUP:
			switch(sym) {
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				case SDLK_SCROLLOCK:
					key = GEM_GRAB;
					break;
				default:
					if (sym < 256) {
						key = sym;
					}
					break;
			}
			if (key != 0) {
				Event e = EvntManager->CreateKeyEvent(key, false, modstate);
				EvntManager->DispatchEvent(e);
			}
			break;
		case SDL_KEYDOWN:
			// reenable special numpad keys unless numlock is off
			if (SDL_GetModState() & KMOD_NUM) {
				switch (sym) {
					case SDLK_KP1: sym = SDLK_1; break;
					case SDLK_KP2: sym = SDLK_2; break;
					case SDLK_KP3: sym = SDLK_3; break;
					case SDLK_KP4: sym = SDLK_4; break;
					// 5 is not special
					case SDLK_KP6: sym = SDLK_6; break;
					case SDLK_KP7: sym = SDLK_7; break;
					case SDLK_KP8: sym = SDLK_8; break;
					case SDLK_KP9: sym = SDLK_9; break;
					default: break;
				}
			}
			switch (sym) {
				case SDLK_ESCAPE:
					key = GEM_ESCAPE;
					break;
				case SDLK_END:
				case SDLK_KP1:
					key = GEM_END;
					break;
				case SDLK_HOME:
				case SDLK_KP7:
					key = GEM_HOME;
					break;
				case SDLK_UP:
				case SDLK_KP8:
					key = GEM_UP;
					break;
				case SDLK_DOWN:
				case SDLK_KP2:
					key = GEM_DOWN;
					break;
				case SDLK_LEFT:
				case SDLK_KP4:
					key = GEM_LEFT;
					break;
				case SDLK_RIGHT:
				case SDLK_KP6:
					key = GEM_RIGHT;
					break;
				case SDLK_DELETE:
#if TARGET_OS_IPHONE < 1
					//iOS currently doesnt have a backspace so we use delete.
					//This change should be future proof in the event apple changes the delete key to a backspace.
					key = GEM_DELETE;
					break;
#endif
				case SDLK_BACKSPACE:
					key = GEM_BACKSP;
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					key = GEM_RETURN;
					break;
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				case SDLK_TAB:
					key = GEM_TAB;
					break;
				case SDLK_PAGEUP:
				case SDLK_KP9:
					key = GEM_PGUP;
					break;
				case SDLK_PAGEDOWN:
				case SDLK_KP3:
					key = GEM_PGDOWN;
					break;
				case SDLK_SCROLLOCK:
					key = GEM_GRAB;
					break;
				case SDLK_F1:
				case SDLK_F2:
				case SDLK_F3:
				case SDLK_F4:
				case SDLK_F5:
				case SDLK_F6:
				case SDLK_F7:
				case SDLK_F8:
				case SDLK_F9:
				case SDLK_F10:
				case SDLK_F11:
				case SDLK_F12:
					//assuming they come sequentially,
					//also, there is no need to ever produce more than 12
					key = GEM_FUNCTIONX(1) + sym-SDLK_F1;
					break;
				default:
					break;
			}

			e = EvntManager->CreateKeyEvent(key, true, modstate);
			if (e.keyboard.character) {
				if (InTextInput() && modstate == 0) {
					return GEM_OK;
				}
#if SDL_VERSION_ATLEAST(1,3,0)
				e.keyboard.character = SDL_GetKeyFromScancode(event.key.keysym.scancode);
#else
				e.keyboard.character = event.key.keysym.unicode;
#endif
			}

			EvntManager->DispatchEvent(e);
			break;
		case SDL_MOUSEMOTION:
			e = EvntManager->CreateMouseMotionEvent(Point(event.motion.x, event.motion.y), modstate);
			EvntManager->DispatchEvent(e);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			bool down = (event.type == SDL_MOUSEBUTTONDOWN) ? true : false;
			Point p(event.button.x, event.button.y);
			EventButton btn = SDL_BUTTON(event.button.button);
			e = EvntManager->CreateMouseBtnEvent(p, btn, down, modstate);
			EvntManager->DispatchEvent(e);
			break;
	}
	return GEM_OK;
}

Sprite2D* SDLVideoDriver::CreateSprite(const Region& rgn, int bpp, ieDword rMask,
	ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	sprite_t* spr = new sprite_t(rgn, bpp, pixels, rMask, gMask, bMask, aMask);

	if (cK) {
		spr->SetColorKey(index);
	}
	/*
	 there is at least one place (BlitGameSprite) that requires 8 or 32bpp sprites
	 untill we support 16bpp fully we cannot do this

	// make sure colorkey is set prior to conversion
	SDL_PixelFormat* fmt = backBuf->format;
	spr->ConvertFormatTo(fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
	*/
	return spr;
}

Sprite2D* SDLVideoDriver::CreateSprite8(const Region& rgn, void* pixels,
										Palette* palette, bool cK, int index)
{
	if (palette) {
		return CreatePalettedSprite(rgn, 8, pixels, palette->col, cK, index);
	} else {
		// an alpha only sprite. used by SpriteCover or as a mask passed to BlitTile
		sprite_t* spr = new sprite_t(rgn, 8, pixels, 0, 0, 0, 0);
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_Surface* mask = spr->GetSurface();
		for (int i = 0; i < mask->format->palette->ncolors; ++i) {
			SDL_Color* c = &mask->format->palette->colors[i];
			c->r = 0;
			c->g = 0;
			c->b = 0;
			c->a = i;
		}
#endif
		return spr;
	}
}

Sprite2D* SDLVideoDriver::CreatePalettedSprite(const Region& rgn, int bpp, void* pixels,
											   Color* palette, bool cK, int index)
{
	sprite_t* spr = new sprite_t(rgn, bpp, pixels, 0, 0, 0, 0);

	spr->SetPalette(palette);
	if (cK) {
		spr->SetColorKey(index);
	}
	return spr;
}

void SDLVideoDriver::BlitTile(const Sprite2D* spr, int x, int y, const Region* clip, unsigned int flags, const Color* tint)
{
	assert(spr->BAM == false);

	Region fClip = ClippedDrawingRect(Region(x, y, 64, 64), clip);
	Region srect(0, 0, fClip.w, fClip.h);
	srect.x -= x - fClip.x;
	srect.y -= y - fClip.y;

	BlitSpriteClipped(spr, srect, fClip, flags, tint);
}

void SDLVideoDriver::BlitSprite(const Sprite2D* spr, const Region& src, Region dst)
{
	dst.x -= spr->Frame.x;
	dst.y -= spr->Frame.y;
	unsigned int flags = (spr->HasTransparency()) ? BLIT_BLENDED : 0;
	BlitSpriteClipped(spr, src, dst, flags);
}

void SDLVideoDriver::BlitGameSprite(const Sprite2D* spr, int x, int y,
									unsigned int flags, Color tint, const Region* clip)
{
	Region srect(Point(0, 0), (clip) ? clip->Dimensions() : Size(spr->Frame.w, spr->Frame.h));
	Region drect = (clip) ? *clip : Region(x - spr->Frame.x, y - spr->Frame.y, spr->Frame.w, spr->Frame.h);
	BlitSpriteClipped(spr, srect, drect, flags, &tint);
}

// SetPixel is in screen coordinates
#if SDL_VERSION_ATLEAST(1,3,0)
#define SetPixel(buffer, _x, _y) { \
Region _r = Region(Point(0,0), buffer->Size()); \
Point _p(_x, _y); \
if (_r.PointInside(_p)) { SDL_Point _p2 = {_p.x,_p.y}; points.push_back(_p2); } }
#else
#define SetPixel(buffer, _x, _y) { \
Region _r = Region(Point(0,0), buffer->Size()); \
Point _p(_x, _y); \
if (_r.PointInside(_p)) { points.push_back(_p); } }
#endif

/** This functions Draws a Circle */
void SDLVideoDriver::DrawCircle(const Point& c, unsigned short r, const Color& color, unsigned int flags)
{
	//Uses the Breshenham's Circle Algorithm
	long x, y, xc, yc, re;

	x = r;
	y = 0;
	xc = 1 - ( 2 * r );
	yc = 1;
	re = 0;

	std::vector<SDL_Point> points;

	while (x >= y) {
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) y, c.y + ( short ) x );
		SetPixel( drawingBuffer, c.x - ( short ) y, c.y + ( short ) x );
		SetPixel( drawingBuffer, c.x - ( short ) y, c.y - ( short ) x );
		SetPixel( drawingBuffer, c.x + ( short ) y, c.y - ( short ) x );

		y++;
		re += yc;
		yc += 2;

		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color), flags);
}

static double ellipseradius(unsigned short xr, unsigned short yr, double angle) {
	double one = (xr * sin(angle));
	double two = (yr * cos(angle));
	return sqrt(xr*xr*yr*yr / (one*one + two*two));
}

/** This functions Draws an Ellipse Segment */
void SDLVideoDriver::DrawEllipseSegment(const Point& c, unsigned short xr,
	unsigned short yr, const Color& color, double anglefrom, double angleto, bool drawlines, unsigned int flags)
{
	/* beware, dragons and clockwise angles be here! */
	double radiusfrom = ellipseradius(xr, yr, anglefrom);
	double radiusto = ellipseradius(xr, yr, angleto);
	long xfrom = (long)round(radiusfrom * cos(anglefrom));
	long yfrom = (long)round(radiusfrom * sin(anglefrom));
	long xto = (long)round(radiusto * cos(angleto));
	long yto = (long)round(radiusto * sin(angleto));

	if (drawlines) {
		DrawLine(c, Point(c.x + xfrom, c.y + yfrom), color, flags);
		DrawLine(c, Point(c.x + xto, c.y + yto), color, flags);
	}

	// *Attempt* to calculate the correct x/y boundaries.
	// TODO: this doesn't work very well - you can't actually bound many
	// arcs this way (imagine a segment with a small piece cut out).
	if (xfrom > xto) {
		long tmp = xfrom; xfrom = xto; xto = tmp;
	}
	if (yfrom > yto) {
		long tmp = yfrom; yfrom = yto; yto = tmp;
	}
	if (xfrom >= 0 && yto >= 0) xto = xr;
	if (xto <= 0 && yto >= 0) xfrom = -xr;
	if (yfrom >= 0 && xto >= 0) yto = yr;
	if (yto <= 0 && xto >= 0) yfrom = -yr;

	//Uses Bresenham's Ellipse Algorithm
	long x, y, xc, yc, ee, tas, tbs, sx, sy;

	tas = 2 * xr * xr;
	tbs = 2 * yr * yr;
	x = xr;
	y = 0;
	xc = yr * yr * ( 1 - ( 2 * xr ) );
	yc = xr * xr;
	ee = 0;
	sx = tbs * xr;
	sy = 0;

	std::vector<SDL_Point> points;

	while (sx >= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		y++;
		sy += tas;
		ee += yc;
		yc += tas;
		if (( 2 * ee + xc ) > 0) {
			x--;
			sx -= tbs;
			ee += xc;
			xc += tbs;
		}
	}

	x = 0;
	y = yr;
	xc = yr * yr;
	yc = xr * xr * ( 1 - ( 2 * yr ) );
	ee = 0;
	sx = 0;
	sy = tas * yr;

	while (sx <= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		x++;
		sx += tbs;
		ee += xc;
		xc += tbs;
		if (( 2 * ee + yc ) > 0) {
			y--;
			sy -= tas;
			ee += yc;
			yc += tas;
		}
	}

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color), flags);
}


/** This functions Draws an Ellipse */
void SDLVideoDriver::DrawEllipse(const Point& c, unsigned short xr,
								 unsigned short yr, const Color& color, unsigned int flags)
{
	//Uses Bresenham's Ellipse Algorithm
	long x, y, xc, yc, ee, tas, tbs, sx, sy;

	tas = 2 * xr * xr;
	tbs = 2 * yr * yr;
	x = xr;
	y = 0;
	xc = yr * yr * ( 1 - ( 2 * xr ) );
	yc = xr * xr;
	ee = 0;
	sx = tbs * xr;
	sy = 0;

	std::vector<SDL_Point> points;

	while (sx >= sy) {
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		y++;
		sy += tas;
		ee += yc;
		yc += tas;
		if (( 2 * ee + xc ) > 0) {
			x--;
			sx -= tbs;
			ee += xc;
			xc += tbs;
		}
	}

	x = 0;
	y = yr;
	xc = yr * yr;
	yc = xr * xr * ( 1 - ( 2 * yr ) );
	ee = 0;
	sx = 0;
	sy = tas * yr;

	while (sx <= sy) {
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		x++;
		sx += tbs;
		ee += xc;
		xc += tbs;
		if (( 2 * ee + yc ) > 0) {
			y--;
			sy -= tas;
			ee += yc;
			yc += tas;
		}
	}

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color), flags);
}

#undef SetPixel

void SDLVideoDriver::BlitSpriteClipped(const Sprite2D* spr, Region src, const Region& dst, unsigned int flags, const Color* tint)
{
#if SDL_VERSION_ATLEAST(1,3,0)
	// in SDL2 SDL_RenderCopyEx will flip the src rect internally if BLIT_MIRRORX or BLIT_MIRRORY is set
	// instead of doing this and then reversing it in that case only for SDL to reverse it yet again
	// lets just not worry about clipping on SDL2. the backends handle all of that for us unline with SDL 1 where we
	// might walk off a memory buffer we have no danger of that. This fixes bizzare clipping issues when a "flipped" sprit is partially offscreen
	Region dclipped = dst;
#else
	// FIXME?: srect isn't verified
	Region dclipped = ClippedDrawingRect(dst);
	int trim = dst.h - dclipped.h;
	if (trim) {
		src.h -= trim;
		if (dclipped.y > dst.y) { // top clipped
			src.y += trim;
		} // already have appropriate y for bottom clip
	}
	trim = dst.w - dclipped.w;
	if (trim) {
		src.w -= trim;
		if (dclipped.x > dst.x) { // left clipped
			src.x += trim;
		}
	} // already have appropriate y for right clip

	if (dclipped.Dimensions().IsEmpty() || src.Dimensions().IsEmpty()) {
		return;
	}

	assert(dclipped.w == src.w && dclipped.h == src.h);
#endif

	if (spr->renderFlags&BLIT_MIRRORX) {
		flags ^= BLIT_MIRRORX;
	}

	if (spr->renderFlags&BLIT_MIRRORY) {
		flags ^= BLIT_MIRRORY;
	}

	if (spr->BAM) {
		BlitSpriteBAMClipped(spr, src, dclipped, flags, tint);
	} else {
		SDL_Rect srect = RectFromRegion(src);
		SDL_Rect drect = RectFromRegion(dclipped);
		BlitSpriteNativeClipped(spr, srect, drect, flags, reinterpret_cast<const SDL_Color*>(tint));
	}
}

void SDLVideoDriver::RenderSpriteVersion(const SDLSurfaceSprite2D* spr, unsigned int renderflags, const Color* tint)
{
	SDLSurfaceSprite2D::version_t version = spr->GetVersion(false);

	if (renderflags != version) {
		if (spr->Bpp == 8) {
			SDL_Palette* pal = static_cast<SDL_Palette*>(spr->NewVersion(renderflags));

			for (size_t i = 0; i < 256; ++i) {
				Color& dstc = reinterpret_cast<Color&>(pal->colors[i]);

				if (renderflags&BLIT_TINTED) {
					assert(tint);
					ShaderTint(*tint, dstc);
				}

				if (renderflags&BLIT_GREY) {
					ShaderGreyscale(dstc);
				} else if (renderflags&BLIT_SEPIA) {
					ShaderSepia(dstc);
				}
			}

#if SDL_VERSION_ATLEAST(1,3,0)
			// FIXME: verify these are correct
			if (renderflags&BLIT_NOSHADOW) {
				pal->colors[1].a = 0;
			} else if (renderflags&BLIT_TRANSSHADOW) {
				pal->colors[1].a = 128;
			}
#endif
		} else {
			// FIXME: this should technically support BLIT_TINTED; we just dont use it
			// FIXME: this should technically support BLIT_NOSHADOW and BLIT_TRANSSHADOW

			SDL_Surface* newV = (SDL_Surface*)spr->NewVersion(renderflags);
			SDL_LockSurface(newV);

			SDL_Rect r = {0, 0, (unsigned short)newV->w, (unsigned short)newV->h};
			SDLPixelIterator beg(r, newV);
			SDLPixelIterator end = SDLPixelIterator::end(beg);
			StaticIterator alpha(Color(0,0,0,0xff));

			if (renderflags & BLIT_GREY) {
				RGBBlendingPipeline<GREYSCALE, true> blender;
				Blit(beg, beg, end, alpha, blender);
			} else if (renderflags & BLIT_SEPIA) {
				RGBBlendingPipeline<SEPIA, true> blender;
				Blit(beg, beg, end, alpha, blender);
			}
			SDL_UnlockSurface(newV);
		}
	}
}

// static class methods

int SDLVideoDriver::SetSurfacePalette(SDL_Surface* surf, const SDL_Color* pal, int numcolors)
{
	if (pal) {
#if SDL_VERSION_ATLEAST(1,3,0)
		return SDL_SetPaletteColors( surf->format->palette, pal, 0, numcolors );
#else
		// const_cast because SDL doesnt alter this and we want our interface to be const correct
		return SDL_SetPalette( surf, SDL_LOGPAL, const_cast<SDL_Color*>(pal), 0, numcolors );
#endif
	}
	return -1;
}

void SDLVideoDriver::SetSurfacePixels(SDL_Surface* surface, const std::vector<SDL_Point>& points, const Color& srcc)
{
	SDL_PixelFormat* fmt = surface->format;
	SDL_LockSurface( surface );

	std::vector<SDL_Point>::const_iterator it;
	it = points.begin();
	for (; it != points.end(); ++it) {
		SDL_Point p = *it;

		unsigned char* start = static_cast<unsigned char*>(surface->pixels);
		unsigned char* dst = start + ((p.y * surface->pitch) + (p.x * fmt->BytesPerPixel));

		if (dst >= start + (surface->pitch * surface->h)) {
			// points cannot be relied upon to be clipped because we allow public access to DrawPoints
			continue;
		}

		Color dstc;
		switch (fmt->BytesPerPixel) {
			case 1:
				SDL_GetRGB( *dst, surface->format, &dstc.r, &dstc.g, &dstc.b );
				ShaderBlend<false>(srcc, dstc);
				*dst = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				break;
			case 2:
				SDL_GetRGB( *reinterpret_cast<Uint16*>(dst), surface->format, &dstc.r, &dstc.g, &dstc.b );
				ShaderBlend<false>(srcc, dstc);
				*reinterpret_cast<Uint16*>(dst) = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				break;
			case 3:
			{
				// FIXME: implement alpha blending for this... or nix it
				// is this even used?
				/*
				Uint32 val = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				pixel[0] = val & 0xff;
				pixel[1] = (val >> 8) & 0xff;
				pixel[2] = (val >> 16) & 0xff;
	#else
				pixel[2] = val & 0xff;
				pixel[1] = (val >> 8) & 0xff;
				pixel[0] = (val >> 16) & 0xff;
	#endif
				*/
			}
				break;
			case 4:
				SDL_GetRGB( *reinterpret_cast<Uint32*>(dst), surface->format, &dstc.r, &dstc.g, &dstc.b );
				ShaderBlend<false>(srcc, dstc);
				*reinterpret_cast<Uint32*>(dst) = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				break;
			default:
				Log(ERROR, "sprite_t", "Working with unknown pixel format: %s", SDL_GetError());
				break;
		}
	}

	SDL_UnlockSurface( surface );
}

Color SDLVideoDriver::GetSurfacePixel(SDL_Surface* surface, short x, short y)
{
	Color c;
	SDL_LockSurface( surface );
	Uint8 Bpp = surface->format->BytesPerPixel;
	unsigned char * pixels = ( ( unsigned char * ) surface->pixels ) +
	( ( y * surface->pitch + (x*Bpp)) );
	Uint32 val = 0;

	if (Bpp == 1) {
		val = *pixels;
	} else if (Bpp == 2) {
		val = *(Uint16 *)pixels;
	} else if (Bpp == 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		val = pixels[0] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[2] << 16);
#else
		val = pixels[2] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[0] << 16);
#endif
	} else if (Bpp == 4) {
		val = *(Uint32 *)pixels;
	}
	
	SDL_UnlockSurface( surface );
	SDL_GetRGBA( val, surface->format, (Uint8 *) &c.r, (Uint8 *) &c.g, (Uint8 *) &c.b, (Uint8 *) &c.a );
	
	// check color key... SDL_GetRGBA wont get this
#if SDL_VERSION_ATLEAST(1,3,0)
	Uint32 ck;
	if (SDL_GetColorKey(surface, &ck) != -1 && ck == val) c.a = SDL_ALPHA_TRANSPARENT;
#else
	if (surface->format->colorkey == val) c.a = SDL_ALPHA_TRANSPARENT;
#endif
	return c;
}
