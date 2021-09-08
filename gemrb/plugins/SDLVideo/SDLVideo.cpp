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
#include "Video/RLE.h"
#include "SDLPixelIterator.h"

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
		Log(ERROR, "SDLVideo", "InitSubSystem failed: %s", SDL_GetError());
		return GEM_ERROR;
	}
	SDL_ShowCursor(SDL_DISABLE);
	return GEM_OK;
}

int SDLVideoDriver::CreateDriverDisplay(const char* title)
{
	int ret = CreateSDLDisplay(title);
	scratchBuffer = CreateBuffer(Region(Point(), screenSize), BufferFormat::DISPLAY_ALPHA);
	scratchBuffer->Clear();
	return ret;
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

static SDL_Keycode TranslateKeycode(SDLKey sym)
{
	switch (sym) {
		case SDLK_ESCAPE:
			return GEM_ESCAPE;
		case SDLK_END:
		case SDLK_KP1:
			return GEM_END;
		case SDLK_HOME:
		case SDLK_KP7:
			return GEM_HOME;
		case SDLK_UP:
		case SDLK_KP8:
			return GEM_UP;
		case SDLK_DOWN:
		case SDLK_KP2:
			return GEM_DOWN;
		case SDLK_LEFT:
		case SDLK_KP4:
			return GEM_LEFT;
		case SDLK_RIGHT:
		case SDLK_KP6:
			return GEM_RIGHT;
		case SDLK_DELETE:
#if TARGET_OS_IPHONE < 1
			//iOS currently doesnt have a backspace so we use delete.
			//This change should be future proof in the event apple changes the delete key to a backspace.
			return GEM_DELETE;
#endif
		case SDLK_BACKSPACE:
			return GEM_BACKSP;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			return GEM_RETURN;
		case SDLK_LALT:
		case SDLK_RALT:
			return GEM_ALT;
		case SDLK_TAB:
			return GEM_TAB;
		case SDLK_PAGEUP:
		case SDLK_KP9:
			return GEM_PGUP;
		case SDLK_PAGEDOWN:
		case SDLK_KP3:
			return GEM_PGDOWN;
		case SDLK_SCROLLOCK:
			return GEM_GRAB;
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
			return GEM_FUNCTIONX(1) + sym-SDLK_F1;
		default:
			break;
	}
	return sym;
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
			key = TranslateKeycode(sym);
			if (key != 0) {
				Event e = EvntManager->CreateKeyEvent(key, false, modstate);
				EvntManager->DispatchEvent(std::move(e));
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
			key = TranslateKeycode(sym);

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

			EvntManager->DispatchEvent(std::move(e));
			break;
		case SDL_MOUSEMOTION:
			e = EvntManager->CreateMouseMotionEvent(Point(event.motion.x, event.motion.y), modstate);
			EvntManager->DispatchEvent(std::move(e));
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			{
				EventButton btn = SDL_BUTTON(event.button.button);
				if (btn) {
					// it has been observed that multibutton mice can
					// result in 0 for some of their extra buttons
					// on at least some platforms
					bool down = (event.type == SDL_MOUSEBUTTONDOWN) ? true : false;
					Point p(event.button.x, event.button.y);
					e = EvntManager->CreateMouseBtnEvent(p, btn, down, modstate);
					EvntManager->DispatchEvent(std::move(e));
				}
			}
			break;
		case SDL_JOYAXISMOTION:
			{
				float pct = event.jaxis.value / float(sizeof(Sint16));
				bool xaxis = event.jaxis.axis % 2;
				// FIXME: I'm sure this delta needs to be scaled
				int delta = xaxis ? pct * screenSize.w : pct * screenSize.h;
				InputAxis axis = InputAxis(event.jaxis.axis);
				e = EvntManager->CreateControllerAxisEvent(axis, delta, pct);
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			{
				bool down = (event.type == SDL_JOYBUTTONDOWN) ? true : false;
				EventButton btn = EventButton(event.jbutton.button);
				e = EvntManager->CreateControllerButtonEvent(btn, down);
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
	}
	return GEM_OK;
}

Holder<Sprite2D> SDLVideoDriver::CreateSprite(const Region& rgn, void* pixels, const PixelFormat& fmt)
{
	if (fmt.RLE) {
#if SDL_VERSION_ATLEAST(1,3,0)
		// SDL2 should not allow RLE sprites so convert it
		void* newpixels = DecodeRLEData(static_cast<uint8_t*>(pixels), rgn.size, fmt.ColorKey);
		free(pixels);
		PixelFormat newfmt = fmt;
		newfmt.RLE = false;
		return Holder<Sprite2D>(new sprite_t(rgn, newpixels, newfmt));
#else
		return MakeHolder<Sprite2D>(rgn, pixels, fmt);
#endif
	}
	return Holder<Sprite2D>(new sprite_t(rgn, pixels, fmt));
}

void SDLVideoDriver::BlitSprite(const Holder<Sprite2D>& spr, const Region& src, Region dst,
								BlitFlags flags, Color tint)
{
	dst.x -= spr->Frame.x;
	dst.y -= spr->Frame.y;
	BlitSpriteClipped(spr, src, dst, flags, &tint);
}

void SDLVideoDriver::BlitGameSprite(const Holder<Sprite2D>& spr, const Point& p,
									BlitFlags flags, Color tint)
{
	Region srect(Point(0, 0), spr->Frame.size);
	Region drect = Region(p - spr->Frame.origin, spr->Frame.size);
	BlitSpriteClipped(spr, srect, drect, flags, &tint);
}

Region SDLVideoDriver::CurrentRenderClip() const
{
	Region bufferRegion(Point(), drawingBuffer->Size());
	return screenClip.Intersect(bufferRegion);
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
void SDLVideoDriver::DrawCircleImp(const Point& c, unsigned short r, const Color& color, BlitFlags flags)
{
	//Uses the Breshenham's Circle Algorithm
	int x = r;
	int y = 0;
	long xc = 1 - ( 2 * r );
	long yc = 1;
	long re = 0;

	std::vector<SDL_Point> points;

	while (x >= y) {
		SetPixel( drawingBuffer, c.x + x, c.y + y );
		SetPixel( drawingBuffer, c.x - x, c.y + y );
		SetPixel( drawingBuffer, c.x - x, c.y - y );
		SetPixel( drawingBuffer, c.x + x, c.y - y );
		SetPixel( drawingBuffer, c.x + y, c.y + x );
		SetPixel( drawingBuffer, c.x - y, c.y + x );
		SetPixel( drawingBuffer, c.x - y, c.y - x );
		SetPixel( drawingBuffer, c.x + y, c.y - x );

		y++;
		re += yc;
		yc += 2;

		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	DrawSDLPoints(points, reinterpret_cast<const SDL_Color&>(color), flags);
}

static double ellipseradius(unsigned short xr, unsigned short yr, double angle) {
	double one = (xr * sin(angle));
	double two = (yr * cos(angle));
	return sqrt(xr*xr*yr*yr / (one*one + two*two));
}

/** This functions Draws an Ellipse Segment */
void SDLVideoDriver::DrawEllipseSegmentImp(const Point& c, unsigned short xr,
	unsigned short yr, const Color& color, double anglefrom, double angleto, bool drawlines, BlitFlags flags)
{
	/* beware, dragons and clockwise angles be here! */
	double radiusfrom = ellipseradius(xr, yr, anglefrom);
	double radiusto = ellipseradius(xr, yr, angleto);
	int xfrom = round(radiusfrom * cos(anglefrom));
	int yfrom = round(radiusfrom * sin(anglefrom));
	int xto = round(radiusto * cos(angleto));
	int yto = round(radiusto * sin(angleto));
	int xrl = xr;
	int yrl = yr;

	if (drawlines) {
		DrawLine(c, Point(c.x + xfrom, c.y + yfrom), color, flags);
		DrawLine(c, Point(c.x + xto, c.y + yto), color, flags);
	}

	// *Attempt* to calculate the correct x/y boundaries.
	// TODO: this doesn't work very well - you can't actually bound many
	// arcs this way (imagine a segment with a small piece cut out).
	if (xfrom > xto) {
		int tmp = xfrom; xfrom = xto; xto = tmp;
	}
	if (yfrom > yto) {
		int tmp = yfrom; yfrom = yto; yto = tmp;
	}
	if (xfrom >= 0 && yto >= 0) xto = xr;
	if (xto <= 0 && yto >= 0) xfrom = -xr;
	if (yfrom >= 0 && xto >= 0) yto = yr;
	if (yto <= 0 && xto >= 0) yfrom = -yr;

	//Uses Bresenham's Ellipse Algorithm
	long xc, yc, ee, tas, tbs, sx, sy;
	int x, y;

	tas = 2 * xrl * xrl;
	tbs = 2 * yrl * yrl;
	x = xrl;
	y = 0;
	xc = yrl * yrl * (1 - (2 * xrl));
	yc = xrl * xrl;
	ee = 0;
	sx = tbs * xrl;
	sy = 0;

	std::vector<SDL_Point> points;

	while (sx >= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x + x, c.y + y );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x - x, c.y + y );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x - x, c.y - y );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x + x, c.y - y );
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
	y = yrl;
	xc = yrl * yrl;
	yc = xrl * xrl * (1 - (2 * yrl));
	ee = 0;
	sx = 0;
	sy = tas * yrl;

	while (sx <= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x + x, c.y + y );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x - x, c.y + y );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x - x, c.y - y );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x + x, c.y - y );
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

	DrawSDLPoints(points, reinterpret_cast<const SDL_Color&>(color), flags);
}

