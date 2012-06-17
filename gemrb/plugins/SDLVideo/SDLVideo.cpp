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

#include "Sprite2D.h"

#include "TileRenderer.inl"
#include "SpriteRenderer.inl"

#include "AnimationFactory.h"
#include "Audio.h"
#include "Game.h" // for GetGlobalTint
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Polygon.h"
#include "SpriteCover.h"
#include "GUI/Console.h"
#include "GUI/Window.h"

#include <cmath>
#include <cassert>
#include <cstdio>

using namespace GemRB;

#if SDL_VERSION_ATLEAST(1,3,0)
#define SDL_SRCCOLORKEY SDL_TRUE
#define SDL_SRCALPHA 0
#define SDLK_SCROLLOCK SDLK_SCROLLLOCK
#define SDLK_KP2 SDLK_KP_2
#define SDLK_KP4 SDLK_KP_4
#define SDLK_KP6 SDLK_KP_6
#define SDLK_KP8 SDLK_KP_8
#endif

SDLVideoDriver::SDLVideoDriver(void)
{
	xCorr = 0;
	yCorr = 0;
	lastTime = 0;
	backBuf=NULL;
	extra=NULL;
	lastMouseDownTime = lastMouseMoveTime = GetTickCount();
	subtitlestrref = 0;
	subtitletext = NULL;
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	core->FreeString(subtitletext); //may be NULL

	if(backBuf) SDL_FreeSurface( backBuf );
	if(extra) SDL_FreeSurface( extra );

	SDL_Quit();

	// This sprite needs to have been freed earlier, because
	// all AnimationFactories and Sprites have already been
	// destructed before the video driver is freed.
	assert(Cursor[VID_CUR_DRAG] == NULL);
}

int SDLVideoDriver::Init(void)
{
	//print("[SDLVideoDriver]: Init...");
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1) {
		//print("[ERROR]");
		return GEM_ERROR;
	}
	//print("[OK]\n");
	SDL_ShowCursor( SDL_DISABLE );
	return GEM_OK;
}

int SDLVideoDriver::SwapBuffers(void)
{
	unsigned long time;
	time = GetTickCount();
	if (( time - lastTime ) < 33) {
		SDL_Delay( 33 - (time - lastTime) );
		time = GetTickCount();
	}
	lastTime = time;

	bool ConsolePopped = core->ConsolePopped;

	if (ConsolePopped) {
		core->DrawConsole();
	}

	return PollEvents();
}

int SDLVideoDriver::PollEvents()
{
	int ret = GEM_OK;
	SDL_Event currentEvent;

	while (ret != GEM_ERROR && SDL_PollEvent(&currentEvent)) {
		ret = ProcessEvent(currentEvent);
	}

	int x, y;
	if (ret == GEM_OK && !(MouseFlags & (MOUSE_DISABLED | MOUSE_GRAYED))
		&& lastTime>lastMouseDownTime
		&& SDL_GetMouseState(&x,&y)==SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		lastMouseDownTime=lastTime + EvntManager->GetRKDelay();
		if (!core->ConsolePopped)
			EvntManager->MouseUp( x, y, GEM_MB_ACTION, GetModState(SDL_GetModState()) );
	} 
	return ret;
}

int SDLVideoDriver::ProcessEvent(const SDL_Event & event)
{
	if (!EvntManager)
		return GEM_ERROR;

	unsigned char key = 0;
	int modstate = GetModState(event.key.keysym.mod);

	/* Loop until there are no events left on the queue */
	switch (event.type) {
			/* Process the appropriate event type */
		case SDL_QUIT:
			/* Handle a QUIT event */
			return GEM_ERROR;
			break;
		case SDL_KEYUP:
			switch(event.key.keysym.sym) {
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				case SDLK_SCROLLOCK:
					key = GEM_GRAB;
					break;
				case SDLK_f:
					if (modstate & GEM_MOD_CTRL) {
						ToggleFullscreenMode();
					}
					break;
				default:
					if (event.key.keysym.sym<256) {
						key=(unsigned char) event.key.keysym.sym;
					}
					break;
			}
			if (!core->ConsolePopped && ( key != 0 ))
				EvntManager->KeyRelease( key, modstate );
			break;
		case SDL_KEYDOWN:
			if ((event.key.keysym.sym == SDLK_SPACE) && modstate & GEM_MOD_CTRL) {
				core->PopupConsole();
				break;
			}
			key = (unsigned char) event.key.keysym.unicode;
			if (key < 32 || key == 127) {
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
						key = GEM_ESCAPE;
						break;
					case SDLK_END:
						key = GEM_END;
						break;
					case SDLK_HOME:
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
						key = GEM_PGUP;
						break;
					case SDLK_PAGEDOWN:
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
						key = GEM_FUNCTION1+event.key.keysym.sym-SDLK_F1;
						break;
					default:
						break;
				}
				if (core->ConsolePopped)
					core->console->OnSpecialKeyPress( key );
				else
					EvntManager->OnSpecialKeyPress( key );
			} else if (( key != 0 )) {
				if (core->ConsolePopped)
					core->console->OnKeyPress( key, modstate);
				else
					EvntManager->KeyPress( key, modstate);
			}
			break;
		case SDL_MOUSEMOTION:
			MouseMovement(event.motion.x, event.motion.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (MouseFlags & MOUSE_DISABLED)
				break;
			lastMouseDownTime=EvntManager->GetRKDelay();
			if (lastMouseDownTime != (unsigned long) ~0) {
				lastMouseDownTime += lastMouseDownTime+lastTime;
			}
			if (CursorIndex != VID_CUR_DRAG)
				CursorIndex = VID_CUR_DOWN;
			CursorPos.x = event.button.x; // - mouseAdjustX[CursorIndex];
			CursorPos.y = event.button.y; // - mouseAdjustY[CursorIndex];
			if (!core->ConsolePopped)
				EvntManager->MouseDown( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );
			break;
		case SDL_MOUSEBUTTONUP:
			if (CursorIndex != VID_CUR_DRAG)
				CursorIndex = VID_CUR_UP;
			CursorPos.x = event.button.x;
			CursorPos.y = event.button.y;
			if (!core->ConsolePopped)
				EvntManager->MouseUp( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );
			break;
	}
	return GEM_OK;
}

void SDLVideoDriver::InitSpriteCover(SpriteCover* sc, int flags)
{
	int i;
	sc->flags = flags;
	sc->pixels = new unsigned char[sc->Width * sc->Height];
	for (i = 0; i < sc->Width*sc->Height; ++i)
		sc->pixels[i] = 0;

}

void SDLVideoDriver::DestroySpriteCover(SpriteCover* sc)
{
	delete[] sc->pixels;
	sc->pixels = NULL;
}


