/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/SDLVideo/SDLVideoDriver.cpp,v 1.53 2003/12/23 17:50:42 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "SDLVideoDriver.h"
#include "../Core/Interface.h"
#include <math.h>

SDLVideoDriver::SDLVideoDriver(void)
{
	CursorIndex = 0;
	Cursor[0] = NULL;
	Cursor[1] = NULL;
	moveX = 0;
	moveY = 0;
	DisableMouse = false;
	xCorr = 0;
	yCorr = 0;
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	SDL_FreeSurface(backBuf);
	SDL_FreeSurface(extra);
	SDL_Quit();
}

int SDLVideoDriver::Init(void)
{
  //printf("[SDLVideoDriver]: Init...");
  if(SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
    //printf("[ERROR]\n");
    return GEM_ERROR;
  }
  //printf("[OK]\n");
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(500, 50);
  SDL_ShowCursor(SDL_DISABLE);
  fadePercent = 0;
  return GEM_OK;
}

int SDLVideoDriver::CreateDisplay(int width, int height, int bpp, bool fullscreen)
{
	printMessage("SDLVideo", "Creating display\n", WHITE);
	DWORD flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
	if(fullscreen)
		flags |= SDL_FULLSCREEN;
	printMessage("SDLVideo", "SDL_SetVideoMode...", WHITE);
	disp = SDL_SetVideoMode(width, height, bpp, flags);
	if(disp == NULL) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	Viewport.x = Viewport.y = 0;
	Viewport.w = width;
	Viewport.h = height;
	printMessage("SDLVideo", "Creating Main Surface...", WHITE);
	SDL_Surface * tmp = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, bpp, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	printStatus("OK", LIGHT_GREEN);
	printMessage("SDLVideo", "Creating Back Buffer...", WHITE);
	backBuf = SDL_DisplayFormat(tmp);
	printStatus("OK", LIGHT_GREEN);
	printMessage("SDLVideo", "Creating Extra Buffer...", WHITE);
	extra = SDL_DisplayFormat(tmp);
	printStatus("OK", LIGHT_GREEN);
	SDL_LockSurface(extra);
	memset(extra->pixels, 0, extra->pitch*extra->h);
	SDL_UnlockSurface(extra);
	SDL_FreeSurface(tmp);
	printMessage("SDLVideo", "CreateDisplay...", WHITE);
	printStatus("OK", LIGHT_GREEN);
	return GEM_OK;
}

VideoModes SDLVideoDriver::GetVideoModes(bool fullscreen)
{
	SDL_Rect ** modes;
	DWORD flags = SDL_HWSURFACE;
	if(fullscreen)
		flags |= SDL_FULLSCREEN;
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
	modes = SDL_ListModes(&pf, fullscreen);
	if(modes == (SDL_Rect**)0)
		return vm;
	if(modes == (SDL_Rect**)-1) {
		vm.AddVideoMode(640, 480, 32, fullscreen);
		vm.AddVideoMode(800, 600, 32, fullscreen);
		vm.AddVideoMode(1024, 786, 32, fullscreen);
		vm.AddVideoMode(1280, 1024, 32, fullscreen);
		vm.AddVideoMode(1600, 1200, 32, fullscreen);
	}
	else {
		for(int i = 0; modes[i]; i++) {
			vm.AddVideoMode(modes[i]->w, modes[i]->h, 32, fullscreen);
		}
	}
	return vm;
}

bool SDLVideoDriver::TestVideoMode(VideoMode & vm)
{
	DWORD flags = SDL_HWSURFACE;
	if(vm.GetFullScreen())
		flags |= SDL_FULLSCREEN;
	if(SDL_VideoModeOK(vm.GetWidth(), vm.GetHeight(), vm.GetBPP(), flags) == vm.GetBPP())
		return true;
	return false;
}

