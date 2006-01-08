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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/SDLVideo/SDLVideoDriver.cpp,v 1.128 2006/01/08 22:31:42 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "SDLVideoDriver.h"
#include "../Core/Interface.h"
#include <cmath>
#include <cassert>
#include "../Core/SpriteCover.h"
#include "../Core/Console.h"
#include "../Core/SoundMgr.h"

class GEM_EXPORT Sprite2D_BAM_Internal {
public:
	SDL_Color pal[256];
	unsigned int datasize;
	bool RLE;
	int transindex;
	bool flip_hor;
	bool flip_ver;
	bool alpha_pal;
};

//actually it won't be transparent :(
SDL_Color TRANSPARENT_BLACK={0,0,0,SDL_ALPHA_TRANSPARENT};

SDLVideoDriver::SDLVideoDriver(void)
{
	CursorIndex = 0;
	Cursor[0] = NULL;
	Cursor[1] = NULL;
	Cursor[2] = NULL;
	CursorPos.x = 0;
	CursorPos.y = 0;
	moveX = 0;
	moveY = 0;
	DisableMouse = false;
	DisableScroll = false;
	xCorr = 0;
	yCorr = 0;
	lastTime = 0;
	GetTime( lastMouseTime );
	backBuf=NULL;
	extra=NULL;
	subtitlestrref = 0;
	subtitletext = NULL;
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	core->FreeString(subtitletext); //may be NULL
	if(backBuf) SDL_FreeSurface( backBuf );
	if(extra) SDL_FreeSurface( extra );
	SDL_Quit();
}

int SDLVideoDriver::Init(void)
{
	//printf("[SDLVideoDriver]: Init...");
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1) {
		//printf("[ERROR]\n");
		return GEM_ERROR;
	}
	//printf("[OK]\n");
	SDL_EnableUNICODE( 1 );
	SDL_EnableKeyRepeat( 500, 50 );
	SDL_ShowCursor( SDL_DISABLE );
	fadeColor.a=0; //fadePercent
	fadeColor.r=0;
	fadeColor.b=0;
	fadeColor.g=0;
	return GEM_OK;
}

int SDLVideoDriver::CreateDisplay(int width, int height, int bpp,
	bool fullscreen)
{
	printMessage( "SDLVideo", "Creating display\n", WHITE );
	ieDword flags = SDL_HWSURFACE;// | SDL_DOUBLEBUF;
	if (fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	printMessage( "SDLVideo", "SDL_SetVideoMode...", WHITE );
	disp = SDL_SetVideoMode( width, height, bpp, flags );
	if (disp == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "SDLVideo", "Checking for HardWare Acceleration...", WHITE );
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	if (!vi) {
		printStatus( "ERROR", LIGHT_RED );
	}
	printStatus( "OK", LIGHT_GREEN );
	Viewport.x = Viewport.y = 0;
	Viewport.w = width;
	Viewport.h = height;
	printMessage( "SDLVideo", "Creating Main Surface...", WHITE );
	SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height,
						bpp, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "SDLVideo", "Creating Back Buffer...", WHITE );
	backBuf = SDL_DisplayFormat( tmp );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "SDLVideo", "Creating Extra Buffer...", WHITE );
	extra = SDL_DisplayFormat( tmp );
	printStatus( "OK", LIGHT_GREEN );
	SDL_LockSurface( extra );
	long val = SDL_MapRGBA( extra->format, fadeColor.r, fadeColor.g, fadeColor.b, 0 );
	SDL_FillRect( extra, NULL, val );	
	SDL_UnlockSurface( extra );
	SDL_FreeSurface( tmp );
	printMessage( "SDLVideo", "CreateDisplay...", WHITE );
	printStatus( "OK", LIGHT_GREEN );
	return GEM_OK;
}

void SDLVideoDriver::SetDisplayTitle(char* title, char* icon)
{
	SDL_WM_SetCaption( title, icon );
}