#undef SetPixel

void SDLVideoDriver::BlitSpriteClipped(const Holder<Sprite2D>& spr, Region src, const Region& dst, BlitFlags flags, const Color* tint)
{
#if SDL_VERSION_ATLEAST(1,3,0)
	// in SDL2 SDL_RenderCopyEx will flip the src rect internally if BlitFlags::MIRRORX or BlitFlags::MIRRORY is set
	// instead of doing this and then reversing it in that case only for SDL to reverse it yet again
	// lets just not worry about clipping on SDL2. the backends handle all of that for us unlike with SDL 1 where we
	// might walk off a memory buffer; we have no danger of that in SDL 2.
	// This fixes bizzare clipping issues when a "flipped" sprite is partially offscreen
	// we still will clip with screenClip later so no worries there
	// we still want to do the clipping for the purposes of avoiding calls to BlitSpriteNativeClipped where
	// expensive calls to RenderSpriteVersion may take place
	Region originalSrc = src;
#endif
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

	if (dclipped.size.IsInvalid() || src.size.IsInvalid()) {
		return;
	}

	assert(dclipped.w == src.w && dclipped.h == src.h);

#if SDL_VERSION_ATLEAST(1,3,0)
	dclipped = dst;
	src = originalSrc;
#endif

	if (spr->renderFlags&BlitFlags::MIRRORX) {
		flags ^= BlitFlags::MIRRORX;
	}

	if (spr->renderFlags&BlitFlags::MIRRORY) {
		flags ^= BlitFlags::MIRRORY;
	}
	
	if (!spr->HasTransparency()) {
		flags &= ~BlitFlags::BLENDED;
	}

	if (spr->Format().RLE) {
		BlitSpriteRLEClipped(spr, src, dclipped, flags, tint);
	} else {
		const sprite_t* native = static_cast<const sprite_t*>(spr.get ());
		BlitSpriteNativeClipped(native, src, dclipped, flags, reinterpret_cast<const SDL_Color*>(tint));
	}
}