int SDLVideoDriver::SwapBuffers(void)
{
	if(core->ConsolePopped) {
		int ret = GEM_OK;
		core->DrawConsole();
		SDL_Event event; /* Event structure */
		while(SDL_PollEvent(&event)) {  /* Loop until there are no events left on the queue */
			switch(event.type){  /* Process the appropiate event type */
			case SDL_QUIT:  /* Handle a KEYDOWN event */         
				ret = GEM_ERROR;
			break;

			case SDL_KEYUP:
				{
				//unsigned char key = Convert(event.key.keysym.sym, event.key.keysym.mod);
				unsigned char key = event.key.keysym.sym;
				if(Evnt && (key != 0))
					Evnt->KeyRelease(key, event.key.keysym.mod);
				}
			break;

			case SDL_KEYDOWN:
				{
				if(event.key.keysym.sym == SDLK_ESCAPE) {
					core->PopupConsole();
					break;
				}
				//unsigned char key = Convert(event.key.keysym.sym, event.key.keysym.mod);
				unsigned char key = event.key.keysym.unicode & 0xff;
				if(key<32 || key==127 ){
					switch(event.key.keysym.sym) {
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
					}
					core->console->OnSpecialKeyPress(key);
				}
				else if((key != 0))
					core->console->OnKeyPress(key, event.key.keysym.mod);
				}
			break;

			case SDL_MOUSEMOTION:
				{
					if(DisableMouse)
						break;
					CursorPos.x = event.motion.x-mouseAdjustX[CursorIndex];
					CursorPos.y = event.motion.y-mouseAdjustY[CursorIndex];
					if(event.motion.x <= 0)
						moveX = -5;
					else {
						if(event.motion.x >= (core->Width-1))
							moveX = 5;
						else
							moveX = 0;
					}
					if(event.motion.y <= 0)
						moveY = -5;
					else {
						if(event.motion.y >= (core->Height-1))
							moveY = 5;
						else
							moveY = 0;
					}
					if(Evnt)
						Evnt->MouseMove(event.motion.x, event.motion.y);
				}
			break;

			case SDL_MOUSEBUTTONDOWN:
				{
					if(DisableMouse)
						break;
					CursorIndex = 1;
					CursorPos.x = event.button.x-mouseAdjustX[CursorIndex];
					CursorPos.y = event.button.y-mouseAdjustY[CursorIndex];
				}
			break;

			case SDL_MOUSEBUTTONUP:
				{
					if(DisableMouse)
						break;
					CursorIndex = 0;
					CursorPos.x = event.button.x-mouseAdjustX[CursorIndex];
					CursorPos.y = event.button.y-mouseAdjustY[CursorIndex];
				}
			break;
			}
		}
		SDL_BlitSurface(backBuf, NULL, disp, NULL);
		if(Cursor[CursorIndex] && !DisableMouse) {
			short x = CursorPos.x;
			short y = CursorPos.y;
			SDL_BlitSurface(Cursor[CursorIndex], NULL, disp, &CursorPos);
			CursorPos.x = x;
			CursorPos.y = y;
		}
		if(fadePercent) {
			//printf("Fade Percent = %d%%\n", fadePercent);
			SDL_SetAlpha(extra, SDL_SRCALPHA, (255*fadePercent)/100);
			SDL_BlitSurface(extra, NULL, disp, NULL);
		}
		SDL_Flip(disp);
		return ret;
	}
	/*SDL_BlitSurface(backBuf, NULL, disp, NULL);
	if(Cursor[CursorIndex])
		SDL_BlitSurface(Cursor[CursorIndex], NULL, disp, &CursorPos);
	if(fadePercent) {
		//printf("Fade Percent = %d%%\n", fadePercent);
		SDL_SetAlpha(extra, SDL_SRCALPHA, (255*fadePercent)/100);
		SDL_BlitSurface(extra, NULL, disp, NULL);
	}
	SDL_Flip(disp);*/
	int ret = GEM_OK;
	//TODO: Implement an efficient Rectangle Merge algorithm for faster redraw
	SDL_Event event; /* Event structure */
	while(SDL_PollEvent(&event)) {  /* Loop until there are no events left on the queue */
		switch(event.type){  /* Process the appropiate event type */
		case SDL_QUIT:  /* Handle a KEYDOWN event */         
			ret = GEM_ERROR;
		break;

		case SDL_KEYUP:
			{
			//unsigned char key = Convert(event.key.keysym.sym, event.key.keysym.mod);
			unsigned char key = event.key.keysym.sym & 0xff;
			if(Evnt && (key != 0))
				Evnt->KeyRelease(key, event.key.keysym.mod);
			}
		break;

		case SDL_KEYDOWN:
			{
			if(event.key.keysym.sym == SDLK_ESCAPE) {
				core->PopupConsole();
				break;
			}
			unsigned char key = event.key.keysym.unicode;
			if(key <32 || key==127) {
				switch(event.key.keysym.sym) {
					case SDLK_LEFT:
						key = GEM_LEFT;
					break;

					case SDLK_RIGHT:
						key = GEM_RIGHT;
					break;

					case SDLK_UP:
						key = GEM_UP;
					break;

					case SDLK_DOWN:
						key = GEM_DOWN;
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

					case SDLK_TAB:
						key = GEM_TAB;
					break;
					case SDLK_LALT:
					case SDLK_RALT:
						key = GEM_ALT;
					break;
				}
				if(Evnt)
					Evnt->OnSpecialKeyPress(key);
			}
			else if(Evnt && (key != 0))
				Evnt->KeyPress(key, event.key.keysym.mod);
			/*if(event.key.keysym.sym == SDLK_RIGHT) {
				Viewport.x += 64;
			}
			else if(event.key.keysym.sym == SDLK_LEFT) {
				Viewport.x -= 64;
			}
			else if(event.key.keysym.sym == SDLK_UP) {
				Viewport.y -= 64;
			}
			else if(event.key.keysym.sym == SDLK_DOWN) {
				Viewport.y += 64;
			}*/
			}
		break;

		case SDL_MOUSEMOTION:
			{
				if(DisableMouse)
					break;
				CursorPos.x = event.motion.x-mouseAdjustX[CursorIndex];
				CursorPos.y = event.motion.y-mouseAdjustY[CursorIndex];
				if(event.motion.x <= 0)
					moveX = -5;
				else {
					if(event.motion.x >= (core->Width-1))
						moveX = 5;
					else
						moveX = 0;
				}
				if(event.motion.y <= 0)
					moveY = -5;
				else {
					if(event.motion.y >= (core->Height-1))
						moveY = 5;
					else
						moveY = 0;
				}
				if(Evnt)
					Evnt->MouseMove(event.motion.x, event.motion.y);
			}
		break;

		case SDL_MOUSEBUTTONDOWN:
			{
				if(DisableMouse)
					break;
				CursorIndex = 1;
				CursorPos.x = event.button.x-mouseAdjustX[CursorIndex];
				CursorPos.y = event.button.y-mouseAdjustY[CursorIndex];
				if(Evnt)
					Evnt->MouseDown(event.button.x, event.button.y, event.button.state, 0);
			}
		break;

		case SDL_MOUSEBUTTONUP:
			{
				if(DisableMouse)
					break;
				CursorIndex = 0;
				CursorPos.x = event.button.x-mouseAdjustX[CursorIndex];
				CursorPos.y = event.button.y-mouseAdjustY[CursorIndex];
				if(Evnt)
					Evnt->MouseUp(event.button.x, event.button.y, event.button.state, 0);
			}
		break;

		case SDL_ACTIVEEVENT:
			{
				if(event.active.state == SDL_APPMOUSEFOCUS)
					if(Evnt && !event.active.gain)
						Evnt->OnSpecialKeyPress(GEM_MOUSEOUT);
			}
		break;
		}
	}
	SDL_BlitSurface(backBuf, NULL, disp, NULL);
	if(Cursor[CursorIndex] && !DisableMouse) {
		short x = CursorPos.x;
		short y = CursorPos.y;
		SDL_BlitSurface(Cursor[CursorIndex], NULL, disp, &CursorPos);
		CursorPos.x = x;
		CursorPos.y = y;
	}
	if(fadePercent) {
		//printf("Fade Percent = %d%%\n", fadePercent);
		SDL_SetAlpha(extra, SDL_SRCALPHA, (255*fadePercent)/100);
		SDL_BlitSurface(extra, NULL, disp, NULL);
	}
	SDL_Flip(disp);
	return ret;
}