// flags: 0 - never dither (full cover)
//	1 - dither if polygon wants it
//	2 - always dither
void SDLVideoDriver::AddPolygonToSpriteCover(SpriteCover* sc, Wall_Polygon* poly)
{

	// possible TODO: change the cover to use a set of intervals per line?
	// advantages: faster
	// disadvantages: makes the blitter much more complex

	int xoff = sc->worldx - sc->XPos;
	int yoff = sc->worldy - sc->YPos;

	std::list<Trapezoid>::iterator iter;
	for (iter = poly->trapezoids.begin(); iter != poly->trapezoids.end();
		++iter)
	{
		int y_top = iter->y1 - yoff; // inclusive
		int y_bot = iter->y2 - yoff; // exclusive

		if (y_top < 0) y_top = 0;
		if ( y_bot > sc->Height) y_bot = sc->Height;
		if (y_top >= y_bot) continue; // clipped

		int ledge = iter->left_edge;
		int redge = iter->right_edge;
		Point& a = poly->points[ledge];
		Point& b = poly->points[(ledge+1)%(poly->count)];
		Point& c = poly->points[redge];
		Point& d = poly->points[(redge+1)%(poly->count)];

		unsigned char* line = sc->pixels + (y_top)*sc->Width;
		for (int sy = y_top; sy < y_bot; ++sy) {
			int py = sy + yoff;

			// TODO: maybe use a 'real' line drawing algorithm to
			// compute these values faster.

			int lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
			int rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y) + 1;

			lt -= xoff;
			rt -= xoff;

			if (lt < 0) lt = 0;
			if (rt > sc->Width) rt = sc->Width;
			if (lt >= rt) { line += sc->Width; continue; } // clipped
			int dither;

			if (sc->flags == 1) {
				dither = poly->wall_flag & WF_DITHER;
			} else {
				dither = sc->flags;
			}
			if (dither) {
				unsigned char* pix = line + lt;
				unsigned char* end = line + rt;

				if ((lt + xoff + sy + yoff) % 2) pix++; // CHECKME: aliasing?
				for (; pix < end; pix += 2)
					*pix = 1;
			} else {
				// we hope memset is faster
				// condition: lt < rt is true
				memset (line+lt, 1, rt-lt);
			}
			line += sc->Width;
		}
	}
}


Sprite2D* SDLVideoDriver::CreateSprite(int w, int h, int bpp, ieDword rMask,
	ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	SDL_Surface* p = SDL_CreateRGBSurfaceFrom( pixels, w, h, bpp, w*( bpp / 8 ),
				rMask, gMask, bMask, aMask );
	if (cK) {
		SDL_SetColorKey( ( SDL_Surface * ) p, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			index );
	}
	return new Sprite2D(w, h, bpp, p, pixels);
}

Sprite2D* SDLVideoDriver::CreateSprite8(int w, int h, int bpp, void* pixels,
	void* palette, bool cK, int index)
{
	SDL_Surface* p = SDL_CreateRGBSurfaceFrom( pixels, w, h, 8, w, 0, 0, 0, 0 );
	int colorcount;
	if (bpp == 8) {
		colorcount = 256;
	} else {
		colorcount = 16;
	}
	SetSurfacePalette( ( SDL_Surface * ) p, ( SDL_Color * ) palette, colorcount );
	if (cK) {
		SDL_SetColorKey( ( SDL_Surface * ) p, SDL_SRCCOLORKEY, index );
	}
	return new Sprite2D(w, h, bpp, p, pixels);
}

Sprite2D* SDLVideoDriver::CreateSpriteBAM8(int w, int h, bool rle,
				const unsigned char* pixeldata,
				AnimationFactory* datasrc,
				Palette* palette, int transindex)
{
	Sprite2D_BAM_Internal* data = new Sprite2D_BAM_Internal;

	palette->IncRef();
	data->pal = palette;
	data->transindex = transindex;
	data->flip_hor = false;
	data->flip_ver = false;
	data->RLE = rle;
	data->source = datasrc;
	datasrc->IncDataRefCount();

	Sprite2D* spr = new Sprite2D(w, h, 8 /* FIXME!!!! */, data, pixeldata);
	spr->BAM = true;
	return spr;
}

void SDLVideoDriver::FreeSprite(Sprite2D*& spr)
{
	if(!spr)
		return;
	assert(spr->RefCount > 0);
	if (--spr->RefCount > 0) {
		spr = NULL;
		return;
	}

	if (spr->BAM) {
		if (spr->vptr) {
			Sprite2D_BAM_Internal* tmp = (Sprite2D_BAM_Internal*)spr->vptr;
			tmp->source->DecDataRefCount();
			delete tmp;
			// this delete also calls Release() on the used palette
		}
	} else {
		if (spr->vptr) {
			SDL_FreeSurface( ( SDL_Surface * ) spr->vptr );
		}
		free( (void*)spr->pixels );
	}
	delete spr;
	spr = NULL;
}

Sprite2D* SDLVideoDriver::DuplicateSprite(const Sprite2D* sprite)
{
	if (!sprite) return NULL;
	Sprite2D* dest = NULL;

	if (!sprite->BAM) {
		SDL_Surface* tmp = ( SDL_Surface* ) sprite->vptr;
		unsigned char *newpixels = (unsigned char*) malloc( sprite->Width*sprite->Height );

		SDL_LockSurface( tmp );
		memcpy(newpixels, sprite->pixels, sprite->Width*sprite->Height);
		dest = CreateSprite8(sprite->Width, sprite->Height, 8,
						newpixels, tmp->format->palette->colors, true, 0);
		SDL_UnlockSurface( tmp );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*) sprite->vptr;
		const unsigned char * rledata;

		rledata = (const unsigned char*) sprite->pixels;

		dest = CreateSpriteBAM8(sprite->Width, sprite->Height, data->RLE,
								rledata, data->source, data->pal,
								data->transindex);
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_ver = data->flip_ver;
		destdata->flip_hor = data->flip_hor;
	}

	return dest;
}

// Note: BlitSpriteRegion's clip region is shifted by Viewport.x/y if
// anchor is false. This is different from the other BlitSprite functions.
void SDLVideoDriver::BlitSpriteRegion(const Sprite2D* spr, const Region& size, int x,
	int y, bool anchor, const Region* clip)
{
	if (!spr->vptr) return;

	// Adjust the clipping rect to match the region to blit, and then
	// let BlitSprite handle the rest


	Region c;

	if (clip) {
		c = *clip;
		if (!anchor) {
			c.x -= Viewport.x;
			c.y -= Viewport.y;
		}
	} else {
		c.x = 0;
		c.y = 0;
		c.w = backBuf->w;
		c.h = backBuf->h;
	}


	Region dest;
	dest.x = x - spr->XPos;
	dest.y = y - spr->YPos;
	if (!anchor) {
		dest.x -= Viewport.x;
		dest.y -= Viewport.y;
	}

	dest.w = size.w;
	dest.h = size.h;

	// c is the clipping rect, dest the unclipped destination rect
	// We clip c to dest, and pass the resulting c to BlitSprite
	if (dest.x > c.x) {
		c.w -= (dest.x - c.x);
		c.x = dest.x;
	}
	if (dest.y > c.y) {
		c.h -= (dest.y - c.y);
		c.y = dest.y;
	}
	if (c.x+c.w > dest.x+dest.w) {
		c.w = dest.x+dest.w-c.x;
	}
	if (c.y+c.h > dest.y+dest.h) {
		c.h = dest.y+dest.h-c.y;
	}

	if (c.w <= 0 || c.h <= 0) return;

	BlitSprite(spr, x - size.x, y - size.y, anchor, &c);
}