BlitFlags SDLVideoDriver::RenderSpriteVersion(const SDLSurfaceSprite2D* spr, BlitFlags renderflags, const Color* tint) const
{
	SDLSurfaceSprite2D::version_t oldVersion = spr->GetVersion();
	SDLSurfaceSprite2D::version_t newVersion = renderflags;
	auto ret = (BlitFlags::GREY | BlitFlags::SEPIA) & newVersion;
	
	if (spr->Format().Bpp == 1) {
		if (tint) {
			assert(renderflags & (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD));
			uint64_t tintv = *reinterpret_cast<const uint32_t*>(tint);
			newVersion |= tintv << 32;
		}
		
		if (oldVersion != newVersion) {
			if (spr->IsPaletteStale()) {
				spr->Invalidate();
			}
			SDL_Palette* pal = static_cast<SDL_Palette*>(spr->NewVersion(newVersion));

			for (size_t i = 0; i < 256; ++i) {
				Color& dstc = reinterpret_cast<Color&>(pal->colors[i]);

				if (renderflags&BlitFlags::COLOR_MOD) {
					assert(tint);
					ShaderTint(*tint, dstc);
					ret |= BlitFlags::COLOR_MOD;
				}
				
				if (renderflags & BlitFlags::ALPHA_MOD) {
					assert(tint);
					dstc.a = tint->a;
					ret |= BlitFlags::ALPHA_MOD;
				}

				if (renderflags&BlitFlags::GREY) {
					ShaderGreyscale(dstc);
				} else if (renderflags&BlitFlags::SEPIA) {
					ShaderSepia(dstc);
				}
			}
		} else {
			ret |= (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD) & newVersion;
		}
	} else if (oldVersion != newVersion) {
		SDL_Surface* newV = (SDL_Surface*)spr->NewVersion(newVersion);
		SDL_LockSurface(newV);

		const Region& r = {0, 0, newV->w, newV->h};
		SDLPixelIterator beg = MakeSDLPixelIterator(newV, r);
		SDLPixelIterator end = SDLPixelIterator::end(beg);
		StaticAlphaIterator alpha(0xff);

		if (renderflags & BlitFlags::GREY) {
			RGBBlendingPipeline<SHADER::GREYSCALE, true> blender;
			Blit(beg, beg, end, alpha, blender);
		} else if (renderflags & BlitFlags::SEPIA) {
			RGBBlendingPipeline<SHADER::SEPIA, true> blender;
			Blit(beg, beg, end, alpha, blender);
		}
		SDL_UnlockSurface(newV);
	}
	return static_cast<BlitFlags>(ret);
}

// static class methods

bool SDLVideoDriver::SetSurfacePalette(SDL_Surface* surf, const SDL_Color* pal, int numcolors)
{
	if (pal) {
#if SDL_VERSION_ATLEAST(1,3,0)
		return SDL_SetPaletteColors(surf->format->palette, pal, 0, numcolors) == 0;
#else
		// const_cast because SDL doesnt alter this and we want our interface to be const correct
		return SDL_SetPalette( surf, SDL_LOGPAL, const_cast<SDL_Color*>(pal), 0, numcolors );
#endif
	}
	return -1;
}