Sprite2D *SDLVideoDriver::CreateSprite(int w, int h, int bpp, DWORD rMask, DWORD gMask, DWORD bMask, DWORD aMask, void* pixels, bool cK, int index)
{
	Sprite2D *spr = new Sprite2D();
	void * p = SDL_CreateRGBSurfaceFrom(pixels, w, h, bpp, w*(bpp/8), rMask, gMask, bMask, aMask);
	if(p != NULL) {
		spr->vptr = p;
		spr->pixels = pixels;
	}
	if(cK)
		SDL_SetColorKey((SDL_Surface*)p, SDL_SRCCOLORKEY | SDL_RLEACCEL, index);
	spr->Width = w;
	spr->Height = h;
	return spr;
}

Sprite2D *SDLVideoDriver::CreateSprite8(int w, int h, int bpp, void* pixels, void* palette, bool cK, int index)
{
	Sprite2D *spr = new Sprite2D();
	void * p = SDL_CreateRGBSurfaceFrom(pixels, w, h, 8, w, 0,0,0,0);
	int colorcount;
	if(bpp == 8)
		colorcount = 256;
	else
		colorcount = 16;
	SDL_SetPalette((SDL_Surface*)p, SDL_LOGPAL, (SDL_Color*)palette, 0, colorcount);
	if(p != NULL) {
		spr->vptr = p;
		spr->pixels = pixels;
	}
	if(cK)
		SDL_SetColorKey((SDL_Surface*)p, SDL_SRCCOLORKEY | SDL_RLEACCEL, index);
	spr->Width = w;
	spr->Height = h;
	if(bpp == 8) {
		spr->palette = (Color*)malloc(256*sizeof(Color));
		memcpy(spr->palette, palette, 256*sizeof(Color));
	}
	return spr;
}

void SDLVideoDriver::FreeSprite(Sprite2D * spr)
{
	if(spr->vptr)
		SDL_FreeSurface((SDL_Surface*)spr->vptr);
	if(spr->pixels)
		free(spr->pixels);
	if(spr->palette)
		free(spr->palette);
	delete(spr);
	spr = NULL;
}