void SDLVideoDriver::BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y, const Region* clip, unsigned int flags)
{
	if (spr->BAM) {
		Log(ERROR, "SDLVideo", "Tile blit not supported for this sprite");
		return;
	}

	x -= Viewport.x;
	y -= Viewport.y;

	int clipx, clipy, clipw, cliph;
	if (clip) {
		clipx = clip->x;
		clipy = clip->y;
		clipw = clip->w;
		cliph = clip->h;
	} else {
		clipx = 0;
		clipy = 0;
		clipw = backBuf->w;
		cliph = backBuf->h;
	}

	int rx = 0,ry = 0;
	int w = 64,h = 64;

	if (x < clipx) {
		rx += (clipx - x);
		w -= (clipx - x);
	}
	if (y < clipy) {
		ry += (clipy - y);
		h -= (clipy - y);
	}
	if (x + w > clipx + clipw)
		w -= (x + w - clipx - clipw);
	if (y + h > clipy + cliph)
		h -= (y + h - clipy - cliph);

	const Uint8* data = (Uint8*) (( SDL_Surface * ) spr->vptr)->pixels;
	const SDL_Color* pal = (( SDL_Surface * ) spr->vptr)->format->palette->colors;

	const Uint8* mask_data = NULL;
	Uint32 ck = 0;
	if (mask) {
		mask_data = (Uint8*) (( SDL_Surface * ) mask->vptr)->pixels;
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_GetColorKey(( SDL_Surface * ) mask->vptr, &ck);
#else
		ck = (( SDL_Surface * ) mask->vptr)->format->colorkey;
#endif
	}

	bool tint = false;
	Color tintcol = {255,255,255,0};

	if (core->GetGame()) {
		const Color* totint = core->GetGame()->GetGlobalTint();
		if (totint) {
			tintcol = *totint;
			tint = true;
		}
	}

#define DO_BLIT \
		if (backBuf->format->BytesPerPixel == 4) \
			BlitTile_internal<Uint32>(backBuf, x, y, rx, ry, w, h, data, pal, mask_data, ck, T, B); \
		else \
			BlitTile_internal<Uint16>(backBuf, x, y, rx, ry, w, h, data, pal, mask_data, ck, T, B); \

	if (flags & TILE_GREY) {

		if (flags & TILE_HALFTRANS) {
			TRBlender_HalfTrans B(backBuf->format);

			TRTinter_Grey T(tintcol);
			DO_BLIT
		} else {
			TRBlender_Opaque B(backBuf->format);

			TRTinter_Grey T(tintcol);
			DO_BLIT
		}

	} else if (flags & TILE_SEPIA) {

		if (flags & TILE_HALFTRANS) {
			TRBlender_HalfTrans B(backBuf->format);

			TRTinter_Sepia T(tintcol);
			DO_BLIT
		} else {
			TRBlender_Opaque B(backBuf->format);

			TRTinter_Sepia T(tintcol);
			DO_BLIT
		}

	} else {

		if (flags & TILE_HALFTRANS) {
			TRBlender_HalfTrans B(backBuf->format);

			if (tint) {
				TRTinter_Tint T(tintcol);
				DO_BLIT
			} else {
				TRTinter_NoTint T;
				DO_BLIT
			}
		} else {
			TRBlender_Opaque B(backBuf->format);

			if (tint) {
				TRTinter_Tint T(tintcol);
				DO_BLIT
			} else {
				TRTinter_NoTint T;
				DO_BLIT
			}
		}

	}

#undef DO_BLIT

}


void SDLVideoDriver::BlitSprite(const Sprite2D* spr, int x, int y, bool anchor,
	const Region* clip)
{
	if (!spr->vptr) return;

	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	if (!anchor) {
		tx -= Viewport.x;
		ty -= Viewport.y;
	}

	if (!spr->BAM) {
		//TODO: Add the destination surface and rect to the Blit Pipeline
		SDL_Rect drect;
		SDL_Rect t;
		SDL_Rect* srect = NULL;
		drect.x = tx;
		drect.y = ty;

		if (clip) {
			if (drect.x + spr->Width <= clip->x)
				return;
			if (drect.x >= clip->x + clip->w)
				return;

			if (drect.y + spr->Height <= clip->y)
				return;
			if (drect.y >= clip->y + clip->h)
				return;

			// determine srect/drect to clip to 'clip'
			t.x = 0;
			t.w = spr->Width;
			if (drect.x < clip->x) {
				t.x += clip->x - drect.x;
				t.w -= clip->x - drect.x;
				drect.x = clip->x;
			}
			if (drect.x + t.w > clip->x + clip->w) {
				t.w = clip->x + clip->w - drect.x;
			}

			t.y = 0;
			t.h = spr->Height;
			if (drect.y < clip->y) {
				t.y += clip->y - drect.y;
				t.h -= clip->y - drect.y;
				drect.y = clip->y;
			}
			if (drect.y + t.h > clip->y + clip->h) {
				t.h = clip->y + clip->h - drect.y;
			}
			srect = &t;

		}
		SDL_BlitSurface( ( SDL_Surface * ) spr->vptr, srect, backBuf, &drect );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

		const Uint8* srcdata = (const Uint8*)spr->pixels;

		Region finalclip = computeClipRect(backBuf, clip, tx, ty, spr->Width, spr->Height);

		if (finalclip.w <= 0 || finalclip.h <= 0)
			return;

		SDL_LockSurface(backBuf);

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_Regular<Uint32> shadow;

			if (data->pal->alpha) {
				SRTinter_NoTint<true> tinter;
				SRBlender_Alpha<Uint32> blender;

				BlitSpritePAL_dispatch<Uint32>(false, data->flip_hor,
				    backBuf, srcdata, data->pal->col, tx, ty, spr->Width, spr->Height, data->flip_ver, finalclip, (Uint8)data->transindex, 0, spr, 0, shadow, tinter, blender);
			} else {
				SRTinter_NoTint<false> tinter;
				SRBlender_NoAlpha<Uint32> blender;

				BlitSpritePAL_dispatch<Uint32>(false, data->flip_hor,
				    backBuf, srcdata, data->pal->col, tx, ty, spr->Width, spr->Height, data->flip_ver, finalclip, (Uint8)data->transindex, 0, spr, 0, shadow, tinter, blender);
			}

		} else {

			SRShadow_Regular<Uint16> shadow;

			if (data->pal->alpha) {
				SRTinter_NoTint<true> tinter;
				SRBlender_Alpha<Uint16> blender;

				BlitSpritePAL_dispatch<Uint16>(false, data->flip_hor,
				    backBuf, srcdata, data->pal->col, tx, ty, spr->Width, spr->Height, data->flip_ver, finalclip, (Uint8)data->transindex, 0, spr, 0, shadow, tinter, blender);
			} else {
				SRTinter_NoTint<false> tinter;
				SRBlender_NoAlpha<Uint16> blender;

				BlitSpritePAL_dispatch<Uint16>(false, data->flip_hor,
				    backBuf, srcdata, data->pal->col, tx, ty, spr->Width, spr->Height, data->flip_ver, finalclip, (Uint8)data->transindex, 0, spr, 0, shadow, tinter, blender);
			}

		}

		SDL_UnlockSurface(backBuf);
	}
}