VideoModes SDLVideoDriver::GetVideoModes(bool fullscreen)
{
	SDL_Rect** modes;
	ieDword flags = SDL_HWSURFACE;
	if (fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	VideoModes vm;
	//32-bit Video Modes
	SDL_PixelFormat pf;
	pf.palette = NULL;
	pf.BitsPerPixel = 32;
	pf.BytesPerPixel = 4;
	pf.Rmask = 0xff000000;
	pf.Gmask = 0x00ff0000;
	pf.Bmask = 0x0000ff00;
	pf.Amask = 0x000000ff;
	pf.Rshift = 24;
	pf.Gshift = 16;
	pf.Bshift = 8;
	pf.Ashift = 0;
	modes = SDL_ListModes( &pf, fullscreen );
	if (modes == ( SDL_Rect * * ) 0) {
		return vm;
	}
	if (modes == ( SDL_Rect * * ) - 1) {
		vm.AddVideoMode( 640, 480, 32, fullscreen );
		vm.AddVideoMode( 800, 600, 32, fullscreen );
		vm.AddVideoMode( 1024, 786, 32, fullscreen );
		vm.AddVideoMode( 1280, 1024, 32, fullscreen );
		vm.AddVideoMode( 1600, 1200, 32, fullscreen );
	} else {
		for (int i = 0; modes[i]; i++) {
			vm.AddVideoMode( modes[i]->w, modes[i]->h, 32, fullscreen );
		}
	}
	return vm;
}

bool SDLVideoDriver::TestVideoMode(VideoMode& vm)
{
	ieDword flags = SDL_HWSURFACE;
	if (vm.GetFullScreen()) {
		flags |= SDL_FULLSCREEN;
	}
	if (SDL_VideoModeOK( vm.GetWidth(), vm.GetHeight(), vm.GetBPP(), flags ) ==
		vm.GetBPP()) {
		return true;
	}
	return false;
}

bool SDLVideoDriver::ToggleFullscreenMode()
{
	return SDL_WM_ToggleFullScreen(disp) != 0;
}

inline int GetModState(int modstate)
{
	int value = 0;
	if (modstate&KMOD_SHIFT) value |= GEM_MOD_SHIFT;
	if (modstate&KMOD_CTRL) value |= GEM_MOD_CTRL;
	if (modstate&KMOD_ALT) value |= GEM_MOD_ALT;
	return value;
}

int SDLVideoDriver::SwapBuffers(void)
{
	int ret = GEM_OK;
	unsigned long time;
	GetTime( time );
	if (( time - lastTime ) < 17) {
		SDL_Delay( 17 - (time - lastTime) );
		return ret;
	}
	lastTime = time;

	unsigned char key;
	bool ConsolePopped = core->ConsolePopped;

	if (ConsolePopped) {
		core->DrawConsole();
	}
	while (SDL_PollEvent( &event )) {
		/* Loop until there are no events left on the queue */
		switch (event.type) {
		/* Process the appropriate event type */
		case SDL_QUIT:
			/* Handle a QUIT event */
			ret = GEM_ERROR;
			break;

		case SDL_KEYUP:
			switch(event.key.keysym.sym) {
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
		                default:
					if (event.key.keysym.sym>=256) {
						key=0;
					} else {
						key=(unsigned char) event.key.keysym.sym;
					}
		                        break;
			}
			if (!ConsolePopped && Evnt && ( key != 0 ))
				Evnt->KeyRelease( key, GetModState(event.key.keysym.mod) );
			break;

		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				core->PopupConsole();
				break;
			}
			key = (unsigned char) event.key.keysym.unicode;
			if (key < 32 || key == 127) {
				switch (event.key.keysym.sym) {
				case SDLK_END:
					key = GEM_END;
					break;
				case SDLK_HOME:
					key = GEM_HOME;
					break;
				case SDLK_UP:
					key = GEM_UP;
					break;
				case SDLK_DOWN:
					key = GEM_DOWN;
					break;
				case SDLK_LEFT:
					key = GEM_LEFT;
					break;
				case SDLK_RIGHT:
					key = GEM_RIGHT;
					break;
				case SDLK_BACKSPACE:
					key = GEM_BACKSP;
					break;
				case SDLK_DELETE:
					key = GEM_DELETE;
					break;
				case SDLK_RETURN:
					key = GEM_RETURN;
					break;
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				default:
					break;
				}
				if (ConsolePopped)
					core->console->OnSpecialKeyPress( key );
				else if (Evnt)
					Evnt->OnSpecialKeyPress( key );
			} else if (( key != 0 )) {
				if (ConsolePopped)
					core->console->OnKeyPress( key, GetModState(event.key.keysym.mod) );
				else if (Evnt)
					Evnt->KeyPress( key, GetModState(event.key.keysym.mod) );
			}
			break;
		case SDL_MOUSEMOTION:
			MouseMovement(event.motion.x, event.motion.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (DisableMouse)
				break;
			if (CursorIndex != 2)
				CursorIndex = 1;
			CursorPos.x = event.button.x; // - mouseAdjustX[CursorIndex];
			CursorPos.y = event.button.y; // - mouseAdjustY[CursorIndex];
			if (Evnt && !ConsolePopped)
				Evnt->MouseDown( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );

			break;

		case SDL_MOUSEBUTTONUP:
			if (DisableMouse)
				break;
			if (CursorIndex != 2)
				CursorIndex = 0;
			CursorPos.x = event.button.x;
			CursorPos.y = event.button.y;
			if (Evnt && !ConsolePopped)
				Evnt->MouseUp( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );

			break;
		 case SDL_ACTIVEEVENT:
			if (ConsolePopped) {
				break;
			}

			if (event.active.state == SDL_APPMOUSEFOCUS) {
				if (Evnt && !event.active.gain)
					Evnt->OnSpecialKeyPress( GEM_MOUSEOUT );
			}
			break;

		}
	}
	SDL_BlitSurface( backBuf, NULL, disp, NULL );
	if (fadeColor.a) {
		SDL_SetAlpha( extra, SDL_SRCALPHA, fadeColor.a );
		SDL_Rect src = {
			0, 0, Viewport.w, Viewport.h
		};
		SDL_Rect dst = {
			xCorr, yCorr, 0, 0
		};
		SDL_BlitSurface( extra, &src, disp, &dst );
	}
	if (Cursor[CursorIndex] && !DisableMouse) {
		SDL_Surface* temp = backBuf;
		backBuf = disp; // FIXME: UGLY HACK!
		BlitSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, true);
		backBuf = temp;
	}

	if (!ConsolePopped) {
		/** Display tooltip if mouse is idle */
		if (( time - lastMouseTime ) > core->TooltipDelay) {
			if (Evnt)
				Evnt->MouseIdle( time - lastMouseTime );

			/** This causes the tooltip to be rendered directly to display */
			SDL_Surface* tmp = backBuf;
			backBuf = disp;
			core->DrawTooltip();
			backBuf = tmp;
		}
	}

	SDL_Flip( disp );
	return ret;
}

bool SDLVideoDriver::ToggleGrabInput()
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

Sprite2D* SDLVideoDriver::CreateSprite(int w, int h, int bpp, ieDword rMask,
	ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	Sprite2D* spr = new Sprite2D();
	void* p = SDL_CreateRGBSurfaceFrom( pixels, w, h, bpp, w*( bpp / 8 ),
				rMask, gMask, bMask, aMask );
	spr->vptr = p;
	spr->pixels = pixels;
	if (cK) {
		SDL_SetColorKey( ( SDL_Surface * ) p, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			index );
	}
	spr->Width = w;
	spr->Height = h;
	return spr;
}

Sprite2D* SDLVideoDriver::CreateSprite8(int w, int h, int bpp, void* pixels,
	void* palette, bool cK, int index)
{
	Sprite2D* spr = new Sprite2D();
	void* p = SDL_CreateRGBSurfaceFrom( pixels, w, h, 8, w, 0, 0, 0, 0 );
	int colorcount;
	if (bpp == 8) {
		colorcount = 256;
	} else {
		colorcount = 16;
	}
	SDL_SetPalette( ( SDL_Surface * ) p, SDL_LOGPAL, ( SDL_Color * ) palette, 0, colorcount );
	spr->vptr = p;
	spr->pixels = pixels;
	if (cK) {
		SDL_SetColorKey( ( SDL_Surface * ) p, SDL_SRCCOLORKEY | SDL_RLEACCEL, index );
	}
	spr->Width = w;
	spr->Height = h;
	return spr;
}

Sprite2D* SDLVideoDriver::CreateSpriteBAM8(int w, int h, bool rle,
										   void* pixeldata,
										   unsigned int datasize,
										   void* palette, int transindex)
{
	Sprite2D* spr = new Sprite2D();
	spr->BAM = true;
	Sprite2D_BAM_Internal* data = new Sprite2D_BAM_Internal;
	spr->vptr = data;

	SDL_Color* pal = (SDL_Color*) palette;
	memcpy(data->pal, pal, 256*sizeof(pal[0]));
	data->transindex = transindex;
	data->datasize = datasize;
	data->flip_hor = false;
	data->flip_ver = false;
	data->alpha_pal = false;
	data->RLE = rle;

	spr->pixels = pixeldata;
	spr->Width = w;
	spr->Height = h;

	return spr;
}