void SDLVideoDriver::BlitSpriteRegion(Sprite2D * spr, Region &size, int x, int y, bool anchor, Region * clip)
{
	//TODO: Add the destination surface and rect to the Blit Pipeline
	SDL_Rect drect;
	SDL_Rect t = {size.x, size.y, size.w, size.h};
	Region c(clip->x, clip->y, clip->w, clip->h);
	if(anchor) {
		drect.x = x;
		drect.y = y;
	}
	else {
		drect.x = x-Viewport.x;
		drect.y = y-Viewport.y;
		if(clip) {
			c.x -= Viewport.x;
			c.y -= Viewport.y;
		}
	}
	if(clip) {
		if(drect.x+size.w <= c.x)
			return;
		else {
			if(drect.x < c.x) {
				t.x = size.x+c.x-drect.x;
				t.w = size.w-(c.x-drect.x);
				drect.x = clip->x;
			}
			else {
				if(drect.x+size.w <= c.x+c.w) {
					t.x = size.x;
					t.w = size.w;
				}
				else {
					if(drect.x >= c.x+c.w) {
						return;
					}
					else {
						t.x = size.x;
						t.w = (c.x+c.w)-drect.x;
					}
				}
			}
		}
		if(drect.y+size.h <= c.y)
			return;
		else {
			if(drect.y < c.y) {
				t.y = size.y+c.y-drect.y;
				t.h = size.h-(c.y-drect.y);
				drect.y = clip->y;
			}
			else {
				if(drect.y+size.h <= c.y+c.h) {
					t.y = 0;
					t.h = size.h;
				}
				else {
					if(drect.y >= c.y+c.h) {
						return;
					}
					else {
						t.y = 0;
						t.h = (c.y+c.h)-drect.y;
					}
				}
			}
		}
	}
	SDL_BlitSurface((SDL_Surface*)spr->vptr, &t, backBuf, &drect);
}

void SDLVideoDriver::BlitSprite(Sprite2D * spr, int x, int y, bool anchor, Region * clip)
{
	//TODO: Add the destination surface and rect to the Blit Pipeline
	SDL_Rect drect;
	SDL_Rect t;
	SDL_Rect *srect = NULL;
	if(anchor) {
		drect.x = x-spr->XPos;
		drect.y = y-spr->YPos;
	}
	else {
		drect.x = x-spr->XPos-Viewport.x;
		drect.y = y-spr->YPos-Viewport.y;
	}
	if(clip) {
		if(drect.x+spr->Width <= clip->x)
			return;
		else {
			if(drect.x < clip->x) {
				t.x = clip->x-drect.x;
				t.w = spr->Width-t.x;
				drect.x = clip->x;
			}
			else {
				if(drect.x+spr->Width <= clip->x+clip->w) {
					t.x = 0;
					t.w = spr->Width;
				}
				else {
					if(drect.x >= clip->x+clip->w) {
						return;
					}
					else {
						t.x = 0;
						t.w = (clip->x+clip->w)-drect.x;
					}
				}
			}
		}
		if(drect.y+spr->Height <= clip->y)
			return;
		else {
			if(drect.y < clip->y) {
				t.y = clip->y-drect.y;
				t.h = spr->Height-t.y;
				drect.y = clip->y;
			}
			else {
				if(drect.y+spr->Height <= clip->y+clip->h) {
					t.y = 0;
					t.h = spr->Height;
				}
				else {
					if(drect.y >= clip->y+clip->h) {
						return;
					}
					else {
						t.y = 0;
						t.h = (clip->y+clip->h)-drect.y;
					}
				}
			}
		}
		srect = &t;
	}
	SDL_BlitSurface((SDL_Surface*)spr->vptr, srect, backBuf, &drect);
}
void SDLVideoDriver::BlitSpriteTinted(Sprite2D * spr, int x, int y, Color tint, Region * clip)
{
	SDL_Surface * tmp = (SDL_Surface*)spr->vptr;
	SDL_Color * pal = tmp->format->palette->colors;
	SDL_Color oldPal[256];//, newPal[256];
	memcpy(oldPal, pal, 256*sizeof(SDL_Color));
	//memcpy(newPal, pal, 2*sizeof(SDL_Color));
	for(int i = 2; i < 256; i++) {
		pal[i].r = (tint.r*oldPal[i].r) >> 8;
		pal[i].g = (tint.g*oldPal[i].g) >> 8;
		pal[i].b = (tint.b*oldPal[i].b) >> 8;
	}
	//SDL_SetPalette(tmp, SDL_LOGPAL, newPal, 0, 256);
	BlitSprite(spr, x, y, false, clip);
	SDL_SetPalette(tmp, SDL_LOGPAL, oldPal, 0, 256);
}

