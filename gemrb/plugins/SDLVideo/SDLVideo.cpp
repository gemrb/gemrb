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
#include "SDLSurfaceSprite2D.h"

#include "TileRenderer.inl"
#include "SpriteRenderer.inl"

#include "AnimationFactory.h"
#include "Game.h" // for GetGlobalTint
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"

#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/Window.h"

#include <cmath>

using namespace GemRB;

#if SDL_VERSION_ATLEAST(1,3,0)
#define SDL_SRCCOLORKEY SDL_TRUE
#define SDL_SRCALPHA 0
#define SDLKey SDL_Keycode
#define SDLK_SCROLLOCK SDLK_SCROLLLOCK
#define SDLK_KP1 SDLK_KP_1
#define SDLK_KP2 SDLK_KP_2
#define SDLK_KP3 SDLK_KP_3
#define SDLK_KP4 SDLK_KP_4
#define SDLK_KP6 SDLK_KP_6
#define SDLK_KP7 SDLK_KP_7
#define SDLK_KP8 SDLK_KP_8
#define SDLK_KP9 SDLK_KP_9
#else
typedef Sint32 SDL_Keycode;
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
	disp = NULL;
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	delete subtitletext;

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
	if (!(MouseFlags&MOUSE_HIDDEN)) {
		SDL_ShowCursor( SDL_DISABLE );
	}
	return GEM_OK;
}

int SDLVideoDriver::SwapBuffers(void)
{
	unsigned long time;
	time = GetTickCount();
	if (( time - lastTime ) < 33) {
#ifndef NOFPSLIMIT
		SDL_Delay( 33 - (time - lastTime) );
#endif
		time = GetTickCount();
	}
	lastTime = time;

	if (Cursor[CursorIndex] && !(MouseFlags & (MOUSE_DISABLED | MOUSE_HIDDEN))) {
		
		if (MouseFlags&MOUSE_GRAYED) {
			//used for greyscale blitting, fadeColor is unused
			BlitGameSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, BLIT_GREY, fadeColor, NULL, NULL, NULL, true);
		} else {
			BlitSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, true);
		}
	}
	if (!(MouseFlags & MOUSE_NO_TOOLTIPS)) {
		//handle tooltips
		unsigned int delay = core->TooltipDelay;
		// The multiplication by 10 is there since the last, disabling slider position is the eleventh
		if (!core->ConsolePopped && (delay<TOOLTIP_DELAY_FACTOR*10) ) {
			unsigned long time = GetTickCount();
			/** Display tooltip if mouse is idle */
			if (( time - lastMouseMoveTime ) > delay) {
				if (EvntManager)
					EvntManager->MouseIdle( time - lastMouseMoveTime );
			}

			core->DrawTooltip();
		}
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

	if (ret == GEM_OK && !(MouseFlags & (MOUSE_DISABLED | MOUSE_GRAYED))
		&& lastTime>lastMouseDownTime
		&& SDL_GetMouseState(NULL, NULL)==SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		// get our internal mouse coordinates instead of system coordinates
		// this is important for SDL2 (Android, iOS currently)
		Point p = GetMousePos();
		lastMouseDownTime=lastTime + EvntManager->GetRKDelay();
		if (!core->ConsolePopped) {
			EvntManager->MouseUp( p.x, p.y, GEM_MB_ACTION, GetModState(SDL_GetModState()) );
			Control* ctl = EvntManager->GetMouseFocusedControl();
			if (ctl && ctl->ControlType == IE_GUI_BUTTON)
				// these are repeat events so the control should stay pressed
				((Button*)ctl)->SetState(IE_GUI_BUTTON_PRESSED);
		}
	} 
	return ret;
}