void SDLVideoDriver::FreeSprite(Sprite2D* spr)
{
	if(!spr)
		return;

	if (spr->BAM) {
		if (spr->vptr) {
			Sprite2D_BAM_Internal* tmp = (Sprite2D_BAM_Internal*)spr->vptr;
			delete tmp;
		}
	} else {
		if (spr->vptr) {
			SDL_FreeSurface( ( SDL_Surface * ) spr->vptr );
		}
	}
	if (spr->pixels) {
		free( spr->pixels );
	}
	delete spr;
	spr = NULL;
}

void SDLVideoDriver::BlitSpriteRegion(Sprite2D* spr, Region& size, int x,
	int y, bool anchor, Region* clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		//TODO: Add the destination surface and rect to the Blit Pipeline
		SDL_Rect drect;
		SDL_Rect t = {
			size.x, size.y, size.w, size.h
		};
		Region c;
		if (clip) {
			c = *clip;
		} else {
			c.x = 0;
			c.y = 0;
			c.w = backBuf->w;
			c.h = backBuf->h;
		}
		if (anchor) {
			drect.x = x;
			drect.y = y;
		} else {
			drect.x = x - Viewport.x;
			drect.y = y - Viewport.y;
			if (clip) {
				c.x -= Viewport.x;
				c.y -= Viewport.y;
			}
		}
		if (clip) {
			if (drect.x + size.w <= c.x)
				return;
			if (drect.x >= c.x + c.w)
				return;

			if (drect.y + size.h <= c.y)
				return;
			if (drect.y >= c.y + c.h)
				return;

			if (drect.x < c.x) {
				t.x += c.x - drect.x;
				t.w -= c.x - drect.x;
				drect.x = c.x;
			}
			if (drect.x + t.w > c.x + c.w) {
				t.w = c.x + c.w - drect.x;
			}

			if (drect.y < c.y) {
				t.y += c.y - drect.y;
				t.h -= c.y - drect.y;
				drect.y = c.y;
			}
			if (drect.y + t.h > c.y + c.h) {
				t.h = c.y + c.h - drect.y;
			}
		}
		SDL_BlitSurface( ( SDL_Surface * ) spr->vptr, &t, backBuf, &drect );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

		Uint8* rle = (Uint8*)spr->pixels;
		int tx, ty;
		if (anchor) {
			tx = x - spr->XPos;
			ty = y - spr->YPos;
		} else {
			tx = x - spr->XPos - Viewport.x;
			ty = y - spr->YPos - Viewport.y;
		}
		if (tx > backBuf->w) return;
		if (tx+spr->Width <= 0) return;

		SDL_LockSurface(backBuf);

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

		SDL_Rect cliprect;
		SDL_GetClipRect(backBuf, &cliprect);
		if (cliprect.x > clipx) {
			clipw -= (cliprect.x - clipx);
			clipx = cliprect.x;
		}
		if (cliprect.y > clipy) {
			cliph -= (cliprect.y - clipy);
			clipy = cliprect.y;
		}
		if (clipx+clipw > cliprect.x+cliprect.w) {
			clipw = cliprect.x+cliprect.w-clipx;
		}
		if (clipy+cliph > cliprect.y+cliprect.h) {
			cliph = cliprect.y+cliprect.h-clipy;
		}

		if (clipx < tx+size.x) {
			clipw -= (tx+size.x - clipx);
			clipx = tx+size.x;
		}
		if (clipy < ty+size.y) {
			cliph -= (ty+size.y - clipy);
			clipy = ty+size.y;
		}
		if (clipx+clipw > tx+size.x+size.w) {
			clipw = tx+size.x+size.w-clipx;
		}
		if (clipy+cliph > ty+size.y+size.h) {
			cliph = ty+size.y+size.h-clipy;
		}

#define ALREADYCLIPPED
#define SPECIALPIXEL
#define FLIP 
#define HFLIP_CONDITIONAL data->flip_hor
#define VFLIP_CONDITIONAL data->flip_ver
#define RLE data->RLE
#define PAL (Color*)data->pal
#undef COVER
#undef TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16
			if (data->alpha_pal) {

#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			} else {

#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			}

		} else {

#define BPP16
			if (data->alpha_pal) {

#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			} else {

#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			}

		}

#undef BPP16
#undef ALREADYCLIPPED
#undef SPECIALPIXEL
#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL

		SDL_UnlockSurface(backBuf);
	}
}


void SDLVideoDriver::BlitSprite(Sprite2D* spr, int x, int y, bool anchor,
	Region* clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		//TODO: Add the destination surface and rect to the Blit Pipeline
		SDL_Rect drect;
		SDL_Rect t;
		SDL_Rect* srect = NULL;
		if (anchor) {
			drect.x = x - spr->XPos;
			drect.y = y - spr->YPos;
		} else {
			drect.x = x - spr->XPos - Viewport.x;
			drect.y = y - spr->YPos - Viewport.y;
		}
		
		if (clip) {
			if (drect.x + spr->Width <= clip->x)
				return;
			if (drect.x >= clip->x + clip->w)
				return;

			if (drect.y + spr->Height <= clip->y)
				return;
			if (drect.y >= clip->y + clip->h)
				return;

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

		Uint8* rle = (Uint8*)spr->pixels;
		int tx, ty;
		if (anchor) {
			tx = x - spr->XPos;
			ty = y - spr->YPos;
		} else {
			tx = x - spr->XPos - Viewport.x;
			ty = y - spr->YPos - Viewport.y;
		}
		if (tx > backBuf->w) return;
		if (tx+spr->Width <= 0) return;

		SDL_LockSurface(backBuf);

#define SPECIALPIXEL
#undef BPP16
#define FLIP 
#define HFLIP_CONDITIONAL data->flip_hor
#define VFLIP_CONDITIONAL data->flip_ver
#define RLE data->RLE
#define PAL (Color*)data->pal
#undef COVER
#undef TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16
			if (data->alpha_pal) {

#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			} else {

#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			}

		} else {

#define BPP16
			if (data->alpha_pal) {

#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			} else {

#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			}

		}

#undef BPP16
#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL
#undef SPECIALPIXEL
#undef PALETTE_ALPHA

		SDL_UnlockSurface(backBuf);
	}
}


void SDLVideoDriver::BlitSpriteTinted(Sprite2D* spr, int x, int y, Color tint, 
	Color *Palette, Region* clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		SDL_Surface* tmp = ( SDL_Surface* ) spr->vptr;
		SDL_Color* pal = tmp->format->palette->colors;
		SDL_Color oldPal[256];
		memcpy( oldPal, pal, 256 * sizeof( SDL_Color ) );
		if (!Palette)
			Palette = (Color *) pal; //this is the original palette
		for (int i = 2; i < 256; i++) {
			pal[i].r = ( tint.r * Palette[i].r ) >> 8;
			pal[i].g = ( tint.g * Palette[i].g ) >> 8;
			pal[i].b = ( tint.b * Palette[i].b ) >> 8;
		}
		
		SDL_SetAlpha( tmp, SDL_SRCALPHA, tint.a);
		BlitSprite( spr, x, y, false, clip );
		//copying back the original palette
		SDL_SetPalette( tmp, SDL_LOGPAL, ( SDL_Color * ) oldPal, 0, 256 );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

		Uint8* rle = (Uint8*)spr->pixels;
		int tx = x - spr->XPos - Viewport.x;
		int ty = y - spr->YPos - Viewport.y;
		if (tx > backBuf->w) return;
		if (tx+spr->Width <= 0) return;
		SDL_LockSurface(backBuf);
		if (!Palette)
			Palette = (Color *) data->pal; //this is the original palette

#define SPECIALPIXEL
#define FLIP 
#define HFLIP_CONDITIONAL data->flip_hor
#define VFLIP_CONDITIONAL data->flip_ver
#define RLE data->RLE
#define PAL Palette
#undef COVER
#define TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16

			if (tint.a != 255) {

#define TINT_ALPHA

				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			} else {

#undef TINT_ALPHA
				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			}

		} else {

#define BPP16
			if (tint.a != 255) {

#define TINT_ALPHA

				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			} else {

#undef TINT_ALPHA
				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			}


		}

#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL
#undef TINT
#undef SPECIALPIXEL
#undef TINT_ALPHA
#undef PALETTE_ALPHA

		SDL_UnlockSurface(backBuf);	
	}
}