void SDLVideoDriver::BlitSpriteMode(Sprite2D * spr, int x, int y, int blendMode, bool anchor, Region * clip)
{
	if(blendMode == IE_NORMAL) {
		BlitSprite(spr, x, y, anchor, clip);
		return;
	}
	SDL_Rect drect;
	SDL_Rect t;
	SDL_Rect *srect = NULL;
	if(anchor) {
		drect.x = x-spr->XPos;
		drect.y = y-spr->YPos;
	}
	else {
		drect.x = x-spr->XPos-Viewport.x;
		drect.y = y-spr->YPos-Viewport.y;
	}
	if(clip) {
		if(drect.x+spr->Width <= clip->x)
			return;
		else {
			if(drect.x < clip->x) {
				t.x = clip->x-drect.x;
				t.w = spr->Width-t.x;
				drect.x = clip->x;
			}
			else {
				if(drect.x+spr->Width <= clip->x+clip->w) {
					t.x = 0;
					t.w = spr->Width;
				}
				else {
					if(drect.x >= clip->x+clip->w) {
						return;
					}
					else {
						t.x = 0;
						t.w = (clip->x+clip->w)-drect.x;
					}
				}
			}
		}
		if(drect.y+spr->Height <= clip->y)
			return;
		else {
			if(drect.y < clip->y) {
				t.y = clip->y-drect.y;
				t.h = spr->Height-t.y;
				drect.y = clip->y;
			}
			else {
				if(drect.y+spr->Height <= clip->y+clip->h) {
					t.y = 0;
					t.h = spr->Height;
				}
				else {
					if(drect.y >= clip->y+clip->h) {
						return;
					}
					else {
						t.y = 0;
						t.h = (clip->y+clip->h)-drect.y;
					}
				}
			}
		}
		srect = &t;
	}
	if(!srect) {
		srect = &t;
		srect->x = 0;
		srect->y = 0;
		srect->w = spr->Width;
		srect->h = spr->Height;
	}
	SDL_Surface * surf = (SDL_Surface*)spr->vptr;
	int destx = drect.x, desty = drect.y;
	Region Screen = core->GetVideoDriver()->GetViewport();
	if((destx > (xCorr+Screen.w)) || ((destx+srect->w) < xCorr))
		return;
	if((desty > (yCorr+Screen.h)) || ((desty+srect->h) < yCorr))
		return;

	SDL_LockSurface(surf);
	SDL_LockSurface(backBuf);

	unsigned char * src, *dst;

	for(int y = srect->y; y < srect->h; y++) {
		if((desty < yCorr) || (desty >= (yCorr+Screen.h))) {
			desty++;
			continue;
		}
		destx = drect.x; 
		src = ((unsigned char*)spr->pixels)+(y*surf->pitch);
		dst = ((unsigned char *)backBuf->pixels)+(desty*backBuf->pitch)+(destx*backBuf->format->BytesPerPixel);
		for(int x = srect->x; x < srect->w; x++) {
			if((destx < xCorr) || (destx >= (xCorr+Screen.w))) {
				src++;
				destx++;
				dst+=4;
				continue;
			}
			if(!(*src)) {
				src++;
				destx++;
				dst+=4;
				continue;
			}
			SDL_Color *c1 = &surf->format->palette->colors[*src++];
			if(c1->b > *dst)
				*dst = c1->b;
			dst++;
			if(c1->g > *dst)
				*dst = c1->g;
			dst++;
			if(c1->r > *dst)
				*dst = c1->r;
			dst+=2;
			destx++;
		}
		desty++;
	}

	SDL_UnlockSurface(backBuf);
	SDL_UnlockSurface(surf);
}

void SDLVideoDriver::SetCursor(Sprite2D * up, Sprite2D * down)
{
	if(up) {
		Cursor[0]=(SDL_Surface*)up->vptr;
		mouseAdjustX[0] = up->XPos;
		mouseAdjustY[0] = up->YPos;
	}
	else
		Cursor[0]=NULL;
	if(down) {
		Cursor[1]=(SDL_Surface*)down->vptr;
		mouseAdjustX[1] = down->XPos;
		mouseAdjustY[1] = down->YPos;
	}
	else
		Cursor[1]=NULL;
	return;
}

Region SDLVideoDriver::GetViewport()
{
	return Viewport;
}

void SDLVideoDriver::SetViewport(int x, int y)
{
	Viewport.x = x;
	Viewport.y = y;
}

void SDLVideoDriver::SetViewport(int x, int y, int w, int h)
{
	//Viewport.x = x;
	//Viewport.y = y;
	xCorr = x;
	yCorr = y;
	Viewport.w = w;
	Viewport.h = h;
	//core->Width = w;
	//core->Height = h;
}

void SDLVideoDriver::MoveViewportTo(int x, int y)
{
	Viewport.x = x - (Viewport.w/2);
	Viewport.y = y - (Viewport.h/2);
}
/** No descriptions */
void SDLVideoDriver::SetPalette(Sprite2D * spr, Color * pal){
	SDL_Surface * sur = (SDL_Surface*)spr->vptr;
	SDL_SetPalette(sur, SDL_LOGPAL, (SDL_Color*)pal, 0, 256);
}

void SDLVideoDriver::ConvertToVideoFormat(Sprite2D * sprite)
{
	SDL_Surface * ss = (SDL_Surface*)sprite->vptr;
	if(ss->format->Amask != 0) //Surface already converted
		return;
	SDL_Surface * ns = SDL_DisplayFormatAlpha(ss);
	if(ns == NULL)
		return;
	SDL_FreeSurface(ss);
	if(sprite->pixels)
		free(sprite->pixels);
	sprite->pixels = NULL;
	sprite->vptr = ns;
}

