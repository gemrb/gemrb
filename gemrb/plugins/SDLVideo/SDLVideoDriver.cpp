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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "SDLVideoDriver.h"
#include "../Core/Interface.h"
#include <cmath>
#include <cassert>
#include "../Core/SpriteCover.h"
#include "../Core/Console.h"
#include "../Core/Audio.h"
#include "../Core/Palette.h"
#include "../Core/AnimationFactory.h"

class Sprite2D_BAM_Internal {
public:
	Sprite2D_BAM_Internal() { pal = 0; }
	~Sprite2D_BAM_Internal() { if (pal) { pal->Release(); pal = 0; } }

	Palette* pal;
	bool RLE;
	int transindex;
	bool flip_hor;
	bool flip_ver;

	// The AnimationFactory in which the data for this sprite is stored.
	// (Used for refcounting of the data.)
	AnimationFactory* source;
};

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
	DisableMouse = 0;
	DisableScroll = false;
	xCorr = 0;
	yCorr = 0;
	lastTime = 0;
	GetTime( lastMouseTime );
	backBuf=NULL;
	extra=NULL;
	subtitlestrref = 0;
	subtitletext = NULL;
	mousescrollspd = 10;
	scrolling = false;
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
	assert(Cursor[2] == NULL);
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

int SDLVideoDriver::CreateDisplay(int w, int h, int b, bool fs)
{
	width=w;
	height=h;
	bpp=b;
	fullscreen=fs;
	printMessage( "SDLVideo", "Creating display\n", WHITE );
	ieDword flags = SDL_SWSURFACE;// | SDL_DOUBLEBUF;
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
	ieDword flags = SDL_SWSURFACE;
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
	ieDword flags = SDL_SWSURFACE;
	if (vm.GetFullScreen()) {
		flags |= SDL_FULLSCREEN;
	}
	if (SDL_VideoModeOK( vm.GetWidth(), vm.GetHeight(), vm.GetBPP(), flags ) == vm.GetBPP()) {
		return true;
	}
	return false;
}