void SDLVideoDriver::BlitSpriteCovered(Sprite2D* spr, int x, int y,
									   Color tint, SpriteCover* cover,
									   Color *Palette, Region* clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		printMessage( "SDLVideo", "Covered blit not supported for this sprite\n", LIGHT_RED );
		BlitSpriteTinted(spr, x, y, tint, Palette, clip);
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

		Uint8* rle = (Uint8*)spr->pixels;
		int tx = x - spr->XPos - Viewport.x;
		int ty = y - spr->YPos - Viewport.y;
		if (tx > backBuf->w) return;
		if (tx+spr->Width <= 0) return;
		SDL_LockSurface(backBuf);
		if (!Palette)
			Palette = (Color *) data->pal;

#define SPECIALPIXEL
#define FLIP 
#define HFLIP_CONDITIONAL data->flip_hor
#define VFLIP_CONDITIONAL data->flip_ver
#define RLE data->RLE
#define PAL Palette
#define COVER
#define COVERX (cover->XPos - spr->XPos)
#define COVERY (cover->YPos - spr->YPos)
#define TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16

			if (tint.a != 255) {

#define TINT_ALPHA

				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			} else {

#undef TINT_ALPHA
				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			}

		} else {

#define BPP16
			if (tint.a != 255) {

#define TINT_ALPHA

				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			} else {

#undef TINT_ALPHA
				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			}


		}

#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL
#undef COVER
#undef COVERX
#undef COVERY
#undef TINT
#undef SPECIALPIXEL
#undef TINT_ALPHA
#undef PALETTE_ALPHA

		SDL_UnlockSurface(backBuf);

	}
}


void SDLVideoDriver::BlitSpriteNoShadow(Sprite2D* spr, int x, int y,
										Color tint, SpriteCover* cover,
										Color *Palette, Region *clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		printMessage( "SDLVideo", "Covered blit not supported for this sprite\n", LIGHT_RED );
		BlitSpriteTinted(spr, x, y, tint, Palette, clip);
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

		Uint8* rle = (Uint8*)spr->pixels;
		int tx = x - spr->XPos - Viewport.x;
		int ty = y - spr->YPos - Viewport.y;
		if (tx > backBuf->w) return;
		if (tx+spr->Width <= 0) return;
		if (!Palette)
			Palette = (Color *) data->pal;
		SDL_LockSurface(backBuf);

#define FLIP 
#define HFLIP_CONDITIONAL data->flip_hor
#define VFLIP_CONDITIONAL data->flip_ver
#define RLE data->RLE
#define PAL Palette
#define COVER
#define COVERX (cover->XPos - spr->XPos)
#define COVERY (cover->YPos - spr->YPos)
#define TINT
#define SPECIALPIXEL if (p != 1)

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16
#include "SDLVideoDriver.inl"

		} else {

#define BPP16
#include "SDLVideoDriver.inl"

		}

#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL
#undef COVER
#undef COVERX
#undef COVERY
#undef TINT
#undef SPECIALPIXEL

		SDL_UnlockSurface(backBuf);

	}
}

// TODO: properly mix translucent shadow and alpha tint/palette
void SDLVideoDriver::BlitSpriteTransShadow(Sprite2D* spr, int x, int y,
										   Color tint, SpriteCover* cover,
										   Color *Palette, Region *clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		printMessage( "SDLVideo", "Covered blit not supported for this sprite\n", LIGHT_RED );
		BlitSpriteTinted(spr, x, y, tint, Palette, clip);
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

		Uint8* rle = (Uint8*)spr->pixels;
		int tx = x - spr->XPos - Viewport.x;
		int ty = y - spr->YPos - Viewport.y;
		if (tx > backBuf->w) return;
		if (tx+spr->Width <= 0) return;
		if (!Palette)
			Palette = (Color *) data->pal;

		Uint32 shadowcol32 = SDL_MapRGBA(backBuf->format, Palette[1].r/2, Palette[1].g/2, Palette[1].b/2, 0);
		Uint16 shadowcol16 = (Uint16)shadowcol32;

		Uint32 mask32 = (backBuf->format->Rmask >> 1) & backBuf->format->Rmask;
		mask32 |= (backBuf->format->Gmask >> 1) & backBuf->format->Gmask;
		mask32 |= (backBuf->format->Bmask >> 1) & backBuf->format->Bmask;
		Uint16 mask16 = (Uint16)mask32;

		SDL_LockSurface(backBuf);

#define FLIP 
#define HFLIP_CONDITIONAL data->flip_hor
#define VFLIP_CONDITIONAL data->flip_ver
#define RLE data->RLE
#define PAL Palette
#define COVER
#define COVERX (cover->XPos - spr->XPos)
#define COVERY (cover->YPos - spr->YPos)
#define TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16
#define SPECIALPIXEL if (p == 1) { *pix = ((*pix >> 1)&mask32) + shadowcol32; } else

			if (tint.a != 255) {

#define TINT_ALPHA

				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			} else {

#undef TINT_ALPHA
				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			}

		} else {

#define BPP16
#undef SPECIALPIXEL
#define SPECIALPIXEL if (p == 1) { *pix = ((*pix >> 1)&mask16) + shadowcol16; } else

			if (tint.a != 255) {

#define TINT_ALPHA

				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			} else {

#undef TINT_ALPHA
				if (data->alpha_pal) {
#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				} else {
#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"
				}

			}


		}