#define MINCOL 2
#define MUL    2

void SDLVideoDriver::CalculateAlpha(Sprite2D * sprite)
{
	SDL_Surface * surf = (SDL_Surface*)sprite->vptr;
	SDL_LockSurface(surf);
	unsigned char * p = (unsigned char*)surf->pixels;
	unsigned char * end = p + (surf->pitch*surf->h);
	unsigned char r,g,b,m;
	while( p < end ) {
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
		m = (r+g+b)/3;
		if(m > MINCOL)
			if((r == 0) && (g == 0xff) && (b == 0))
				*p++ = 0xff;
			else
				*p++ = (m*MUL > 0xff) ? 0xff : m*MUL;
		else
			*p++ = 0;
	}
	SDL_UnlockSurface(surf);
}

/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
void SDLVideoDriver::DrawRect(Region &rgn, Color &color){
	/*SDL_Surface * rectsurf = SDL_CreateRGBSurface(SDL_HWSURFACE, rgn.w, rgn.h, 8, 0,0,0,0);
	SDL_Color pal[2];
	pal[0].r = color.r;
	pal[0].g = color.g;
	pal[0].b = color.b;
	pal[0].unused = color.a;
	
	pal[1].r = 0;
	pal[1].g = 0;
	pal[1].b = 0;
	pal[1].unused = 0;

	SDL_SetPalette(rectsurf, SDL_LOGPAL, pal, 0, 2);
	
	SDL_Rect drect = {0,0,rgn.w, rgn.h};
	SDL_FillRect(rectsurf, &drect, 1);

	drect.x = rgn.x;
	drect.y = rgn.y;
	
	if(color.a != 0)
		SDL_SetAlpha(rectsurf, SDL_SRCALPHA | SDL_RLEACCEL, 128);
	
	SDL_BlitSurface(rectsurf, NULL, disp, &drect);
	
	SDL_FreeSurface(rectsurf);*/
	SDL_Rect drect = {rgn.x,rgn.y,rgn.w, rgn.h};
	SDL_FillRect(backBuf, &drect, (color.a << 24)+(color.r<<16)+(color.g<<8)+color.b);
}
void SDLVideoDriver::SetPixel(short x, short y, Color &color)
{
	x+=xCorr;
	y+=yCorr;
	if((x >= (xCorr+Viewport.w)) || (y >= (yCorr+Viewport.h)))
		return;
	if((x < xCorr) || (y < yCorr))
		return;
	unsigned char *pixels = ((unsigned char *)backBuf->pixels)+((y*disp->w)*disp->format->BytesPerPixel)+(x*disp->format->BytesPerPixel);
	*pixels++ = color.b;
	*pixels++ = color.g;
	*pixels++ = color.r;
	*pixels++ = color.a;
}
void SDLVideoDriver::GetPixel(short x, short y, Color *color)
{
	unsigned char *pixels = ((unsigned char *)backBuf->pixels)+((y*disp->w)*disp->format->BytesPerPixel)+(x*disp->format->BytesPerPixel);
	 color->b = *pixels++;
	 color->g = *pixels++;
	 color->r = *pixels++;
	 color->a = *pixels++;
}
void SDLVideoDriver::DrawLine(short x1, short y1, short x2, short y2, Color &color)
{
	x1 -= Viewport.x;
	x2 -= Viewport.x;
	y1 -= Viewport.y;
	y2 -= Viewport.y;
	bool yLonger=false;
	int shortLen=y2-y1;
	int longLen=x2-x1;
	if (abs(shortLen)>abs(longLen)) {
		int swap=shortLen;
		shortLen=longLen;
		longLen=swap;				
		yLonger=true;
	}
	int decInc;
	if (longLen==0) decInc=0;
	else decInc = (shortLen << 16) / longLen;

	if (yLonger) {
		if (longLen>0) {
			longLen+=y1;
			for (int j=0x8000+(x1<<16);y1<=longLen;++y1) {
				SetPixel(j >> 16,y1,color);	
				j+=decInc;
			}
			return;
		}
		longLen+=y1;
		for (int j=0x8000+(x1<<16);y1>=longLen;--y1) {
			SetPixel(j >> 16,y1,color);	
			j-=decInc;
		}
		return;	
	}

	if (longLen>0) {
		longLen+=x1;
		for (int j=0x8000+(y1<<16);x1<=longLen;++x1) {
			SetPixel(x1,j >> 16, color);
			j+=decInc;
		}
		return;
	}
	longLen+=x1;
	for (int j=0x8000+(y1<<16);x1>=longLen;--x1) {
		SetPixel(x1,j >> 16,color);
		j-=decInc;
	}
}
/** This functions Draws a Circle */
void SDLVideoDriver::DrawCircle(short cx, short cy, unsigned short r, Color &color)
{
	//Uses the Breshenham's Circle Algorithm
	long x, y, xc, yc, re;

	x = r;
	y = 0;
	xc = 1-(2*r);
	yc = 1;
	re = 0;

	if(SDL_MUSTLOCK(disp))
		SDL_LockSurface(disp);
	while(x >= y) {
		SetPixel(cx + (short)x, cy + (short)y, color);
		SetPixel(cx - (short)x, cy + (short)y, color);
		SetPixel(cx - (short)x, cy - (short)y, color);
		SetPixel(cx + (short)x, cy - (short)y, color);
		SetPixel(cx + (short)y, cy + (short)x, color);
		SetPixel(cx - (short)y, cy + (short)x, color);
		SetPixel(cx - (short)y, cy - (short)x, color);
		SetPixel(cx + (short)y, cy - (short)x, color);

		y++;
		re+=yc;
		yc+=2;

		if(((2*re) + xc) > 0) {
			x--;
			re+=xc;
			xc+=2;
		}
	}
	if(SDL_MUSTLOCK(disp))
		SDL_UnlockSurface(disp);
}
/** This functions Draws an Ellipse */
void SDLVideoDriver::DrawEllipse(short cx, short cy, unsigned short xr, unsigned short yr, Color &color)
{
	//Uses the Breshenham's Ellipse Algorithm
	long x, y, xc, yc, ee, tas, tbs, sx, sy;

	if(SDL_MUSTLOCK(disp))
		SDL_LockSurface(disp);
	tas = 2*xr*xr;
	tbs = 2*yr*yr;
	x = xr;
	y = 0;
	xc = yr*yr*(1-(2*xr));
	yc = xr*xr;
	ee = 0;
	sx = tbs*xr;
	sy = 0;

	while( sx >= sy ) {
		SetPixel(cx+(short)x, cy+(short)y, color);
		SetPixel(cx-(short)x, cy+(short)y, color);
		SetPixel(cx-(short)x, cy-(short)y, color);
		SetPixel(cx+(short)x, cy-(short)y, color);
		y++;
		sy+=tas;
		ee+=yc;
		yc+=tas;
		if((2*ee+xc) > 0) {
			x--;
			sx-=tbs;
			ee+=xc;
			xc+=tbs;
		}
	}

	x = 0;
	y = yr;
	xc = yr*yr;
	yc = xr*xr*(1-(2*yr));
	ee = 0;
	sx = 0;
	sy = tas*yr;

	while( sx <= sy) {
		SetPixel(cx+(short)x, cy+(short)y, color);
		SetPixel(cx-(short)x, cy+(short)y, color);
		SetPixel(cx-(short)x, cy-(short)y, color);
		SetPixel(cx+(short)x, cy-(short)y, color);
		x++;
		sx+=tbs;
		ee+=xc;
		xc+=tbs;
		if((2*ee+yc) > 0) {
			y--;
			sy-=tas;
			ee+=yc;
			yc+=tas;
		}
	}
	if(SDL_MUSTLOCK(disp))
		SDL_UnlockSurface(disp);
}