bool SDLVideoDriver::ToggleFullscreenMode(int set_reset)
{
	if (set_reset==-1) {
		 set_reset=fullscreen;
	}
	if (fullscreen == !set_reset) {
		fullscreen=set_reset;
		ieDword flags = SDL_SWSURFACE;// | SDL_DOUBLEBUF;
		if (fullscreen) {
			flags |= SDL_FULLSCREEN;
		}
		SDL_Surface* disp2 = SDL_SetVideoMode( width, height, bpp, flags );
		if (disp2) {
			disp = disp2;
			MoveMouse(CursorPos.x,CursorPos.y);
			return true;
		}
	}
	return false;
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
	static bool lastevent = false; /* last event was a mousedown */
	static unsigned long lastmousetime = 0;
	int x,y;

	int ret = GEM_OK;
	unsigned long time;
	GetTime( time );
	if (( time - lastTime ) < 17) {
		SDL_Delay( 17 - (time - lastTime) );
		return ret;
	}
	lastTime = time;

	unsigned char key = 0;
	bool ConsolePopped = core->ConsolePopped;

	if (ConsolePopped) {
		core->DrawConsole();
	}

	while ( SDL_PollEvent(&event) ) {
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
					if (event.key.keysym.sym<256) {
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
				case SDLK_KP_ENTER:
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
			lastevent = false;
			MouseMovement(event.motion.x, event.motion.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (DisableMouse || !Evnt) //grayed mouse is disabled in this sense
				break;
			lastevent = true;
			lastmousetime=Evnt->GetRKDelay();
			if (lastmousetime != (unsigned long) ~0) {
				lastmousetime += lastmousetime+time;
			}
			if (CursorIndex != 2)
				CursorIndex = 1;
			CursorPos.x = event.button.x; // - mouseAdjustX[CursorIndex];
			CursorPos.y = event.button.y; // - mouseAdjustY[CursorIndex];
			if (!ConsolePopped)
				Evnt->MouseDown( event.button.x, event.button.y, 1 << ( event.button.button - 1 ), GetModState(SDL_GetModState()) );

			break;

		case SDL_MOUSEBUTTONUP:
			lastevent = false;
			if (DisableMouse || !Evnt) //grayed mouse is disabled in this sense
				break;
			if (CursorIndex != 2)
				CursorIndex = 0;
			CursorPos.x = event.button.x;
			CursorPos.y = event.button.y;
			if (!ConsolePopped)
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

	if (Evnt && !DisableMouse && lastevent && time>lastmousetime && SDL_GetMouseState(&x,&y)==SDL_BUTTON(1)) {
		lastmousetime=time+Evnt->GetRKDelay();
		if (!ConsolePopped)
			Evnt->MouseUp( x, y, 1 << ( 0 ), GetModState(SDL_GetModState()) );
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

	if (Cursor[CursorIndex] && !(DisableMouse&MOUSE_DISABLED)) {
		SDL_Surface* temp = backBuf;
		backBuf = disp; // FIXME: UGLY HACK!
		if (DisableMouse&MOUSE_GRAYED) {
			//used for greyscale blitting, fadeColor is unused
			BlitGameSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, BLIT_GREY, fadeColor, NULL, NULL, NULL, true);
		} else {
			BlitSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, true);
		}
		backBuf = temp;
	}

	//handle tooltips
	unsigned int delay = core->TooltipDelay;
	// The multiplication by 10 is there since the last, disabling slider position is the eleventh
	if (!ConsolePopped && (delay<TOOLTIP_DELAY_FACTOR*10) ) {
		GetTime( time );
		/** Display tooltip if mouse is idle */
		if (( time - lastMouseTime ) > delay) {
			if (Evnt)
				Evnt->MouseIdle( time - lastMouseTime );
		}

		/** This causes the tooltip to be rendered directly to display */
		SDL_Surface* tmp = backBuf;
		backBuf = disp; // FIXME: UGLY HACK!
		core->DrawTooltip();
		backBuf = tmp;
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
	sc->pixels = 0;
}


// flags: 0 - never dither (full cover)
//	1 - dither if polygon wants it
//	2 - always dither
void SDLVideoDriver::AddPolygonToSpriteCover(SpriteCover* sc,
											 Wall_Polygon* poly)
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
	spr->Bpp = bpp;
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
	spr->Bpp = bpp;
	return spr;
}

Sprite2D* SDLVideoDriver::CreateSpriteBAM8(int w, int h, bool rle,
											 const unsigned char* pixeldata,
											 AnimationFactory* datasrc,
											 Palette* palette, int transindex)
{
	Sprite2D* spr = new Sprite2D();
	spr->BAM = true;
	Sprite2D_BAM_Internal* data = new Sprite2D_BAM_Internal;
	spr->vptr = data;

	palette->IncRef();
	data->pal = palette;
	data->transindex = transindex;
	data->flip_hor = false;
	data->flip_ver = false;
	data->RLE = rle;
	data->source = datasrc;
	datasrc->IncDataRefCount();

	spr->pixels = (const void*)pixeldata;
	spr->Width = w;
	spr->Height = h;
	spr->Bpp = 8; // FIXME!!!!

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

Sprite2D* SDLVideoDriver::DuplicateSprite(Sprite2D* sprite)
{
	if (!sprite) return NULL;
	Sprite2D* dest = 0;

	if (!sprite->BAM) {
		SDL_Surface* tmp = ( SDL_Surface* ) sprite->vptr;
		unsigned char *newpixels = (unsigned char*) malloc( sprite->Width*sprite->Height );

		SDL_LockSurface( tmp );
		memcpy(newpixels, sprite->pixels, sprite->Width*sprite->Height);
		dest = CreateSprite8(sprite->Width, sprite->Height, 8,
							 newpixels, tmp->format->palette->colors, true, 0);
		SDL_UnlockSurface( tmp );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;
		const Uint8* rledata;

		rledata = (const Uint8*)sprite->pixels;

		dest = CreateSpriteBAM8(sprite->Width, sprite->Height, data->RLE,
								rledata, data->source, data->pal,
								data->transindex);
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_ver = data->flip_ver;
		destdata->flip_hor = data->flip_hor;
	}

	return dest;
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

		const Uint8* rle = (const Uint8*)spr->pixels;
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
#define PAL data->pal
#undef COVER
#undef TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16
			if (data->pal->alpha) {

#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			} else {

#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			}

		} else {

#define BPP16
			if (data->pal->alpha) {

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

		const Uint8* rle = (const Uint8*)spr->pixels;
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
#define PAL data->pal
#undef COVER
#undef TINT

		if (backBuf->format->BytesPerPixel == 4) {

#undef BPP16
			if (data->pal->alpha) {

#define PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			} else {

#undef PALETTE_ALPHA
#include "SDLVideoDriver.inl"

			}

		} else {

#define BPP16
			if (data->pal->alpha) {

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

void SDLVideoDriver::BlitSpriteHalfTrans(Sprite2D* spr, int x, int y,
										 bool anchor, Region* clip)
{
	if (!spr->vptr) return;

	if (!spr->BAM) {
		SDL_Surface* surf = ( SDL_Surface * ) spr->vptr;
		SDL_SetAlpha(surf, SDL_SRCALPHA | SDL_RLEACCEL, 128);
		BlitSprite(spr, x, y, anchor, clip);
		SDL_SetAlpha(surf, SDL_RLEACCEL, 0);
	} else {
		printMessage( "SDLVideo", "HalfTrans blit not supported for this sprite\n", LIGHT_RED );

	}
}

//cannot make const reference from tint, it is modified locally
void SDLVideoDriver::BlitGameSprite(Sprite2D* spr, int x, int y,
									unsigned int flags, Color tint,
									SpriteCover* cover, Palette *palette,
									Region* clip, bool anchor)
{
	if (!spr->vptr) return;
	if (!spr->BAM) {
		printMessage( "SDLVideo", "BlitGameSprite not supported for this sprite\n", LIGHT_RED );
		BlitSprite(spr, x, y, false, clip);
		return;
	}

	Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;

	if (!palette)
		palette = data->pal;

	// implicit flags:
	const unsigned int blit_COVERED =      0x20000000U;
	const unsigned int blit_TINTALPHA =    0x40000000U;
	const unsigned int blit_PALETTEALPHA = 0x80000000U;

	if (cover) flags |= blit_COVERED;
	if ((flags & BLIT_TINTED) && tint.a != 255) flags |= blit_TINTALPHA;
	if (palette->alpha) flags |= blit_PALETTEALPHA;

	// flag combinations which are often used:
	// (ignoring MIRRORX/Y since those are handled conditionally)

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


	const Uint8* rle = (const Uint8*)spr->pixels;
	//int tx = x - spr->XPos - Viewport.x;
	//int ty = y - spr->YPos - Viewport.y;
	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	if (!anchor) {
		tx-=Viewport.x;
		ty-=Viewport.y;
	}
	if (tx > backBuf->w) return;
	if (tx+spr->Width <= 0) return;
	SDL_LockSurface(backBuf);

	bool hflip = data->flip_hor;
	bool vflip = data->flip_ver;
	if (flags & BLIT_MIRRORX) hflip = !hflip;
	if (flags & BLIT_MIRRORY) vflip = !vflip;

	Uint32 shadowcol32 = 0, mask32;
	Uint16 shadowcol16 = 0, mask16;

	if (flags & BLIT_TRANSSHADOW) {
		shadowcol32 = SDL_MapRGBA(backBuf->format, palette->col[1].r/2,
									palette->col[1].g/2, palette->col[1].b/2, 0);
		shadowcol16 = (Uint16)shadowcol32;
	}

	mask32 = (backBuf->format->Rmask >> 1) & backBuf->format->Rmask;
	mask32 |= (backBuf->format->Gmask >> 1) & backBuf->format->Gmask;
	mask32 |= (backBuf->format->Bmask >> 1) & backBuf->format->Bmask;
	mask16 = (Uint16)mask32;

	unsigned int remflags = flags & ~(BLIT_MIRRORX | BLIT_MIRRORY);
	if (remflags & BLIT_NOSHADOW) remflags &= ~BLIT_TRANSSHADOW;

#define FLIP
#define HFLIP_CONDITIONAL hflip
#define VFLIP_CONDITIONAL vflip
#define RLE data->RLE
#define PAL palette
#define COVERX (cover->XPos - spr->XPos)
#define COVERY (cover->YPos - spr->YPos)
#undef TINT_ALPHA
#undef PALETTE_ALPHA

	if (remflags == (blit_COVERED | BLIT_TINTED)) {

#define COVER
#define SPECIALPIXEL
#define TINT

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}

#undef COVER
#undef TINT
#undef SPECIALPIXEL

	} else if (remflags == (blit_COVERED | BLIT_TINTED | BLIT_TRANSSHADOW)) {

#define COVER
#define TINT

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#define SPECIALPIXEL if (p == 1) { *pix = ((*pix >> 1)&mask32) + shadowcol32; } else
#include "SDLVideoDriver.inl"
#undef SPECIALPIXEL
		} else {
#define BPP16
#define SPECIALPIXEL if (p == 1) { *pix = ((*pix >> 1)&mask16) + shadowcol16; } else
#include "SDLVideoDriver.inl"
#undef SPECIALPIXEL
		}

#undef COVER
#undef TINT

	} else if (remflags == (blit_COVERED | BLIT_TINTED | BLIT_NOSHADOW)) {

#define COVER
#define TINT
#define SPECIALPIXEL if (p != 1)

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}

#undef SPECIALPIXEL
#undef COVER
#undef TINT


	} else if (remflags == BLIT_TINTED) {

#define SPECIALPIXEL
#define TINT

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}

#undef TINT
#undef SPECIALPIXEL

	} else if (remflags == BLIT_HALFTRANS) {

#define HALFALPHA
#define SPECIALPIXEL

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}

#undef HALFALPHA
#undef SPECIALPIXEL

	} else if (remflags == (blit_COVERED | BLIT_HALFTRANS)) {

#define HALFALPHA
#define COVER
#define SPECIALPIXEL

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}

#undef HALFALPHA
#undef COVER
#undef SPECIALPIXEL

	} else if (remflags == blit_COVERED) {

#define COVER
#define SPECIALPIXEL

		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}

#undef COVER
#undef SPECIALPIXEL

	}/* else if (remflags == 0) {

#define SPECIALPIXEL
		if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
		} else {
#define BPP16
#include "SDLVideoDriver.inl"
		}
#undef SPECIALPIXEL

}*/ else {
		// handling the following effects with conditionals:
		// halftrans
		// noshadow
		// transshadow
		// grey
		// red
		// glow (not yet)
		// blended (not yet)

		// handling the following effects by repeated inclusion:
		// palettealpha
		// tinted
		// covered

//		printf("Unoptimized blit: %04X\n", flags);

#define SPECIALPIXEL   int ia=0; if ((remflags & BLIT_HALFTRANS) || (p == 1 && (remflags & BLIT_TRANSSHADOW))) ia = 1; if (p == 1 && (remflags & BLIT_NOSHADOW)) { } else

#define CUSTOMBLENDING
#define RVALUE(r,g,b) (r)
#define GVALUE(r,g,b) (g)
#define BVALUE(r,g,b) (b)
#define AVALUE(r,g,b,a) (a)>>ia
#define CUSTOMBLEND(r,g,b) do { if (remflags & BLIT_GREY) { unsigned int t = (r)+(g)+(b); t /= 3; (r)=t; (g)=t; (b)=t; } if (remflags & BLIT_RED) { (g) /= 2; (b) /= 2; } } while(0)

#define TINT_ALPHA

		if (!(remflags & BLIT_TINTED)) tint.a = 255;

		if (remflags & blit_PALETTEALPHA) {
#define PALETTE_ALPHA
			if (remflags & blit_COVERED) {
#define COVER
				if (remflags & BLIT_TINTED) {
#define TINT
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
#undef TINT
				} else {
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
				}
#undef COVER
			} else {
				if (remflags & BLIT_TINTED) {
#define TINT
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
#undef TINT
				} else {
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
				}
			}
#undef PALETTE_ALPHA
		} else {
			if (remflags & blit_COVERED) {
#define COVER
				if (remflags & BLIT_TINTED) {
#define TINT
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
#undef TINT
				} else {
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
				}
#undef COVER
			} else {
				if (remflags & BLIT_TINTED) {
#define TINT
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
#undef TINT
				} else {
					if (backBuf->format->BytesPerPixel == 4) {
#undef BPP16
#include "SDLVideoDriver.inl"
					} else {
#define BPP16
#include "SDLVideoDriver.inl"
					}
				}
			}
		}

#undef SPECIALPIXEL
#undef CUSTOMBLENDING
#undef RVALUE
#undef GVALUE
#undef BVALUE
#undef AVALUE
#undef CUSTOMBLEND
#undef TINT_ALPHA

	}

#undef FLIP
#undef HFLIP_CONDITIONAL
#undef VFLIP_CONDITIONAL
#undef RLE
#undef PAL
#undef COVERX
#undef COVERY

	SDL_UnlockSurface(backBuf);

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
	FreeSprite(Cursor[2]);
	if (drag) {
		Cursor[2] = drag;
		CursorIndex = 2;
	} else {
		CursorIndex = 0;
		Cursor[2] = NULL;
	}
}

Sprite2D* SDLVideoDriver::GetScreenshot( Region r )
{
	int Width = r.w ? r.w : disp->w;
	int Height = r.h ? r.h : disp->h;
	SDL_Rect src = {r.x, r.y, r.w, r.h};


	SDL_Surface* surf = SDL_CreateRGBSurface( SDL_SWSURFACE, Width, Height, 24,
				0xFF0000, 0x00FF00, 0x0000FF, 0x000000 );
	SDL_BlitSurface( backBuf, (r.w && r.h) ? &src : NULL, surf, NULL);
	void* pixels = malloc( Width * Height * 3 );
	memcpy( pixels, surf->pixels, Width * Height * 3 );
	//freeing up temporary surface as we copied its pixels
	Sprite2D* screenshot = CreateSprite( Width, Height, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, pixels, false, 0 );
	SDL_FreeSurface(surf);
	return screenshot;
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

void SDLVideoDriver::MoveViewportTo(int x, int y)
{
	if (x != Viewport.x || y != Viewport.y) {
		core->GetAudioDrv()->UpdateListenerPos( (x - xCorr) + disp->w / 2, (y - yCorr) + disp->h / 2 );
		Viewport.x = x;
		Viewport.y = y;
	}
}
/** No descriptions */
void SDLVideoDriver::SetPalette(Sprite2D* spr, Palette* pal)
{
	if (!spr->BAM) {
		SDL_Surface* sur = ( SDL_Surface* ) spr->vptr;
		SDL_SetPalette( sur, SDL_LOGPAL, ( SDL_Color * ) pal->col, 0, 256 );
	} else {
		if (!spr->vptr) return;
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;
		data->pal->Release();
		pal->IncRef();
		data->pal = pal;
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
		free( (void*)sprite->pixels );
		sprite->pixels = NULL;
		sprite->vptr = ns;
	}
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDLVideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill, bool clipped)
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
			SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
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

/** This function Draws a clipped sprite */
void SDLVideoDriver::DrawRectSprite(const Region& rgn, const Color& color, Sprite2D* sprite)
{
	if (sprite->BAM) {
		printMessage( "SDLVideo", "DrawRectSprite not supported for this sprite\n", LIGHT_RED );
		return;
	}

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
		SDL_Surface * rectsurf = SDL_CreateRGBSurface( SDL_SWSURFACE | SDL_SRCALPHA, rgn.w, rgn.h, 8, 0, 0, 0, 0 );
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

void SDLVideoDriver::SetPixel(short x, short y, const Color& color, bool clipped)
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

Color SDLVideoDriver::SpriteGetPixel(Sprite2D* sprite, unsigned short x, unsigned short y)
{
	Color c = { 0, 0, 0, 0 };

	if (x >= sprite->Width || y >= sprite->Height) return c;

	if (!sprite->BAM) {
		SDL_Surface *surf = (SDL_Surface*)(sprite->vptr);

		SDL_LockSurface( surf );
		unsigned char * pixels = ( ( unsigned char * ) surf->pixels ) +
			( ( y * surf->w + x) * surf->format->BytesPerPixel );
		long val = 0;
		memcpy( &val, pixels, surf->format->BytesPerPixel );
		SDL_UnlockSurface( surf );

		SDL_GetRGBA( val, surf->format, &c.r, &c.g, &c.b, &c.a );
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)sprite->vptr;

		if (data->flip_ver)
			y = sprite->Height - y - 1;
		if (data->flip_hor)
			x = sprite->Width - x - 1;

		int skipcount = y * sprite->Width + x;

		const Uint8* rle = (const Uint8*)sprite->pixels;
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

		if (skipcount >= 0 && *rle != data->transindex) {
			c = data->pal->col[*rle];
			//c.r = data->pal->col[*rle].r;
			//c.g = data->pal->col[*rle].g;
			//c.b = data->pal->col[*rle].b;
			c.a = 0xff;
		}
	}
	return c;
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

		const Uint8* rle = (const Uint8*)sprite->pixels;
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
		DrawLine( lastX, lastY, poly->points[i].x, poly->points[i].y, color, true );
		lastX = poly->points[i].x;
		lastY = poly->points[i].y;
	}
	DrawLine( lastX, lastY, poly->points[0].x, poly->points[0].y, color, true );

	return;
}

/** Blits a Sprite filling the Region */
/** This method may be moved to Video.cpp, it is independent of SDL */
void SDLVideoDriver::BlitTiled(Region rgn, Sprite2D* img, bool anchor)
{
	int xrep = ( rgn.w + img->Width - 1 ) / img->Width;
	int yrep = ( rgn.h + img->Height - 1 ) / img->Height;
	for (int y = 0; y < yrep; y++) {
		for (int x = 0; x < xrep; x++) {
			BlitSprite(img, rgn.x + (x*img->Width),
				 rgn.y + (y*img->Height), anchor, &rgn);
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
Palette* SDLVideoDriver::GetPalette(Sprite2D* spr)
{
	if (!spr->BAM) {
		SDL_Surface* s = ( SDL_Surface* ) spr->vptr;
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
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)spr->vptr;
		data->pal->IncRef();
		return data->pal;
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
		dest = DuplicateSprite(sprite);
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
		dest = DuplicateSprite(sprite);
		Sprite2D_BAM_Internal* destdata = (Sprite2D_BAM_Internal*)dest->vptr;
		destdata->flip_ver = !destdata->flip_ver;
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
		dest = DuplicateSprite(sprite);
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
		dest = DuplicateSprite(sprite);
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

Color SDLVideoDriver::SpriteGetPixelSum(Sprite2D* sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio)
{
	Color sum;
	unsigned int count = ratio*ratio;
	unsigned int r=0, g=0, b=0, a=0;

	for (unsigned int x = 0; x < ratio; x++) {
		for (unsigned int y = 0; y < ratio; y++) {
			Color c = SpriteGetPixel( sprite, xbase*ratio+x, ybase*ratio+y );
			r += c.r;
			g += c.g;
			b += c.b;
			a += c.a;
		}
	}

	sum.r = r / count;
	sum.g = g / count;
	sum.b = b / count;
	sum.a = a / count;

	return sum;
}

Sprite2D* SDLVideoDriver::SpriteScaleDown( Sprite2D* sprite, unsigned int ratio )
{
	unsigned int Width = sprite->Width / ratio;
	unsigned int Height = sprite->Height / ratio;

	void* pixels = malloc( Width * Height * 4 );
	int i = 0;

	for (unsigned int y = 0; y < Height; y++) {
		for (unsigned int x = 0; x < Width; x++) {
			Color c = SpriteGetPixelSum( sprite, x, y, ratio );

			*((char*)pixels + i++) = c.r;
			*((char*)pixels + i++) = c.g;
			*((char*)pixels + i++) = c.b;
			*((char*)pixels + i++) = c.a;
		}
	}

	Sprite2D* small = CreateSprite( Width, Height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, pixels, false, 0 );

	small->XPos = sprite->XPos / ratio;
	small->YPos = sprite->YPos / ratio;

	return small;
}

Sprite2D* SDLVideoDriver::CreateAlpha( Sprite2D *sprite)
{
	if (!sprite)
		return 0;

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
	return CreateSprite( sprite->Width, sprite->Height, 32, 0xFF000000,
	                     0x00FF0000, 0x0000FF00, 0x000000FF, pixels );
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

#define SCROLL_BORDER  5

void SDLVideoDriver::MouseMovement(int x, int y)
{
	GetTime( lastMouseTime );
	if (DisableMouse&MOUSE_DISABLED)
		return;
	CursorPos.x = x; // - mouseAdjustX[CursorIndex];
	CursorPos.y = y; // - mouseAdjustY[CursorIndex];
	if (DisableScroll) {
		moveX = 0;
		moveY = 0;
	} else {
		if (x <= SCROLL_BORDER)
			moveX = -mousescrollspd;
		else {
			if (event.motion.x >= ( core->Width - SCROLL_BORDER ))
				moveX = mousescrollspd;
			else
				moveX = 0;
		}
		if (y <= SCROLL_BORDER)
			moveY = -mousescrollspd;
		else {
			if (y >= ( core->Height - SCROLL_BORDER ))
				moveY = mousescrollspd;
			else
				moveY = 0;
		}
	}

	if (moveX != 0 || moveY != 0) {
		scrolling = true;
		CursorPosition = moveX + moveY;
	} else if (scrolling) {
		scrolling = false;
		SetDragCursor(NULL);
	}

	if (Evnt)
		Evnt->MouseMove(x, y);
}

/* no idea how elaborate this should be*/
void SDLVideoDriver::MoveMouse(unsigned int x, unsigned int y)
{
	SDL_WarpMouse(x,y);
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

void SDLVideoDriver::MouseClickEvent(Uint8 type, Uint8 button)
{
	SDL_MouseButtonEvent *event = new SDL_MouseButtonEvent();
	event->type = type;
	event->button = button;
	event->state = (type==SDL_MOUSEBUTTONDOWN)?SDL_PRESSED:SDL_RELEASED;
	event->x = CursorPos.x;
	event->y = CursorPos.y;
	SDL_PushEvent((SDL_Event *) event);
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
	//setting the subtitle region to the bottom 1/4th of the screen
	subtitleregion.w = w;
	subtitleregion.h = h/4;
	subtitleregion.x = 0;
	subtitleregion.y = h-h/4;
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

void SDLVideoDriver::SetMovieFont(Font *stfont, Palette *pal)
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
		// FIXME: ugly hack!
		SDL_Surface* temp = backBuf;
		backBuf = disp;
		subtitlefont->Print(subtitleregion, (unsigned char *) subtitletext, subtitlepal, IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_BOTTOM, true);
		backBuf = temp;
	}
}
// sets brightness and contrast
// FIXME:SetGammaRamp doesn't seem to work
void SDLVideoDriver::SetGamma(int brightness, int /*contrast*/)
{
	SDL_SetGamma(0.8+brightness/50.0,0.8+brightness/50.0,0.8+brightness/50.0);
}

void SDLVideoDriver::SetMouseScrollSpeed(int speed)
{
	mousescrollspd=(speed+1)*2;
}

int SDLVideoDriver::whereIsTheCursor()
{
	return CursorPosition;
}

bool SDLVideoDriver::isScrolling()
{
	return scrolling;
}

void SDLVideoDriver::drawScrollCursorSprite(int Position)
{
	if (Position == 0) {
		if (moveX == -mousescrollspd) {//bottom left
			SetDragCursor(core->GetScrollCursorSprite(5,numScrollCursor));
		} else if (moveX == mousescrollspd ) {//upper right
			SetDragCursor(core->GetScrollCursorSprite(1,numScrollCursor));
		}
	} else if (Position == mousescrollspd) {
		if (moveX == mousescrollspd) {//right
			SetDragCursor(core->GetScrollCursorSprite(0,numScrollCursor));
		} else { //bottom
			SetDragCursor(core->GetScrollCursorSprite(6,numScrollCursor));
		}
	} else if (Position == -mousescrollspd ) {
		if (moveX == -mousescrollspd) {//left
			SetDragCursor(core->GetScrollCursorSprite(4,numScrollCursor));
		} else { //upper
			SetDragCursor(core->GetScrollCursorSprite(2,numScrollCursor));
		}
	} else if (Position == -2 * mousescrollspd ) { //upper left
		SetDragCursor(core->GetScrollCursorSprite(3,numScrollCursor));
	} else if (Position == 2 * mousescrollspd ) { //bottom right
		SetDragCursor(core->GetScrollCursorSprite(7,numScrollCursor));
	}
	numScrollCursor = (numScrollCursor+1) % 15;
}