#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL
#undef COVER
#undef COVERX
#undef COVERY
#undef TINT
#undef SPECIALPIXEL
#undef TINT_ALPHA
#undef PALETTE_ALPHA

		SDL_UnlockSurface(backBuf);

	}
}



void SDLVideoDriver::SetCursor(Sprite2D* up, Sprite2D* down)
{
	if (up) {
		Cursor[0] = up;
	} else {
		Cursor[0] = NULL;
	}
	if (down) {
		Cursor[1] = down;
	} else {
		Cursor[1] = NULL;
	}
	return;
}

// Drag cursor is shown instead of all other cursors
void SDLVideoDriver::SetDragCursor(Sprite2D* drag)
{
	if (drag) {
		Cursor[2] = drag;
		CursorIndex = 2;
	} else {
		CursorIndex = 0;
		Cursor[2] = NULL;
	}
}

Sprite2D* SDLVideoDriver::GetPreview(int /*w*/, int /*h*/)
{
	int Width = disp->w;
	int Height = disp->h;

	SDL_Surface* surf = SDL_CreateRGBSurface( SDL_SWSURFACE, Width, Height, 24,
				0xFF0000, 0x00FF00, 0x0000FF, 0x000000 );
	SDL_BlitSurface( backBuf, NULL, surf, NULL);
	void* pixels = malloc( Width * Height * 3 );
	memcpy( pixels, surf->pixels, Width * Height * 3 );
	//freeing up temporary surface as we copied its pixels
	Sprite2D* preview = CreateSprite( Width, Height, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, pixels, false, 0 );
	SDL_FreeSurface(surf);
	return preview;
}

Region SDLVideoDriver::GetViewport()
{
	return Viewport;
}

void SDLVideoDriver::SetViewport(int x, int y, unsigned int w, unsigned int h)
{
	if (x>disp->w)
		x=disp->w;
	xCorr = x;
	if (y>disp->h)
		y=disp->h;
	yCorr = y;
	if (w>(unsigned int) disp->w)
		w=0;
	Viewport.w = w;
	if (h>(unsigned int) disp->h)
		h=0;
	Viewport.h = h;
}

void SDLVideoDriver::MoveViewportTo(int x, int y, bool center)
{
	if (center) {
		x -= ( Viewport.w / 2 );
		y -= ( Viewport.h / 2 );
	}

	if (x != Viewport.x || y != Viewport.y) {
		core->GetSoundMgr()->UpdateViewportPos( (x - xCorr) + disp->w / 2, (y - yCorr) + disp->h / 2 );
		Viewport.x = x;
		Viewport.y = y;
	}
}
/** No descriptions */
void SDLVideoDriver::SetPalette(Sprite2D* spr, Color* pal)
{
	if (!spr->BAM) {
		SDL_Surface* sur = ( SDL_Surface* ) spr->vptr;
		SDL_SetPalette( sur, SDL_LOGPAL, ( SDL_Color * ) pal, 0, 256 );
	} else {
		if (!spr->vptr) return;
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;
		memcpy(data->pal, pal, 256*sizeof(data->pal[0]));
	}
}

void SDLVideoDriver::ConvertToVideoFormat(Sprite2D* sprite)
{
	if (!sprite->BAM) {
		SDL_Surface* ss = ( SDL_Surface* ) sprite->vptr;
		if (ss->format->Amask != 0) //Surface already converted
		{
			return;
		}
		SDL_Surface* ns = SDL_DisplayFormatAlpha( ss );
		if (ns == NULL) {
			return;
		}
		SDL_FreeSurface( ss );
		if (sprite->pixels) {
			free( sprite->pixels );
		}
		sprite->pixels = NULL;
		sprite->vptr = ns;
	}
}

#define MINCOL 2
#define MUL    2