//cannot make const reference from tint, it is modified locally
void SDLVideoDriver::BlitGameSprite(const Sprite2D* spr, int x, int y,
		unsigned int flags, Color tint,
		SpriteCover* cover, Palette *palette,
		const Region* clip, bool anchor)
{
	assert(spr);
	if (!spr->vptr) return;

	// WARNING: this pointer is only valid with BAM sprites
	Sprite2D_BAM_Internal* data = NULL;

	if (!spr->BAM) {
		SDL_Surface* surf = ( SDL_Surface * ) spr->vptr;
		if (surf->format->BytesPerPixel != 4 && surf->format->BytesPerPixel != 1) {
			// TODO...
			Log(ERROR, "SDLVideo", "BlitGameSprite not supported for this sprite");
			BlitSprite(spr, x, y, false, clip);
			return;
		}
	} else {
		data = (Sprite2D_BAM_Internal*)spr->vptr;
		if (!palette)
			palette = data->pal;
	}

	// global tint
	if (!anchor && core->GetGame()) {
		const Color *totint = core->GetGame()->GetGlobalTint();
		if (totint) {
			if (flags & BLIT_TINTED) {
				tint.r = (tint.r * totint->r) >> 8;
				tint.g = (tint.g * totint->g) >> 8;
				tint.b = (tint.b * totint->b) >> 8;
			} else {
				flags |= BLIT_TINTED;
				tint = *totint;
				tint.a = 255;
			}
		}
	}


	// implicit flags:
	const unsigned int blit_TINTALPHA =    0x40000000U;
	const unsigned int blit_PALETTEALPHA = 0x80000000U;

	// NB: blit_TINTALPHA isn't directly used or checked, but its presence
	// affects the special case checks below
	if ((flags & BLIT_TINTED) && tint.a != 255) flags |= blit_TINTALPHA;

	if (spr->BAM && palette->alpha) flags |= blit_PALETTEALPHA;

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


	const Uint8* srcdata = (const Uint8*)spr->pixels;
	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	if (!anchor) {
		tx -= Viewport.x;
		ty -= Viewport.y;
	}

	Region finalclip = computeClipRect(backBuf, clip, tx, ty, spr->Width, spr->Height);

	if (finalclip.w <= 0 || finalclip.h <= 0)
		return;

	SDL_LockSurface(backBuf);

	bool hflip = spr->BAM ? data->flip_hor : false;
	bool vflip = spr->BAM ? data->flip_ver : false;
	if (flags & BLIT_MIRRORX) hflip = !hflip;
	if (flags & BLIT_MIRRORY) vflip = !vflip;

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags & ~(BLIT_MIRRORX | BLIT_MIRRORY);
	if (remflags & BLIT_NOSHADOW) remflags &= ~BLIT_TRANSSHADOW;
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;


	if (spr->BAM && remflags == BLIT_TINTED) {

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_Regular<Uint32> shadow;
			SRTinter_Tint<false, false> tinter(tint);
			SRBlender_NoAlpha<Uint32> blender;

			BlitSpritePAL_dispatch<Uint32>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		} else {

			SRShadow_Regular<Uint16> shadow;
			SRTinter_Tint<false, false> tinter(tint);
			SRBlender_NoAlpha<Uint16> blender;

			BlitSpritePAL_dispatch<Uint16>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		}

	} else if (spr->BAM && remflags == (BLIT_TINTED | BLIT_TRANSSHADOW)) {

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_HalfTrans<Uint32> shadow(backBuf->format, palette->col[1]);
			SRTinter_Tint<false, false> tinter(tint);
			SRBlender_NoAlpha<Uint32> blender;

			BlitSpritePAL_dispatch<Uint32>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		} else {

			SRShadow_HalfTrans<Uint16> shadow(backBuf->format, palette->col[1]);
			SRTinter_Tint<false, false> tinter(tint);
			SRBlender_NoAlpha<Uint16> blender;

			BlitSpritePAL_dispatch<Uint16>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		}

	} else if (spr->BAM && remflags == (BLIT_TINTED | BLIT_NOSHADOW)) {

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_None<Uint32> shadow;
			SRTinter_Tint<false, false> tinter(tint);
			SRBlender_NoAlpha<Uint32> blender;

			BlitSpritePAL_dispatch<Uint32>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		} else {

			SRShadow_None<Uint16> shadow;
			SRTinter_Tint<false, false> tinter(tint);
			SRBlender_NoAlpha<Uint16> blender;

			BlitSpritePAL_dispatch<Uint16>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		}

	} else if (spr->BAM && remflags == BLIT_HALFTRANS) {

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_HalfTrans<Uint32> shadow(backBuf->format, palette->col[1]);
			SRTinter_NoTint<false> tinter;
			SRBlender_NoAlpha<Uint32> blender;

			BlitSpritePAL_dispatch<Uint32>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		} else {

			SRShadow_HalfTrans<Uint16> shadow(backBuf->format, palette->col[1]);
			SRTinter_NoTint<false> tinter;
			SRBlender_NoAlpha<Uint16> blender;

			BlitSpritePAL_dispatch<Uint16>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		}

	} else if (spr->BAM && remflags == 0) {

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_Regular<Uint32> shadow;
			SRTinter_NoTint<false> tinter;
			SRBlender_NoAlpha<Uint32> blender;

			BlitSpritePAL_dispatch<Uint32>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		} else {

			SRShadow_Regular<Uint16> shadow;
			SRTinter_NoTint<false> tinter;
			SRBlender_NoAlpha<Uint16> blender;

			BlitSpritePAL_dispatch<Uint16>(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);

		}

	} else if (spr->BAM) {
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

		if (backBuf->format->BytesPerPixel == 4) {

			SRShadow_Flags<Uint32> shadow; // for halftrans, noshadow, transshadow
			SRBlender_Alpha<Uint32> blender;
			if (remflags & blit_PALETTEALPHA) {
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<true> tinter(tint);

					BlitSpritePAL_dispatch<Uint32>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<true> tinter;

					BlitSpritePAL_dispatch<Uint32>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				}
			} else {
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<false> tinter(tint);

					BlitSpritePAL_dispatch<Uint32>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<false> tinter;

					BlitSpritePAL_dispatch<Uint32>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				}

			}

		} else {

			SRShadow_Flags<Uint16> shadow; // for halftrans, noshadow, transshadow
			SRBlender_Alpha<Uint16> blender;
			if (remflags & blit_PALETTEALPHA) {
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<true> tinter(tint);

					BlitSpritePAL_dispatch<Uint16>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<true> tinter;

					BlitSpritePAL_dispatch<Uint16>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				}
			} else {
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<false> tinter(tint);

					BlitSpritePAL_dispatch<Uint16>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<false> tinter;

					BlitSpritePAL_dispatch<Uint16>(cover, hflip,
					    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)data->transindex, cover, spr, remflags, shadow, tinter, blender);
				}

			}

		}

	} else {
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

		if (remflags & BLIT_HALFTRANS) {
			// handle halftrans with 50% alpha tinting
			if (!(remflags & BLIT_TINTED)) {
				tint.r = tint.g = tint.b = tint.a = 255;
				remflags |= BLIT_TINTED;
			}
			tint.a >>= 1;

		}

		const Color *col = 0;

		SDL_Surface* surf = ( SDL_Surface * ) spr->vptr;
		if (surf->format->BytesPerPixel == 1) {

			if (remflags & BLIT_TINTED)
				tint.a = 255;

			// NB: GemRB::Color has exactly the same layout as SDL_Color
			if (!palette)
				col = (const Color*)surf->format->palette->colors;
			else
				col = palette->col;

			const Uint8 *data = (const Uint8*)spr->pixels;

			if (backBuf->format->BytesPerPixel == 4) {

				SRBlender_Alpha<Uint32> blender;
				SRShadow_NOP<Uint32> shadow;
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<false> tinter(tint);

					BlitSpritePAL_dispatch<Uint32>(cover, hflip,
					    backBuf, data, col, tx, ty, spr->Width, spr->Height, vflip, finalclip, -1, cover, spr, remflags, shadow, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<false> tinter;

					BlitSpritePAL_dispatch<Uint32>(cover, hflip,
					    backBuf, data, col, tx, ty, spr->Width, spr->Height, vflip, finalclip, -1, cover, spr, remflags, shadow, tinter, blender);
				}

			} else {

				SRBlender_Alpha<Uint16> blender;
				SRShadow_NOP<Uint16> shadow;
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<false> tinter(tint);

					BlitSpritePAL_dispatch<Uint16>(cover, hflip,
					    backBuf, data, col, tx, ty, spr->Width, spr->Height, vflip, finalclip, -1, cover, spr, remflags, shadow, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<false> tinter;

					BlitSpritePAL_dispatch<Uint16>(cover, hflip,
					    backBuf, data, col, tx, ty, spr->Width, spr->Height, vflip, finalclip, -1, cover, spr, remflags, shadow, tinter, blender);
				}

			}

		} else {

			const Uint32 *data = (const Uint32*)spr->pixels;
			if (backBuf->format->BytesPerPixel == 4) {

				SRBlender_Alpha<Uint32> blender;
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<true> tinter(tint);

					BlitSpriteRGB_dispatch<Uint32>(cover, hflip,
					    backBuf, data, tx, ty, spr->Width, spr->Height, vflip, finalclip, cover, spr, remflags, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<true> tinter;

					BlitSpriteRGB_dispatch<Uint32>(cover, hflip,
					    backBuf, data, tx, ty, spr->Width, spr->Height, vflip, finalclip, cover, spr, remflags, tinter, blender);
				}

			} else {

				SRBlender_Alpha<Uint16> blender;
				if (remflags & BLIT_TINTED) {
					SRTinter_Flags<true> tinter(tint);

					BlitSpriteRGB_dispatch<Uint16>(cover, hflip,
					    backBuf, data, tx, ty, spr->Width, spr->Height, vflip, finalclip, cover, spr, remflags, tinter, blender);
				} else {
					SRTinter_FlagsNoTint<true> tinter;

					BlitSpriteRGB_dispatch<Uint16>(cover, hflip,
					    backBuf, data, tx, ty, spr->Width, spr->Height, vflip, finalclip, cover, spr, remflags, tinter, blender);
				}

			}
		}

	}

	SDL_UnlockSurface(backBuf);

}