Sprite2D * SDLVideoDriver::PrecalculatePolygon(Point * points, int count, Color &color)
{
	short minX = 20000, maxX = 0, minY = 20000, maxY = 0;
	for(int i = 0; i < count; i++) {
		if(points[i].x < minX)
			minX = points[i].x;
		if(points[i].y < minY)
			minY = points[i].y;
		if(points[i].x > maxX)
			maxX = points[i].x;
		if(points[i].y > maxY)
			maxY = points[i].y;

	}
	short width = maxX-minX;
	short height = maxY-minY;

	void * pixels = malloc(width*height);
	memset(pixels, 0, width*height);

	unsigned char * ptr = (unsigned char*)pixels;

	Gem_Polygon * poly = new Gem_Polygon(points, count);

	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			if(poly->PointIn(x+minX, y+minY))
				*ptr = 1;
			ptr++;
		}
	}

	delete(poly);

	Color palette[2];
	memset(palette, 0, 2*sizeof(Color));
	palette[0].g = 0xff;
	palette[0].a = 0x00;
	palette[1].r = color.r;
	palette[1].g = color.g;
	palette[1].b = color.b;
	palette[1].a = 128;


	Sprite2D *spr = new Sprite2D();
	void * p = SDL_CreateRGBSurfaceFrom(pixels, width, height, 8, width, 0,0,0,0);
	SDL_SetPalette((SDL_Surface*)p, SDL_LOGPAL, (SDL_Color*)palette, 0, 2);
	if(p != NULL) {
		spr->vptr = p;
		spr->pixels = pixels;
	}
	SDL_SetColorKey((SDL_Surface*)p, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
	SDL_SetAlpha((SDL_Surface*)p, SDL_SRCALPHA | SDL_RLEACCEL, 128);
	spr->Width = width;
	spr->Height = height;
	return spr;
}