void SDLVideoDriver::CalculateAlpha(Sprite2D* sprite)
{
	if (!sprite->BAM) {
		// FIXME: 32 bit RGBA only currently
		SDL_Surface* surf = ( SDL_Surface* ) sprite->vptr;
		if (surf->format->BytesPerPixel != 4) return;
		SDL_LockSurface( surf );
		unsigned char * p = ( unsigned char * ) surf->pixels;
		unsigned char * end = p + ( surf->pitch * surf->h );
		unsigned char r,g,b,m;
		while (p < end) {
			r = *p++;
		/*if(r > MINCOL)
					r = 0xff;
				else
					r*=MUL;*/
			g = *p++;
		/*if(g > MINCOL)
					g = 0xff;
				else
					g*=MUL;*/
			b = *p++;
		/*if(b > MINCOL)
					b = 0xff;
				else
					b*=MUL;*/
			m = ( r + g + b ) / 3;
			if (m > MINCOL)
				if (( r == 0 ) && ( g == 0xff ) && ( b == 0 ))
					*p++ = 0xff;
				else
					*p++ = ( m * MUL > 0xff ) ? 0xff : m * MUL;
			else
				*p++ = 0;
		}
		SDL_UnlockSurface( surf );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;
		for (int i = 0; i < 256; ++i) {
			unsigned int r = data->pal[i].r;
			unsigned int g = data->pal[i].g;
			unsigned int b = data->pal[i].b;
			unsigned int m = (r + g + b) / 3;
			if (m > MINCOL)
				if (( r == 0 ) && ( g == 0xff ) && ( b == 0 ))
					data->pal[i].unused = 0xff;
				else
					data->pal[i].unused = ( m * MUL > 0xff ) ? 0xff : m * MUL;
			else
				data->pal[i].unused = 0;
		}
		data->alpha_pal = true;
	}
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDLVideoDriver::DrawRect(Region& rgn, Color& color, bool fill, bool clipped)
{
	SDL_Rect drect = {
		rgn.x, rgn.y, rgn.w, rgn.h
	};
	if (fill) {
		if ( SDL_ALPHA_TRANSPARENT == color.a ) {
			return;
		} else if ( SDL_ALPHA_OPAQUE == color.a ) {
			long val = SDL_MapRGBA( backBuf->format, color.r, color.g, color.b, color.a );
			SDL_FillRect( backBuf, &drect, val );
		} else {
			SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_HWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
			SDL_Color c;
			c.r = color.r;
			c.b = color.b;
			c.g = color.g;
			SDL_SetPalette( rectsurf, SDL_LOGPAL, &c, 0, 1 );
			SDL_SetAlpha( rectsurf, SDL_SRCALPHA | SDL_RLEACCEL, color.a );
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

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDLVideoDriver::DrawRectSprite(Region& rgn, Color& color, Sprite2D* sprite)
{
	SDL_Surface* surf = ( SDL_Surface* ) sprite->vptr;
	SDL_Rect drect = {
		rgn.x, rgn.y, rgn.w, rgn.h
	};
	if ( SDL_ALPHA_TRANSPARENT == color.a ) {
		return;
	} else if ( SDL_ALPHA_OPAQUE == color.a ) {
		long val = SDL_MapRGBA( surf->format, color.r, color.g, color.b, color.a );
		SDL_FillRect( surf, &drect, val );
	} else {
		SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_HWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
		SDL_Color c;
		c.r = color.r;
		c.b = color.b;
		c.g = color.g;
		SDL_SetPalette( rectsurf, SDL_LOGPAL, &c, 0, 1 );
		SDL_SetAlpha( rectsurf, SDL_SRCALPHA | SDL_RLEACCEL, color.a );
		SDL_BlitSurface( rectsurf, NULL, surf, &drect );
		SDL_FreeSurface( rectsurf );
	}
}

void SDLVideoDriver::SetPixel(short x, short y, Color& color, bool clipped)
{
	//printf("x: %d; y: %d; XC: %d; YC: %d, VX: %d, VY: %d, VW: %d, VH: %d\n", x, y, xCorr, yCorr, Viewport.x, Viewport.y, Viewport.w, Viewport.h);
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
	// FIXME: is it endian safe?
	memcpy( pixels, &val, disp->format->BytesPerPixel );
	SDL_UnlockSurface( backBuf );
}

void SDLVideoDriver::GetPixel(short x, short y, Color* color)
{
	SDL_LockSurface( backBuf );
	unsigned char * pixels = ( ( unsigned char * ) backBuf->pixels ) +
		( ( y * disp->w + x) * disp->format->BytesPerPixel );
	long val = 0;
	memcpy( &val, pixels, disp->format->BytesPerPixel );
	SDL_UnlockSurface( backBuf );

	SDL_GetRGBA( val, backBuf->format, &color->r, &color->g, &color->b, &color->a );
}

// (x,y) is _not_ relative to sprite's (xpos,ypos)
bool SDLVideoDriver::IsSpritePixelTransparent(Sprite2D* sprite, unsigned short x, unsigned short y)
{
	if (x >= sprite->Width || y >= sprite->Height) return true;

	if (!sprite->BAM) {
		SDL_Surface *surf = (SDL_Surface*)(sprite->vptr);

		SDL_LockSurface( surf );
		unsigned char * pixels = ( ( unsigned char * ) surf->pixels ) +
			( ( y * surf->w + x) * surf->format->BytesPerPixel );
		long val = 0;
		memcpy( &val, pixels, surf->format->BytesPerPixel );
		SDL_UnlockSurface( surf );

		return val == 0;
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;

		if (data->flip_ver)
			y = sprite->Height - y - 1;
		if (data->flip_hor)
			x = sprite->Width - x - 1;

		int skipcount = y * sprite->Width + x;

		Uint8* rle = (Uint8*)sprite->pixels;
		if (data->RLE) {
			while (skipcount > 0) {
				if (*rle++ == data->transindex)
					skipcount -= (*rle++)+1;
				else
					skipcount--;
			}
		} else {
			// uncompressed
			rle += skipcount;
			skipcount = 0;
		}
		if (skipcount < 0 || *rle == data->transindex)
			return true;
		else
			return false;
	}
}

/*
 * Draws horizontal line. When clipped=true, it draws the line relative 
 * to Area origin and clips it by Area viewport borders, 
 * else it draws relative to screen origin and ignores the vieport
 */
void SDLVideoDriver::DrawHLine(short x1, short y, short x2, Color& color, bool clipped)
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
	for ( ; x1 <= x2 ; x1++ ) 
		SetPixel( x1, y, color, clipped );
}

/*
 * Draws vertical line. When clipped=true, it draws the line relative 
 * to Area origin and clips it by Area viewport borders, 
 * else it draws relative to screen origin and ignores the vieport
 */
void SDLVideoDriver::DrawVLine(short x, short y1, short y2, Color& color, bool clipped)
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

	for ( ; y1 <= y2 ; y1++ ) 
		SetPixel( x, y1, color, clipped );
}

void SDLVideoDriver::DrawLine(short x1, short y1, short x2, short y2,
	Color& color)
{
	x1 -= Viewport.x;
	x2 -= Viewport.x;
	y1 -= Viewport.y;
	y2 -= Viewport.y;
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
				SetPixel( j >> 16, y1, color );	
				j += decInc;
			}
			return;
		}
		longLen += y1;
		for (int j = 0x8000 + ( x1 << 16 ); y1 >= longLen; --y1) {
			SetPixel( j >> 16, y1, color );	
			j -= decInc;
		}
		return;
	}

	if (longLen > 0) {
		longLen += x1;
		for (int j = 0x8000 + ( y1 << 16 ); x1 <= longLen; ++x1) {
			SetPixel( x1, j >> 16, color );
			j += decInc;
		}
		return;
	}
	longLen += x1;
	for (int j = 0x8000 + ( y1 << 16 ); x1 >= longLen; --x1) {
		SetPixel( x1, j >> 16, color );
		j -= decInc;
	}
}
/** This functions Draws a Circle */
void SDLVideoDriver::DrawCircle(short cx, short cy, unsigned short r,
	Color& color)
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
		SetPixel( cx + ( short ) x, cy + ( short ) y, color );
		SetPixel( cx - ( short ) x, cy + ( short ) y, color );
		SetPixel( cx - ( short ) x, cy - ( short ) y, color );
		SetPixel( cx + ( short ) x, cy - ( short ) y, color );
		SetPixel( cx + ( short ) y, cy + ( short ) x, color );
		SetPixel( cx - ( short ) y, cy + ( short ) x, color );
		SetPixel( cx - ( short ) y, cy - ( short ) x, color );
		SetPixel( cx + ( short ) y, cy - ( short ) x, color );

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
/** This functions Draws an Ellipse */
void SDLVideoDriver::DrawEllipse(short cx, short cy, unsigned short xr,
	unsigned short yr, Color& color, bool clipped)
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