Sprite2D* SDLVideoDriver::GetScreenshot( Region r )
{
	unsigned int Width = r.w ? r.w : disp->w;
	unsigned int Height = r.h ? r.h : disp->h;
	SDL_Rect src = {(Sint16)r.x, (Sint16)r.y, (Uint16)r.w, (Uint16)r.h};


	SDL_Surface* surf = SDL_CreateRGBSurface( SDL_SWSURFACE, Width, Height, 24,
				0xFF0000, 0x00FF00, 0x0000FF, 0x000000 );
	SDL_BlitSurface( backBuf, (r.w && r.h) ? &src : NULL, surf, NULL);
	void* pixels = malloc( Width * Height * 3 );
	for (unsigned int y = 0; y < Height; y++)
		memcpy( (char*)pixels+(Width * y * 3), (char*)surf->pixels+(surf->pitch * y), Width * 3 );
	//freeing up temporary surface as we copied its pixels
	Sprite2D* screenshot = CreateSprite( Width, Height, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, pixels, false, 0 );
	SDL_FreeSurface(surf);
	return screenshot;
}

/** No descriptions */
void SDLVideoDriver::SetPalette(void *data, Palette* pal)
{
	SDL_Surface* sur = ( SDL_Surface* ) data;
	SetSurfacePalette(sur, ( SDL_Color * ) pal->col, 256);
}

void SDLVideoDriver::ConvertToVideoFormat(Sprite2D* sprite)
{
	if (!sprite->BAM) {
		SDL_Surface* ss = ( SDL_Surface* ) sprite->vptr;
		if (ss->format->Amask != 0) //Surface already converted
		{
			return;
		}
		SDL_Surface* ns = SDL_ConvertSurface( ss, disp->format, 0);
		if (ns == NULL) {
			return;
		}
		SDL_FreeSurface( ss );
		free( (void*)sprite->pixels );
		sprite->pixels = NULL;
		sprite->vptr = ns;
	}
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDLVideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill, bool clipped)
{
	SDL_Rect drect = {
		(Sint16)rgn.x, (Sint16)rgn.y, (Uint16)rgn.w, (Uint16)rgn.h
	};
	if (fill) {
		if ( SDL_ALPHA_TRANSPARENT == color.a ) {
			return;
		} else if ( SDL_ALPHA_OPAQUE == color.a ) {
			long val = SDL_MapRGBA( backBuf->format, color.r, color.g, color.b, color.a );
			SDL_FillRect( backBuf, &drect, val );
		} else {
			SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
			SDL_Color c;
			c.r = color.r;
			c.b = color.b;
			c.g = color.g;
			SetSurfacePalette( rectsurf, &c, 1 );
			SetSurfaceAlpha(rectsurf, color.a);
			SDL_BlitSurface( rectsurf, NULL, backBuf, &drect );
			SDL_FreeSurface( rectsurf );
		}
	} else {
		DrawHLine( rgn.x, rgn.y, rgn.x + rgn.w - 1, color, clipped );
		DrawVLine( rgn.x, rgn.y, rgn.y + rgn.h - 1, color, clipped );
		DrawHLine( rgn.x, rgn.y + rgn.h - 1, rgn.x + rgn.w - 1, color, clipped );
		DrawVLine( rgn.x + rgn.w - 1, rgn.y, rgn.y + rgn.h - 1, color, clipped );
	}
}

/** This function Draws a clipped sprite */
void SDLVideoDriver::DrawRectSprite(const Region& rgn, const Color& color, const Sprite2D* sprite)
{
	if (sprite->BAM) {
		Log(ERROR, "SDLVideo", "DrawRectSprite not supported for this sprite");
		return;
	}

	SDL_Surface* surf = ( SDL_Surface* ) sprite->vptr;
	SDL_Rect drect = {
		(Sint16)rgn.x, (Sint16)rgn.y, (Uint16)rgn.w, (Uint16)rgn.h
	};
	if ( SDL_ALPHA_TRANSPARENT == color.a ) {
		return;
	} else if ( SDL_ALPHA_OPAQUE == color.a ) {
		long val = SDL_MapRGBA( surf->format, color.r, color.g, color.b, color.a );
		SDL_FillRect( surf, &drect, val );
	} else {
		SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
		SDL_Color c;
		c.r = color.r;
		c.b = color.b;
		c.g = color.g;
		SetSurfacePalette( rectsurf, &c, 1 );
		SetSurfaceAlpha(rectsurf, color.a);
		SDL_BlitSurface( rectsurf, NULL, surf, &drect );
		SDL_FreeSurface( rectsurf );
	}
}

inline void ReadPixel(long &val, Pixel* pixels , int BytesPerPixel) {
	if (BytesPerPixel == 1) {
		val = *pixels;
	} else if (BytesPerPixel == 2) {
		val = *(Uint16 *)pixels;
	} else if (BytesPerPixel == 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		val = pixels[0] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[2] << 16);
#else
		val = pixels[2] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[0] << 16);
#endif
	} else if (BytesPerPixel == 4) {
		val = *(Uint32 *)pixels;
	}
}