void SDLVideoDriver::DrawPolyline(Gem_Polygon * poly, Color &color, bool fill)
{
	if(!poly->count)
		return;
	if(fill) {
		if(!poly->fill) {
			poly->fill = PrecalculatePolygon(poly->points, poly->count, color);
		}
		Region Screen = Viewport;
		Screen.x = xCorr;
		Screen.y = yCorr;
		BlitSprite(poly->fill, poly->BBox.x+xCorr, poly->BBox.y+yCorr, false, &Screen);
	}
	short lastX = poly->points[0].x, lastY = poly->points[0].y;
	int i;

	for(i = 1; i < poly->count; i++) {
		DrawLine(lastX, lastY, poly->points[i].x, poly->points[i].y, color);
		lastX = poly->points[i].x;
		lastY = poly->points[i].y;
	}
	DrawLine(lastX, lastY, poly->points[0].x, poly->points[0].y, color);
	return;
}
/** Creates a Palette from Color */
Color * SDLVideoDriver::CreatePalette(Color color, Color back)
{
	Color * pal = (Color*)malloc(256*sizeof(Color));
	pal[0].r = 0;
	pal[0].g = 0xff;
	pal[0].b = 0;
	pal[0].a = 0;
	for(int i = 1; i < 256; i++) {
		pal[i].r = back.r+(unsigned char)(((color.r-back.r)*(i))/255.0);
		pal[i].g = back.g+(unsigned char)(((color.g-back.g)*(i))/255.0);
		pal[i].b = back.b+(unsigned char)(((color.b-back.b)*(i))/255.0);
		pal[i].a = 0;
	}
	return pal;
}
/** Blits a Sprite filling the Region */
void SDLVideoDriver::BlitTiled(Region rgn, Sprite2D * img, bool anchor)
{
	if(!anchor) {
		rgn.x -= Viewport.x;
		rgn.y -= Viewport.y;
	}
	int xrep = (rgn.w + img->Width - 1)/img->Width;
	int yrep = (rgn.h + img->Height - 1)/img->Height;
	for(int y = 0; y < yrep; y++) {
		for(int x = 0; x < xrep; x++) {
			SDL_Rect srect = {0,0, ((img->Width % rgn.w) == 0) ? img->Width : img->Width % rgn.w, ((img->Height % rgn.h) == 0) ? img->Height : img->Height % rgn.h };
			SDL_Rect drect = {rgn.x+(x*img->Width), rgn.y+(y*img->Height), 1, 1};
			SDL_BlitSurface((SDL_Surface*)img->vptr, &srect, backBuf, &drect);
		}
	}
}
/** Send a Quit Signal to the Event Queue */
bool SDLVideoDriver::Quit()
{
	SDL_Event evnt;
	evnt.type = SDL_QUIT;
	if(SDL_PushEvent(&evnt)==-1)
		return false;
	return true;
}
/** Get the Palette of a Sprite */
Color * SDLVideoDriver::GetPalette(Sprite2D * spr)
{
	SDL_Surface * s = (SDL_Surface*)spr->vptr;
	if(s->format->BitsPerPixel != 8)
		return NULL;
	Color * pal = (Color*)malloc(256*sizeof(Color));
	if(spr->palette) {
		for(int i = 0; i < 256; i++) {
			pal[i].r = spr->palette[i].r;
			pal[i].g = spr->palette[i].g;
			pal[i].b = spr->palette[i].b;
		}
	}
	else {
		for(int i = 0; i < s->format->palette->ncolors; i++) {
			pal[i].r = s->format->palette->colors[i].r;
			pal[i].g = s->format->palette->colors[i].g;
			pal[i].b = s->format->palette->colors[i].b;
		}
	}
	return pal;
}

void SDLVideoDriver::MirrorAnimation(Animation * anim)
{
	Sprite2D * frame = NULL;
	int i = 0;
	do {
		frame = anim->GetFrame(i++);
		if(!frame)
			break;
		unsigned char *buffer = (unsigned char*)malloc(frame->Width*frame->Height);
		unsigned char *dst = buffer;
		for(int y = 0; y < frame->Height; y++) {
			unsigned char *src = ((unsigned char*)frame->pixels)+(y*frame->Width)+frame->Width-1;
			for(int x = 0; x < frame->Width; x++) {
				*dst = *src;
				dst++;
				src--;
			}
		}
		memcpy(frame->pixels, buffer, frame->Width*frame->Height);
		free(buffer);
		frame->XPos = frame->Width-frame->XPos;
	} while(true);
}

void SDLVideoDriver::SetFadePercent(int percent)
{
	fadePercent = percent;
}

void SDLVideoDriver::SetClipRect(Region * clip)
{
	if(clip) {
		SDL_Rect tmp;
		SDL_Rect * rect = &tmp;
		rect->x = clip->x;
		rect->y = clip->y;
		rect->w = clip->w;
		rect->h = clip->h;
		SDL_SetClipRect(backBuf, rect);
	}
	else 
		SDL_SetClipRect(backBuf, NULL);
}