void SDLVideoDriver::DrawPolyline(Gem_Polygon* poly, Color& color, bool fill)
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

			Uint8* line = (Uint8*)(backBuf->pixels) + (y_top+yCorr)*backBuf->pitch;

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
		DrawLine( lastX, lastY, poly->points[i].x, poly->points[i].y, color );
		lastX = poly->points[i].x;
		lastY = poly->points[i].y;
	}
	DrawLine( lastX, lastY, poly->points[0].x, poly->points[0].y, color );

	return;
}

/** Frees a Palette */
void SDLVideoDriver::FreePalette(Color *&palette)
{
	if(palette) {
		free(palette);
		palette = NULL;
	}
}

/** Creates a Palette from Color */
Color* SDLVideoDriver::CreatePalette(Color color, Color back)
{
	Color* pal = ( Color* ) malloc( 256 * sizeof( Color ) );
	pal[0].r = 0;
	pal[0].g = 0xff;
	pal[0].b = 0;
	pal[0].a = 0;
	for (int i = 1; i < 256; i++) {
		pal[i].r = back.r +
			( unsigned char ) ( ( ( color.r - back.r ) * ( i ) ) / 255.0 );
		pal[i].g = back.g +
			( unsigned char ) ( ( ( color.g - back.g ) * ( i ) ) / 255.0 );
		pal[i].b = back.b +
			( unsigned char ) ( ( ( color.b - back.b ) * ( i ) ) / 255.0 );
		pal[i].a = 0;
	}
	return pal;
}
/** Blits a Sprite filling the Region */
void SDLVideoDriver::BlitTiled(Region rgn, Sprite2D* img, bool anchor)
{
	if (!anchor) {
		rgn.x -= Viewport.x;
		rgn.y -= Viewport.y;
	}
	int xrep = ( rgn.w + img->Width - 1 ) / img->Width;
	int yrep = ( rgn.h + img->Height - 1 ) / img->Height;
	for (int y = 0; y < yrep; y++) {
		for (int x = 0; x < xrep; x++) {
			BlitSprite(img, rgn.x + (x*img->Width), rgn.y + (y*img->Height),
					   false, &rgn);
		}
	}
}
/** Send a Quit Signal to the Event Queue */
bool SDLVideoDriver::Quit()
{
	SDL_Event evnt;
	evnt.type = SDL_QUIT;
	if (SDL_PushEvent( &evnt ) == -1) {
		return false;
	}
	return true;
}
/** Get the Palette of a Sprite */
Color* SDLVideoDriver::GetPalette(Sprite2D* spr)
{
	if (!spr->BAM) {
		SDL_Surface* s = ( SDL_Surface* ) spr->vptr;
		if (s->format->BitsPerPixel != 8) {
			return NULL;
		}
		Color* pal = ( Color* ) malloc( 256 * sizeof( Color ) );
		for (int i = 0; i < s->format->palette->ncolors; i++) {
			pal[i].r = s->format->palette->colors[i].r;
			pal[i].g = s->format->palette->colors[i].g;
			pal[i].b = s->format->palette->colors[i].b;
		}
		return pal;
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;
		Color* pal = ( Color* ) malloc( 256 * sizeof( Color ) );
		for (int i = 0; i < 256; i++) {
			pal[i].r = data->pal[i].r;
			pal[i].g = data->pal[i].g;
			pal[i].b = data->pal[i].b;
		}
		return pal;
	}
}

// Flips given sprite vertically (up-down). If MirrorAnchor=true,
// flips its anchor (i.e. origin//base point) as well
// returns new sprite

Sprite2D *SDLVideoDriver::MirrorSpriteVertical(Sprite2D* sprite, bool MirrorAnchor)
{
	if (!sprite || !sprite->vptr)
		return NULL;

	Sprite2D* dest = 0;


	if (!sprite->BAM) {
		SDL_Surface* tmp = ( SDL_Surface* ) sprite->vptr;
		unsigned char *newpixels = (unsigned char*) malloc( sprite->Width*sprite->Height );

		SDL_LockSurface( tmp );
		memcpy(newpixels, sprite->pixels, sprite->Width*sprite->Height);
		dest = CreateSprite8(sprite->Width, sprite->Height, 8,
							 newpixels, tmp->format->palette->colors, true, 0);
		
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
		SDL_UnlockSurface( tmp );
	} else {
		// TODO: use refcounting on the data to avoid copying it
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;
		Uint8* rledata = (Uint8*)malloc(data->datasize);
		memcpy(rledata, sprite->pixels, data->datasize);
		dest = CreateSpriteBAM8(sprite->Width, sprite->Height, data->RLE,
								rledata, data->datasize, data->pal,
								data->transindex);
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_ver = !data->flip_ver;
		destdata->flip_hor = data->flip_hor;
	}

	dest->XPos = dest->XPos;
	if (MirrorAnchor)
		dest->YPos = sprite->Height - sprite->YPos;
	else
		dest->YPos = sprite->YPos;
	
	return dest;
}

// Flips given sprite horizontally (left-right). If MirrorAnchor=true,
//   flips its anchor (i.e. origin//base point) as well
Sprite2D *SDLVideoDriver::MirrorSpriteHorizontal(Sprite2D* sprite, bool MirrorAnchor)
{
	if (!sprite || !sprite->vptr)
		return NULL;

	Sprite2D* dest = 0;

	if (!sprite->BAM) {
		SDL_Surface* tmp = ( SDL_Surface* ) sprite->vptr;
		unsigned char *newpixels = (unsigned char*) malloc( sprite->Width*sprite->Height );

		SDL_LockSurface( tmp );
		memcpy(newpixels, sprite->pixels, sprite->Width*sprite->Height);
		dest = CreateSprite8(sprite->Width, sprite->Height, 8,
							 newpixels, tmp->format->palette->colors, true, 0);
		
		for (int y = 0; y < dest->Height; y++) {
			unsigned char * dst = (unsigned char *) dest->pixels + ( y * dest->Width );
			unsigned char * src = dst + dest->Width - 1;
			for (int x = 0; x < dest->Width / 2; x++) {
				unsigned char swp=*dst;
				*dst++ = *src;
				*src-- = swp;
			}
		}
		SDL_UnlockSurface( tmp );
	} else {
		// TODO: use refcounting on the data to avoid copying it
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;
		Uint8* rledata = (Uint8*)malloc(data->datasize);
		memcpy(rledata, sprite->pixels, data->datasize);
		dest = CreateSpriteBAM8(sprite->Width, sprite->Height, data->RLE,
								rledata, data->datasize, data->pal,
								data->transindex);
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_ver = data->flip_ver;
		destdata->flip_hor = !data->flip_hor;
	}

	if (MirrorAnchor)
		dest->XPos = sprite->Width - sprite->XPos;
	else
		dest->XPos = sprite->XPos;
	dest->YPos = sprite->YPos;
	
	return dest;
}