int SDLVideoDriver::ProcessEvent(const SDL_Event & event)
{
	if (!EvntManager)
		return GEM_ERROR;

	SDL_Keycode key = SDLK_UNKNOWN;
	int modstate = GetModState(event.key.keysym.mod);
	SDLKey sym = event.key.keysym.sym;

	/* Loop until there are no events left on the queue */
	switch (event.type) {
			/* Process the appropriate event type */
		case SDL_QUIT:
			/* Quit event originated from outside GemRB so ask the user if we should exit */
			core->AskAndExit();
			return GEM_OK;
			break;
		case SDL_KEYUP:
			switch(sym) {
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				case SDLK_SCROLLOCK:
					key = GEM_GRAB;
					break;
				case SDLK_f:
					// FIXME: this is a poor place to handle this
					// cant do it in GameControl because it only exists while in a game
					// could move to event manager, but that seems on par with doing it here
					if (modstate & GEM_MOD_CTRL) {
						ToggleFullscreenMode();
						break;
					}
					// fallthrough
				default:
					if (sym < 256) {
						key = sym;
					}
					break;
			}
			if (!core->ConsolePopped && ( key != 0 ))
				EvntManager->KeyRelease( key, modstate );
			break;
		case SDL_KEYDOWN:
			if ((sym == SDLK_SPACE) && modstate & GEM_MOD_CTRL) {
				core->PopupConsole();
				break;
			}
#if SDL_VERSION_ATLEAST(1,3,0)
			key = SDL_GetKeyFromScancode(event.key.keysym.scancode);
#else
			key = event.key.keysym.unicode;
#endif
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
					key = GEM_FUNCTION1 + sym-SDLK_F1;
					break;
				default:
					if (( key != 0 )) {
						if (core->ConsolePopped)
							core->console->OnKeyPress( key, modstate);
						else
							EvntManager->KeyPress( key, modstate);
					}
					return GEM_OK;
			}
			if (core->ConsolePopped)
				core->console->OnSpecialKeyPress( key );
			else
				EvntManager->OnSpecialKeyPress( key );
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

Sprite2D* SDLVideoDriver::CreateSprite(int w, int h, int bpp, ieDword rMask,
	ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	SDLSurfaceSprite2D* spr = new SDLSurfaceSprite2D(w, h, bpp, pixels, rMask, gMask, bMask, aMask);

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

Sprite2D* SDLVideoDriver::CreateSprite8(int w, int h, void* pixels,
										Palette* palette, bool cK, int index)
{
	return CreatePalettedSprite(w, h, 8, pixels, palette->col, cK, index);
}

Sprite2D* SDLVideoDriver::CreatePalettedSprite(int w, int h, int bpp, void* pixels,
											   Color* palette, bool cK, int index)
{
	SDLSurfaceSprite2D* spr = new SDLSurfaceSprite2D(w, h, bpp, pixels);
	spr->SetPalette(palette);
	if (cK) {
		spr->SetColorKey(index);
	}
	return spr;
}

void SDLVideoDriver::BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y, const Region* clip, unsigned int flags)
{
	if (spr->BAM) {
		Log(ERROR, "SDLVideo", "Tile blit not supported for this sprite");
		return;
	}

	x -= Viewport.x;
	y -= Viewport.y;

	Region fClip = ClippedDrawingRect(Region(x, y, 64, 64), clip);

	const Uint8* data = (const Uint8*)spr->pixels;
	const SDL_Color* pal = reinterpret_cast<const SDL_Color*>(spr->GetPaletteColors());

	const Uint8* mask_data = NULL;
	Uint32 ck = 0;
	if (mask) {
		mask_data = (Uint8*) mask->pixels;
		ck = mask->GetColorKey();
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
			BlitTile_internal<Uint32>(backBuf, x, y, fClip.x - x, fClip.y - y, fClip.w, fClip.h, data, pal, mask_data, ck, T, B); \
		else \
			BlitTile_internal<Uint16>(backBuf, x, y, fClip.x - x, fClip.y - y, fClip.w, fClip.h, data, pal, mask_data, ck, T, B); \

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
								const Region* clip, Palette* palette)
{
	Region dst(x - spr->XPos, y - spr->YPos, spr->Width, spr->Height);

	if (!anchor) {
		dst.x -= Viewport.x;
		dst.y -= Viewport.y;
	}

	Region fClip = ClippedDrawingRect(dst, clip);

	if (fClip.Dimensions().IsEmpty()) {
		return; // already know blit fails
	}

	Region src(0, 0, spr->Width, spr->Height);

	// adjust the src region to account for the clipping
	src.x += fClip.x - dst.x; // the left edge
	src.w -= dst.w - fClip.w; // the right edge
	src.y += fClip.y - dst.y; // the top edge
	src.h -= dst.h - fClip.h; // the bottom edge

	assert(src.w == fClip.w && src.h == fClip.h);

	// just pass fclip as dst
	BlitSprite(spr, src, fClip, palette);
}

void SDLVideoDriver::BlitSprite(const Sprite2D* spr, const Region& src, const Region& dst, Palette* palette)
{
	if (dst.w <= 0 || dst.h <= 0)
		return; // we already know blit fails

	if (!spr->BAM) {
		SDL_Surface* surf = ((SDLSurfaceSprite2D*)spr)->GetSurface();
		if (palette) {
			SDL_Color* palColors = (SDL_Color*)spr->GetPaletteColors();
			SetSurfacePalette(surf, (SDL_Color*)palette->col);
			BlitSurfaceClipped(surf, src, dst);
			SetSurfacePalette(surf, palColors);
		} else {
			BlitSurfaceClipped(surf, src, dst);
		}
	} else {
		const Uint8* srcdata = (const Uint8*)spr->pixels;

		SDL_LockSurface(backBuf);

		Palette* pal = palette;
		if (!pal) {
			pal = spr->GetPalette();
			pal->release();
		}
		SRShadow_Regular shadow;

		// FIXME: our BAM blitters dont let us start at an arbitrary point in the source
		// We will compensate by tricking them by manipulating the location and size of the blit
		// then using dst as a clipping rect to achieve the effect of a partial src copy

		int x = dst.x - src.x;
		int y = dst.y - src.y;
		int w = spr->Width;
		int h = spr->Height;

		if (pal->alpha) {
			SRTinter_NoTint<true> tinter;
			SRBlender_Alpha blender;

			BlitSpritePAL_dispatch(false, (spr->renderFlags&BLIT_MIRRORX),
								   backBuf, srcdata, pal->col, x, y, w, h,
								   (spr->renderFlags&BLIT_MIRRORY), dst,
								   (Uint8)spr->GetColorKey(), 0, spr, 0, shadow, tinter, blender);
		} else {
			SRTinter_NoTint<false> tinter;
			SRBlender_NoAlpha blender;

			BlitSpritePAL_dispatch(false, (spr->renderFlags&BLIT_MIRRORX),
								   backBuf, srcdata, pal->col, x, y, w, h,
								   (spr->renderFlags&BLIT_MIRRORY), dst,
								   (Uint8)spr->GetColorKey(), 0, spr, 0, shadow, tinter, blender);
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

	if (!spr->BAM) {
		SDL_Surface* surf = ((SDLSurfaceSprite2D*)spr)->GetSurface();
		if (surf->format->BytesPerPixel != 4 && surf->format->BytesPerPixel != 1) {
			// TODO...
			Log(ERROR, "SDLVideo", "BlitGameSprite not supported for this sprite");
			BlitSprite(spr, x, y, false, clip);
			return;
		}
	} else {
		if (!palette) {
			palette = spr->GetPalette();
			palette->release(); // GetPalette increases the ref count
		}
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

	Region finalclip = ClippedDrawingRect(Region(tx, ty, spr->Width, spr->Height), clip);
	if (finalclip.w <= 0 || finalclip.h <= 0)
		return;

	SDL_LockSurface(backBuf);

	bool hflip = spr->BAM ? (spr->renderFlags&BLIT_MIRRORX) : false;
	bool vflip = spr->BAM ? (spr->renderFlags&BLIT_MIRRORY) : false;
	if (flags & BLIT_MIRRORX) hflip = !hflip;
	if (flags & BLIT_MIRRORY) vflip = !vflip;

	// remove already handled flags and incompatible combinations
	unsigned int remflags = flags & ~(BLIT_MIRRORX | BLIT_MIRRORY);
	if (remflags & BLIT_NOSHADOW) remflags &= ~BLIT_TRANSSHADOW;
	if (remflags & BLIT_GREY) remflags &= ~BLIT_SEPIA;


	if (spr->BAM && remflags == BLIT_TINTED) {

		SRShadow_Regular shadow;
		SRTinter_Tint<false, false> tinter(tint);
		SRBlender_NoAlpha blender;

		BlitSpritePAL_dispatch(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);

	} else if (spr->BAM && remflags == (BLIT_TINTED | BLIT_TRANSSHADOW)) {

		SRShadow_HalfTrans shadow(backBuf->format, palette->col[1]);
		SRTinter_Tint<false, false> tinter(tint);
		SRBlender_NoAlpha blender;

		BlitSpritePAL_dispatch(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);

	} else if (spr->BAM && remflags == (BLIT_TINTED | BLIT_NOSHADOW)) {

		SRShadow_None shadow;
		SRTinter_Tint<false, false> tinter(tint);
		SRBlender_NoAlpha blender;

		BlitSpritePAL_dispatch(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);

	} else if (spr->BAM && remflags == BLIT_HALFTRANS) {

		SRShadow_HalfTrans shadow(backBuf->format, palette->col[1]);
		SRTinter_NoTint<false> tinter;
		SRBlender_NoAlpha blender;

		BlitSpritePAL_dispatch(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);

	} else if (spr->BAM && remflags == 0) {

		SRShadow_Regular shadow;
		SRTinter_NoTint<false> tinter;
		SRBlender_NoAlpha blender;

		BlitSpritePAL_dispatch(cover, hflip, backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);

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

		SRShadow_Flags shadow; // for halftrans, noshadow, transshadow
		SRBlender_Alpha blender;
		if (remflags & blit_PALETTEALPHA) {
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<true> tinter(tint);

				BlitSpritePAL_dispatch(cover, hflip,
				    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<true> tinter;

				BlitSpritePAL_dispatch(cover, hflip,
				    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);
			}
		} else {
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<false> tinter(tint);

				BlitSpritePAL_dispatch(cover, hflip,
				    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<false> tinter;

				BlitSpritePAL_dispatch(cover, hflip,
				    backBuf, srcdata, palette->col, tx, ty, spr->Width, spr->Height, vflip, finalclip, (Uint8)spr->GetColorKey(), cover, spr, remflags, shadow, tinter, blender);
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
		const SDL_Surface* surf = ((SDLSurfaceSprite2D*)spr)->GetSurface();
		if (surf->format->BytesPerPixel == 1) {

			if (remflags & BLIT_TINTED)
				tint.a = 255;

			// NB: GemRB::Color has exactly the same layout as SDL_Color
			if (!palette)
				col = reinterpret_cast<const Color*>(surf->format->palette->colors);
			else
				col = palette->col;

			const Uint8 *data = (const Uint8*)spr->pixels;

			SRBlender_Alpha blender;
			SRShadow_NOP shadow;
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<false> tinter(tint);

				BlitSpritePAL_dispatch(cover, hflip,
				    backBuf, data, col, tx, ty, spr->Width, spr->Height, vflip, finalclip, -1, cover, spr, remflags, shadow, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<false> tinter;

				BlitSpritePAL_dispatch(cover, hflip,
				    backBuf, data, col, tx, ty, spr->Width, spr->Height, vflip, finalclip, -1, cover, spr, remflags, shadow, tinter, blender);
			}

		} else {

			const Uint32 *data = (const Uint32*)spr->pixels;

			SRBlender_Alpha blender;
			if (remflags & BLIT_TINTED) {
				SRTinter_Flags<true> tinter(tint);

				BlitSpriteRGB_dispatch(cover, hflip,
				    backBuf, data, tx, ty, spr->Width, spr->Height, vflip, finalclip, cover, spr, remflags, tinter, blender);
			} else {
				SRTinter_FlagsNoTint<true> tinter;

				BlitSpriteRGB_dispatch(cover, hflip,
				    backBuf, data, tx, ty, spr->Width, spr->Height, vflip, finalclip, cover, spr, remflags, tinter, blender);
			}

		}

	}

	SDL_UnlockSurface(backBuf);

}

Sprite2D* SDLVideoDriver::GetScreenshot( Region r )
{
	unsigned int Width = r.w ? r.w : disp->w;
	unsigned int Height = r.h ? r.h : disp->h;

	void* pixels = malloc( Width * Height * 3 );
	SDLSurfaceSprite2D* screenshot = new SDLSurfaceSprite2D(Width, Height, 24, pixels,
															0x00ff0000, 0x0000ff00, 0x000000ff);
	SDL_Rect src = RectFromRegion(r);
	SDL_BlitSurface( backBuf, (r.w && r.h) ? &src : NULL, screenshot->GetSurface(), NULL);

	return screenshot;
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDLVideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill, bool clipped)
{
	if (fill) {
		if ( SDL_ALPHA_TRANSPARENT == color.a ) {
			return;
		} else if ( SDL_ALPHA_OPAQUE == color.a ) {
			long val = SDL_MapRGBA( backBuf->format, color.r, color.g, color.b, color.a );
			SDL_Rect drect = RectFromRegion(ClippedDrawingRect(rgn));
			SDL_FillRect( backBuf, &drect, val );
		} else {
			SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
			SDL_Color c;
			c.r = color.r;
			c.b = color.b;
			c.g = color.g;
			SetSurfacePalette(rectsurf, &c, 1);
			SetSurfaceAlpha(rectsurf, color.a);
			BlitSurfaceClipped(rectsurf, Region(0, 0, rgn.w, rgn.h), rgn);
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

	SDL_Surface* surf = ((SDLSurfaceSprite2D*)sprite)->GetSurface();
	SDL_Rect drect = RectFromRegion(rgn);
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
		SDLVideoDriver::SetSurfacePalette(rectsurf, &c, 1);
		SetSurfaceAlpha(rectsurf, color.a);
		SDL_BlitSurface( rectsurf, NULL, surf, &drect );
		SDL_FreeSurface( rectsurf );
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

	SDLVideoDriver::SetSurfacePixel(backBuf, x, y, color);
}

void SDLVideoDriver::GetPixel(short x, short y, Color& c)
{
	SDLVideoDriver::GetSurfacePixel(backBuf, x, y, c);
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
		delete subtitletext;
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
		subtitlefont->Print(subtitleregion, *subtitletext, subtitlepal, IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_BOTTOM);
		backBuf = temp;
	}
}

void SDLVideoDriver::BlitSurfaceClipped(SDL_Surface* surf, const Region& src, const Region& dst)
{
	SDL_Rect srect = RectFromRegion(src); // FIXME: this may not be clipped
	Region dclipped = ClippedDrawingRect(dst);
	int trim = dst.h - dclipped.h;
	if (trim) {
		srect.h -= trim;
		if (dclipped.y > dst.y) { // top clipped
			srect.y += trim;
		} // already have appropriate y for bottom clip
	}
	trim = dst.w - dclipped.w;
	if (trim) {
		srect.w -= trim;
		if (dclipped.x > dst.x) { // left clipped
			srect.x += trim;
		}
	} // already have appropriate y for right clip

	SDL_Rect drect = RectFromRegion(dclipped);
	// since we should already be clipped we can call SDL_LowerBlit directly
	SDL_LowerBlit(surf, &srect, backBuf, &drect);
}

// static class methods

void SDLVideoDriver::SetSurfacePalette(SDL_Surface* surf, SDL_Color* pal, int numcolors)
{
	if (pal) {
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_SetPaletteColors( surf->format->palette, pal, 0, numcolors );
#else
		SDL_SetPalette( surf, SDL_LOGPAL | SDL_RLEACCEL, pal, 0, numcolors );
#endif
	}
}

void SDLVideoDriver::SetSurfacePixel(SDL_Surface* surface, short x, short y, const Color& color)
{
	SDL_PixelFormat* fmt = surface->format;
	unsigned char * pixels = ( ( unsigned char * ) surface->pixels ) +
	( ( y * surface->w + x) * fmt->BytesPerPixel );

	Uint32 val = SDL_MapRGBA( fmt, color.r, color.g, color.b, color.a );
	SDL_LockSurface( surface );

	switch (fmt->BytesPerPixel) {
		case 1:
			*pixels = (unsigned char)val;
			break;
		case 2:
			*(Uint16 *)pixels = (Uint16)val;
			break;
		case 3:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			pixels[0] = val & 0xff;
			pixels[1] = (val >> 8) & 0xff;
			pixels[2] = (val >> 16) & 0xff;
#else
			pixels[2] = val & 0xff;
			pixels[1] = (val >> 8) & 0xff;
			pixels[0] = (val >> 16) & 0xff;
#endif
			break;
		case 4:
			*(Uint32 *)pixels = val;
			break;
		default:
			Log(ERROR, "SDLSurfaceSprite2D", "Working with unknown pixel format: %s", SDL_GetError());
			break;
	}

	SDL_UnlockSurface( surface );
}

void SDLVideoDriver::GetSurfacePixel(SDL_Surface* surface, short x, short y, Color& c)
{
	SDL_LockSurface( surface );
	Uint8 Bpp = surface->format->BytesPerPixel;
	unsigned char * pixels = ( ( unsigned char * ) surface->pixels ) +
	( ( y * surface->w + x) * Bpp );
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
}

SDL_Rect SDLVideoDriver::RectFromRegion(const Region& rgn)
{
	SDL_Rect rect = {(Sint16)rgn.x, (Sint16)rgn.y, (Uint16)rgn.w, (Uint16)rgn.h};
	return rect;
}