inline void WritePixel(const long val, Pixel* pixels, int BytesPerPixel) {
	if (BytesPerPixel == 1) {
		*pixels = (unsigned char)val;
	} else if (BytesPerPixel == 2) {
		*(Uint16 *)pixels = (Uint16)val;
	} else if (BytesPerPixel == 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		pixels[0] = val & 0xff;
		pixels[1] = (val >> 8) & 0xff;
		pixels[2] = (val >> 16) & 0xff;
#else
		pixels[2] = val & 0xff;
		pixels[1] = (val >> 8) & 0xff;
		pixels[0] = (val >> 16) & 0xff;
#endif
	} else if (BytesPerPixel == 4) {
		*(Uint32 *)pixels = val;
	}
}

void SDLVideoDriver::SetPixel(short x, short y, const Color& color, bool clipped)
{
	//print("x: %d; y: %d; XC: %d; YC: %d, VX: %d, VY: %d, VW: %d, VH: %d", x, y, xCorr, yCorr, Viewport.x, Viewport.y, Viewport.w, Viewport.h);
	if (clipped) {
		x += xCorr;
		y += yCorr;
		if (( x >= ( xCorr + Viewport.w ) ) || ( y >= ( yCorr + Viewport.h ) )) {
			return;
		}
		if (( x < xCorr ) || ( y < yCorr )) {
			return;
		}
	} else {
		if (( x >= disp->w ) || ( y >= disp->h )) {
			return;
		}
		if (( x < 0 ) || ( y < 0 )) {
			return;
		}
	}

	unsigned char * pixels = ( ( unsigned char * ) backBuf->pixels ) +
		( ( y * disp->w + x) * disp->format->BytesPerPixel );

	long val = SDL_MapRGBA( backBuf->format, color.r, color.g, color.b, color.a );
	SDL_LockSurface( backBuf );
	WritePixel(val, pixels, backBuf->format->BytesPerPixel);
	SDL_UnlockSurface( backBuf );
}

void SDLVideoDriver::GetPixel(short x, short y, Color& c)
{
	SDL_LockSurface( backBuf );
	unsigned char * pixels = ( ( unsigned char * ) backBuf->pixels ) +
		( ( y * disp->w + x) * disp->format->BytesPerPixel );
	long val = 0;
	ReadPixel(val, pixels, backBuf->format->BytesPerPixel);
	SDL_UnlockSurface( backBuf );

	SDL_GetRGBA( val, backBuf->format, (Uint8 *) &c.r, (Uint8 *) &c.g, (Uint8 *) &c.b, (Uint8 *) &c.a );
}

void SDLVideoDriver::GetPixel(void *vptr, unsigned short x, unsigned short y, Color &c)
{
	SDL_Surface *surf = (SDL_Surface*)(vptr);

	SDL_LockSurface( surf );
	unsigned char * pixels = ( ( unsigned char * ) surf->pixels ) +
		( ( y * surf->w + x) * surf->format->BytesPerPixel );
	long val = 0;
	ReadPixel(val, pixels, surf->format->BytesPerPixel);
	SDL_UnlockSurface( surf );

	SDL_GetRGBA( val, surf->format, (Uint8 *) &c.r, (Uint8 *) &c.g, (Uint8 *) &c.b, (Uint8 *) &c.a );
}

long SDLVideoDriver::GetPixel(void *vptr, unsigned short x, unsigned short y)
{
	SDL_Surface *surf = (SDL_Surface*)(vptr);

	SDL_LockSurface( surf );
	unsigned char * pixels = ( ( unsigned char * ) surf->pixels ) +
		( ( y * surf->w + x) * surf->format->BytesPerPixel );
	long val = 0;
	ReadPixel(val, pixels, surf->format->BytesPerPixel);
	SDL_UnlockSurface( surf );

	return val;
}

/*
 * Draws horizontal line. When clipped=true, it draws the line relative
 * to Area origin and clips it by Area viewport borders,
 * else it draws relative to screen origin and ignores the vieport
 */
void SDLVideoDriver::DrawHLine(short x1, short y, short x2, const Color& color, bool clipped)
{
	if (x1 > x2) {
		short tmpx = x1;
		x1 = x2;
		x2 = tmpx;
	}
	if (clipped) {
		x1 -= Viewport.x;
		y -= Viewport.y;
		x2 -= Viewport.x;
	}
	for (; x1 <= x2 ; x1++ )
		SetPixel( x1, y, color, clipped );
}

/*
 * Draws vertical line. When clipped=true, it draws the line relative
 * to Area origin and clips it by Area viewport borders,
 * else it draws relative to screen origin and ignores the vieport
 */
void SDLVideoDriver::DrawVLine(short x, short y1, short y2, const Color& color, bool clipped)
{
	if (y1 > y2) {
		short tmpy = y1;
		y1 = y2;
		y2 = tmpy;
	}
	if (clipped) {
		x -= Viewport.x;
		y1 -= Viewport.y;
		y2 -= Viewport.y;
	}

	for (; y1 <= y2 ; y1++ )
		SetPixel( x, y1, color, clipped );
}

void SDLVideoDriver::DrawLine(short x1, short y1, short x2, short y2,
	const Color& color, bool clipped)
{
	if (clipped) {
		x1 -= Viewport.x;
		x2 -= Viewport.x;
		y1 -= Viewport.y;
		y2 -= Viewport.y;
	}
	bool yLonger = false;
	int shortLen = y2 - y1;
	int longLen = x2 - x1;
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

	if (yLonger) {
		if (longLen > 0) {
			longLen += y1;
			for (int j = 0x8000 + ( x1 << 16 ); y1 <= longLen; ++y1) {
				SetPixel( j >> 16, y1, color, clipped );
				j += decInc;
			}
			return;
		}
		longLen += y1;
		for (int j = 0x8000 + ( x1 << 16 ); y1 >= longLen; --y1) {
			SetPixel( j >> 16, y1, color, clipped );
			j -= decInc;
		}
		return;
	}

	if (longLen > 0) {
		longLen += x1;
		for (int j = 0x8000 + ( y1 << 16 ); x1 <= longLen; ++x1) {
			SetPixel( x1, j >> 16, color, clipped );
			j += decInc;
		}
		return;
	}
	longLen += x1;
	for (int j = 0x8000 + ( y1 << 16 ); x1 >= longLen; --x1) {
		SetPixel( x1, j >> 16, color, clipped );
		j -= decInc;
	}
}
/** This functions Draws a Circle */
void SDLVideoDriver::DrawCircle(short cx, short cy, unsigned short r,
	const Color& color, bool clipped)
{
	//Uses the Breshenham's Circle Algorithm
	long x, y, xc, yc, re;

	x = r;
	y = 0;
	xc = 1 - ( 2 * r );
	yc = 1;
	re = 0;

	if (SDL_MUSTLOCK( disp )) {
		SDL_LockSurface( disp );
	}
	while (x >= y) {
		SetPixel( cx + ( short ) x, cy + ( short ) y, color, clipped );
		SetPixel( cx - ( short ) x, cy + ( short ) y, color, clipped );
		SetPixel( cx - ( short ) x, cy - ( short ) y, color, clipped );
		SetPixel( cx + ( short ) x, cy - ( short ) y, color, clipped );
		SetPixel( cx + ( short ) y, cy + ( short ) x, color, clipped );
		SetPixel( cx - ( short ) y, cy + ( short ) x, color, clipped );
		SetPixel( cx - ( short ) y, cy - ( short ) x, color, clipped );
		SetPixel( cx + ( short ) y, cy - ( short ) x, color, clipped );

		y++;
		re += yc;
		yc += 2;

		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}
	if (SDL_MUSTLOCK( disp )) {
		SDL_UnlockSurface( disp );
	}
}