void SDLVideoDriver::CreateAlpha( Sprite2D *sprite)
{
	// Note: we convert BAM sprites back to non-BAM (SDL_Surface) sprites
	// for per-pixel non-indexed alpha.

	if (!sprite || !sprite->vptr)
		return;

	unsigned int *pixels = (unsigned int *) malloc (sprite->Width * sprite->Height * 4);
	int i=0;
	for (int y = 0; y < sprite->Height; y++) {
		for (int x = 0; x < sprite->Width; x++) {
			int sum = 0;
			int cnt = 0;
			for (int xx=x-2;xx<x+2;xx++) {
				for(int yy=y-2;yy<y+2;yy++) {
					if (xx < 0 || xx >= sprite->Width) continue;
					if (yy < 0 || yy >= sprite->Height) continue;
					cnt++;
					if (IsSpritePixelTransparent(sprite, xx, yy))
						sum++;
				}
			}
			int tmp=255 - (sum * 255 / cnt);
			pixels[i++]=tmp;
		}
	}
	if ( sprite->pixels ) {
		free (sprite->pixels);
	}
	if (sprite->BAM) {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;
		delete data;
		sprite->BAM = false;
	} else {
		SDL_FreeSurface ((SDL_Surface*)sprite->vptr);
	}
	sprite->pixels = pixels;
	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom( pixels, sprite->Width, sprite->Height, 32, sprite->Width*4, 0xff00, 0xff00, 0xff00, 255 );
	sprite->vptr = surf;
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

void SDLVideoDriver::SetClipRect(Region* clip)
{
	if (clip) {
		SDL_Rect tmp;
		SDL_Rect* rect = &tmp;
		rect->x = clip->x;
		rect->y = clip->y;
		rect->w = clip->w;
		rect->h = clip->h;
		SDL_SetClipRect( backBuf, rect );
	} else {
		SDL_SetClipRect( backBuf, NULL );
	}
}

void SDLVideoDriver::MouseMovement(int x, int y)
{
	GetTime( lastMouseTime );
	if (DisableMouse)
		return;
	CursorPos.x = x; // - mouseAdjustX[CursorIndex];
	CursorPos.y = y; // - mouseAdjustY[CursorIndex];
	if (DisableScroll) {
		moveX = 0;
		moveY = 0;
	} else {
		if (x <= 1)
			moveX = -5;
		else {
			if (event.motion.x >= ( core->Width - 1 ))
				moveX = 5;
			else
				moveX = 0;
		}
		if (y <= 1)
			moveY = -5;
		else {
			if (y >= ( core->Height - 1 ))
				moveY = 5;
			else
				moveY = 0;
		}
	}
	if (Evnt)
		Evnt->MouseMove(x, y);
}

/* no idea how elaborate this should be*/
void SDLVideoDriver::MoveMouse(unsigned int x, unsigned int y)
{
	SDL_WarpMouse(x,y);
}

void SDLVideoDriver::InitMovieScreen(int &w, int &h)
{
	SDL_LockSurface( disp );
	memset( disp->pixels, 0,
		disp->w * disp->h * disp->format->BytesPerPixel );
	SDL_UnlockSurface( disp );
	SDL_Flip( disp );
	w = disp->w;
	h = disp->h;
	//setting the subtitle region to the bottom 1/10th of the screen
	subtitleregion.w = w;
	subtitleregion.h = h/10;
	subtitleregion.x = 0;
	subtitleregion.y = h-h/10;
}

void SDLVideoDriver::showFrame(unsigned char* buf, unsigned int bufw,
	unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
	unsigned int h, unsigned int dstx, unsigned int dsty, 
	int g_truecolor, unsigned char *pal, ieDword titleref)
{
	int i;
	SDL_Surface* sprite;
	SDL_Rect srcRect, destRect;

	assert( bufw == w && bufh == h );

	if (g_truecolor) {
		sprite = SDL_CreateRGBSurfaceFrom( buf, bufw, bufh, 16, 2 * bufw,
		                        0x7C00, 0x03E0, 0x001F, 0 );
	} else {
		sprite = SDL_CreateRGBSurfaceFrom( buf, bufw, bufh, 8, bufw, 0x7C00,
		                        0x03E0, 0x001F, 0 );

		for (i = 0; i < 256; i++) {
		        sprite->format->palette->colors[i].r = ( *pal++ ) << 2;
		        sprite->format->palette->colors[i].g = ( *pal++ ) << 2;
		        sprite->format->palette->colors[i].b = ( *pal++ ) << 2;
		        sprite->format->palette->colors[i].unused = 0;
		}
	}

	srcRect.x = sx;
	srcRect.y = sy;
	srcRect.w = w;
	srcRect.h = h;
	destRect.x = dstx;
	destRect.y = dsty;
	destRect.w = w;
	destRect.h = h;

	SDL_BlitSurface( sprite, &srcRect, disp, &destRect );
	if (titleref>0)
		DrawMovieSubtitle( titleref );
	SDL_Flip( disp );
	SDL_FreeSurface( sprite );
}

int SDLVideoDriver::PollMovieEvents()
{
        SDL_Event event;

        while (SDL_PollEvent( &event )) {
                switch (event.type) {
                        case SDL_QUIT:
                        case SDL_MOUSEBUTTONDOWN:
                                return 1;
                        case SDL_KEYDOWN:
                                switch (event.key.keysym.sym) {
                                        case SDLK_ESCAPE:
                                        case SDLK_q:
                                                return 1;
                                        case SDLK_f:
                                                SDL_WM_ToggleFullScreen( disp );
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

void SDLVideoDriver::SetMovieFont(Font *stfont, Color *pal)
{
	subtitlefont = stfont;
	subtitlepal = pal;
}

void SDLVideoDriver::DrawMovieSubtitle(ieDword strRef)
{
	if (strRef!=subtitlestrref) {
		core->FreeString(subtitletext);
		subtitletext = core->GetString(strRef);
		subtitlestrref = strRef;
		printf("Fetched subtitle %s\n", subtitletext);
	}
	if (subtitlefont) {
		subtitlefont->Print(subtitleregion, (unsigned char *) subtitletext, subtitlepal, IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_BOTTOM, true);
	}
}