static double ellipseradius(unsigned short xr, unsigned short yr, double angle) {
	double one = (xr * sin(angle));
	double two = (yr * cos(angle));
	return sqrt(xr*xr*yr*yr / (one*one + two*two));
}

/** This functions Draws an Ellipse Segment */
void SDLVideoDriver::DrawEllipseSegment(short cx, short cy, unsigned short xr,
	unsigned short yr, const Color& color, double anglefrom, double angleto, bool drawlines, bool clipped)
{
	/* beware, dragons and clockwise angles be here! */
	double radiusfrom = ellipseradius(xr, yr, anglefrom);
	double radiusto = ellipseradius(xr, yr, angleto);
	long xfrom = (long)round(radiusfrom * cos(anglefrom));
	long yfrom = (long)round(radiusfrom * sin(anglefrom));
	long xto = (long)round(radiusto * cos(angleto));
	long yto = (long)round(radiusto * sin(angleto));

	if (drawlines) {
		// TODO: DrawLine's clipping code works differently to the clipping
		// here so we add Viewport.x/Viewport.y as a hack for now
		DrawLine(cx + Viewport.x, cy + Viewport.y,
			cx + xfrom + Viewport.x, cy + yfrom + Viewport.y, color, clipped);
		DrawLine(cx + Viewport.x, cy + Viewport.y,
			cx + xto + Viewport.x, cy + yto + Viewport.y, color, clipped);
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

	if (SDL_MUSTLOCK( disp )) {
		SDL_LockSurface( disp );
	}
	tas = 2 * xr * xr;
	tbs = 2 * yr * yr;
	x = xr;
	y = 0;
	xc = yr * yr * ( 1 - ( 2 * xr ) );
	yc = xr * xr;
	ee = 0;
	sx = tbs * xr;
	sy = 0;

	while (sx >= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( cx + ( short ) x, cy + ( short ) y, color, clipped );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( cx - ( short ) x, cy + ( short ) y, color, clipped );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( cx - ( short ) x, cy - ( short ) y, color, clipped );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( cx + ( short ) x, cy - ( short ) y, color, clipped );
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
			SetPixel( cx + ( short ) x, cy + ( short ) y, color, clipped );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( cx - ( short ) x, cy + ( short ) y, color, clipped );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( cx - ( short ) x, cy - ( short ) y, color, clipped );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( cx + ( short ) x, cy - ( short ) y, color, clipped );
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
	if (SDL_MUSTLOCK( disp )) {
		SDL_UnlockSurface( disp );
	}
}


/** This functions Draws an Ellipse */
void SDLVideoDriver::DrawEllipse(short cx, short cy, unsigned short xr,
	unsigned short yr, const Color& color, bool clipped)
{
	//Uses Bresenham's Ellipse Algorithm
	long x, y, xc, yc, ee, tas, tbs, sx, sy;

	if (SDL_MUSTLOCK( disp )) {
		SDL_LockSurface( disp );
	}
	tas = 2 * xr * xr;
	tbs = 2 * yr * yr;
	x = xr;
	y = 0;
	xc = yr * yr * ( 1 - ( 2 * xr ) );
	yc = xr * xr;
	ee = 0;
	sx = tbs * xr;
	sy = 0;

	while (sx >= sy) {
		SetPixel( cx + ( short ) x, cy + ( short ) y, color, clipped );
		SetPixel( cx - ( short ) x, cy + ( short ) y, color, clipped );
		SetPixel( cx - ( short ) x, cy - ( short ) y, color, clipped );
		SetPixel( cx + ( short ) x, cy - ( short ) y, color, clipped );
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
		SetPixel( cx + ( short ) x, cy + ( short ) y, color, clipped );
		SetPixel( cx - ( short ) x, cy + ( short ) y, color, clipped );
		SetPixel( cx - ( short ) x, cy - ( short ) y, color, clipped );
		SetPixel( cx + ( short ) x, cy - ( short ) y, color, clipped );
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
	if (SDL_MUSTLOCK( disp )) {
		SDL_UnlockSurface( disp );
	}
}

void SDLVideoDriver::DrawPolyline(Gem_Polygon* poly, const Color& color, bool fill)
{
	if (!poly->count) {
		return;
	}

	if (poly->BBox.x > Viewport.x + Viewport.w) return;
	if (poly->BBox.y > Viewport.y + Viewport.h) return;
	if (poly->BBox.x + poly->BBox.w < Viewport.x) return;
	if (poly->BBox.y + poly->BBox.h < Viewport.y) return;

	if (fill) {
		Uint32 alphacol32 = SDL_MapRGBA(backBuf->format, color.r/2, color.g/2, color.b/2, 0);
		Uint16 alphacol16 = (Uint16)alphacol32;

		// color mask for doing a 50/50 alpha blit
		Uint32 mask32 = (backBuf->format->Rmask >> 1) & backBuf->format->Rmask;
		mask32 |= (backBuf->format->Gmask >> 1) & backBuf->format->Gmask;
		mask32 |= (backBuf->format->Bmask >> 1) & backBuf->format->Bmask;

		Uint16 mask16 = (Uint16)mask32;

		SDL_LockSurface(backBuf);
		std::list<Trapezoid>::iterator iter;
		for (iter = poly->trapezoids.begin(); iter != poly->trapezoids.end();
			++iter)
		{
			int y_top = iter->y1 - Viewport.y; // inclusive
			int y_bot = iter->y2 - Viewport.y; // exclusive

			if (y_top < 0) y_top = 0;
			if (y_bot > Viewport.h) y_bot = Viewport.h;
			if (y_top >= y_bot) continue; // clipped

			int ledge = iter->left_edge;
			int redge = iter->right_edge;
			Point& a = poly->points[ledge];
			Point& b = poly->points[(ledge+1)%(poly->count)];
			Point& c = poly->points[redge];
			Point& d = poly->points[(redge+1)%(poly->count)];

                       Pixel* line = (Pixel*)(backBuf->pixels) + (y_top+yCorr)*backBuf->pitch;

			for (int y = y_top; y < y_bot; ++y) {
				int py = y + Viewport.y;

				// TODO: maybe use a 'real' line drawing algorithm to
				// compute these values faster.

				int lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
				int rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y) + 1;

				lt -= Viewport.x;
				rt -= Viewport.x;

				if (lt < 0) lt = 0;
				if (rt > Viewport.w) rt = Viewport.w;
				if (lt >= rt) { line += backBuf->pitch; continue; } // clipped


				// Draw a 50% alpha line from (y,lt) to (y,rt)

				if (backBuf->format->BytesPerPixel == 2) {
					Uint16* pix = (Uint16*)line + lt + xCorr;
					Uint16* end = pix + (rt - lt);
					for (; pix < end; pix++)
						*pix = ((*pix >> 1)&mask16) + alphacol16;
				} else if (backBuf->format->BytesPerPixel == 4) {
					Uint32* pix = (Uint32*)line + lt + xCorr;
					Uint32* end = pix + (rt - lt);
					for (; pix < end; pix++)
						*pix = ((*pix >> 1)&mask32) + alphacol32;
				} else {
					assert(false);
				}
				line += backBuf->pitch;
			}
		}
		SDL_UnlockSurface(backBuf);
	}

	short lastX = poly->points[0]. x, lastY = poly->points[0].y;
	unsigned int i;

	for (i = 1; i < poly->count; i++) {
		DrawLine( lastX, lastY, poly->points[i].x, poly->points[i].y, color, true );
		lastX = poly->points[i].x;
		lastY = poly->points[i].y;
	}
	DrawLine( lastX, lastY, poly->points[0].x, poly->points[0].y, color, true );

	return;
}

/** Send a Quit Signal to the Event Queue */
bool SDLVideoDriver::Quit()
{
	SDL_Event evtQuit = SDL_Event();

	evtQuit.type = SDL_QUIT;
	if (SDL_PushEvent( &evtQuit ) == -1) {
		return false;
	}
	return true;
}

Palette* SDLVideoDriver::GetPalette(void *vptr)
{
	SDL_Surface* s = ( SDL_Surface* ) vptr;
	if (s->format->BitsPerPixel != 8) {
		return NULL;
	}
	Palette* pal = new Palette();
	for (int i = 0; i < s->format->palette->ncolors; i++) {
		pal->col[i].r = s->format->palette->colors[i].r;
		pal->col[i].g = s->format->palette->colors[i].g;
		pal->col[i].b = s->format->palette->colors[i].b;
	}
	return pal;
}

// Flips given sprite vertically (up-down). If MirrorAnchor=true,
// flips its anchor (i.e. origin//base point) as well
// returns new sprite

Sprite2D *SDLVideoDriver::MirrorSpriteVertical(const Sprite2D* sprite, bool MirrorAnchor)
{
	if (!sprite || !sprite->vptr)
		return NULL;

	Sprite2D* dest = DuplicateSprite(sprite);

	if (!sprite->BAM) {
		for (int x = 0; x < dest->Width; x++) {
			unsigned char * dst = ( unsigned char * ) dest->pixels + x;
			unsigned char * src = dst + ( dest->Height - 1 ) * dest->Width;
			for (int y = 0; y < dest->Height / 2; y++) {
				unsigned char swp = *dst;
				*dst = *src;
				*src = swp;
				dst += dest->Width;
				src -= dest->Width;
			}
		}
	} else {
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_ver = !destdata->flip_ver;
	}

	dest->XPos = sprite->XPos;
	if (MirrorAnchor)
		dest->YPos = sprite->Height - sprite->YPos;
	else
		dest->YPos = sprite->YPos;

	return dest;
}

// Flips given sprite horizontally (left-right). If MirrorAnchor=true,
//   flips its anchor (i.e. origin//base point) as well
Sprite2D *SDLVideoDriver::MirrorSpriteHorizontal(const Sprite2D* sprite, bool MirrorAnchor)
{
	if (!sprite || !sprite->vptr)
		return NULL;

	Sprite2D* dest = DuplicateSprite(sprite);

	if (!sprite->BAM) {
		for (int y = 0; y < dest->Height; y++) {
			unsigned char * dst = (unsigned char *) dest->pixels + ( y * dest->Width );
			unsigned char * src = dst + dest->Width - 1;
			for (int x = 0; x < dest->Width / 2; x++) {
				unsigned char swp=*dst;
				*dst++ = *src;
				*src-- = swp;
			}
		}
	} else {
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_hor = !destdata->flip_hor;
	}

	if (MirrorAnchor)
		dest->XPos = sprite->Width - sprite->XPos;
	else
		dest->XPos = sprite->XPos;
	dest->YPos = sprite->YPos;

	return dest;
}

void SDLVideoDriver::SetFadeColor(int r, int g, int b)
{
	if (r>255) r=255;
	else if(r<0) r=0;
	fadeColor.r=r;
	if (g>255) g=255;
	else if(g<0) g=0;
	fadeColor.g=g;
	if (b>255) b=255;
	else if(b<0) b=0;
	fadeColor.b=b;
	long val = SDL_MapRGBA( extra->format, fadeColor.r, fadeColor.g, fadeColor.b, fadeColor.a );
	SDL_FillRect( extra, NULL, val );
}

void SDLVideoDriver::SetFadePercent(int percent)
{
	if (percent>100) percent = 100;
	else if (percent<0) percent = 0;
	fadeColor.a = (255 * percent ) / 100;
}

void SDLVideoDriver::SetClipRect(const Region* clip)
{
	if (clip) {
		SDL_Rect tmp;
		tmp.x = clip->x;
		tmp.y = clip->y;
		tmp.w = clip->w;
		tmp.h = clip->h;
		SDL_SetClipRect( backBuf, &tmp );
	} else {
		SDL_SetClipRect( backBuf, NULL );
	}
}

void SDLVideoDriver::GetClipRect(Region& clip)
{
	SDL_Rect tmp;
	SDL_GetClipRect( backBuf, &tmp );

	clip.x = tmp.x;
	clip.y = tmp.y;
	clip.w = tmp.w;
	clip.h = tmp.h;
}


void SDLVideoDriver::GetMousePos(int &x, int &y)
{
	x=CursorPos.x;
	y=CursorPos.y;
}

void SDLVideoDriver::MouseMovement(int x, int y)
{
	lastMouseMoveTime = GetTickCount();
	if (MouseFlags&MOUSE_DISABLED)
		return;
	CursorPos.x = x; // - mouseAdjustX[CursorIndex];
	CursorPos.y = y; // - mouseAdjustY[CursorIndex];
	if (EvntManager)
		EvntManager->MouseMove(x, y);
}

void SDLVideoDriver::ClickMouse(unsigned int button)
{
	MouseClickEvent(SDL_MOUSEBUTTONDOWN, (Uint8) button);
	MouseClickEvent(SDL_MOUSEBUTTONUP, (Uint8) button);
	if (button&GEM_MB_DOUBLECLICK) {
		MouseClickEvent(SDL_MOUSEBUTTONDOWN, (Uint8) button);
		MouseClickEvent(SDL_MOUSEBUTTONUP, (Uint8) button);
	}
}

void SDLVideoDriver::MouseClickEvent(SDL_EventType type, Uint8 button)
{
	SDL_Event evtClick = SDL_Event();

	evtClick.type = type;
	evtClick.button.type = type;
	evtClick.button.button = button;
	evtClick.button.state = (type==SDL_MOUSEBUTTONDOWN)?SDL_PRESSED:SDL_RELEASED;
	evtClick.button.x = CursorPos.x;
	evtClick.button.y = CursorPos.y;
	SDL_PushEvent(&evtClick);
}

int SDLVideoDriver::PollMovieEvents()
{
	SDL_Event event;

	while (SDL_PollEvent( &event )) {
		switch (event.type) {
			case SDL_QUIT:
			case SDL_MOUSEBUTTONUP:
				return 1;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
					case SDLK_q:
						return 1;
					case SDLK_f:
						ToggleFullscreenMode();
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	return 0;
}

void SDLVideoDriver::DrawMovieSubtitle(ieDword strRef)
{
	if (strRef!=subtitlestrref) {
		core->FreeString(subtitletext);
		if (!strRef)
			return;
		subtitletext = core->GetString(strRef);
		subtitlestrref = strRef;
	}
	if (subtitlefont && subtitletext) {
		// FIXME: ugly hack!
		SDL_Surface* temp = backBuf;
		backBuf = disp;

		//FYI: that 0 is pitch black
		//SDL_FillRect(disp, &subtitleregion_sdl, 0);
		subtitlefont->Print(subtitleregion, (unsigned char *) subtitletext, subtitlepal, IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_BOTTOM, true);
		backBuf = temp;
	}
}
