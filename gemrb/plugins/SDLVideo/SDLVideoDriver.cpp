#include "../../includes/win32def.h"
#include "SDLVideoDriver.h"
#include "../Core/Interface.h"
#include <math.h>

SDLVideoDriver::SDLVideoDriver(void)
{
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	SDL_Quit();
}

int SDLVideoDriver::Init(void)
{
  printf("[SDLVideoDriver]: Init...");
  if(SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
    printf("[ERROR]\n");
    return GEM_ERROR;
  }
  printf("[OK]\n");
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(500, 50);
  return GEM_OK;
}

int SDLVideoDriver::CreateDisplay(int width, int height, int bpp, bool fullscreen)
{
	DWORD flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
	if(fullscreen)
		flags |= SDL_FULLSCREEN;
	disp = SDL_SetVideoMode(width, height, bpp, flags);
	if(disp == NULL)
		return GEM_ERROR;
	Viewport.x = Viewport.y = 0;
	Viewport.w = width;
	Viewport.h = height;
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
				unsigned char key = event.key.keysym.unicode & 0xff;
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
				unsigned char key = event.key.keysym.unicode & 0xff;
				if((key == 0) || (key == 13)){
					switch(event.key.keysym.sym) {
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
			}
		}
		SDL_Flip(disp);
		return ret;
	}
	SDL_Flip(disp);
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
			unsigned char key = event.key.keysym.unicode & 0xff;
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
			unsigned char key = event.key.keysym.unicode & 0xff;
			if(key == 0) {
				switch(event.key.keysym.sym) {
					case SDLK_LEFT:
						key = GEM_LEFT;
					break;

					case SDLK_RIGHT:
						key = GEM_RIGHT;
					break;

					case SDLK_DELETE:
						key = GEM_DELETE;
					break;
				}
				if(Evnt)
					Evnt->OnSpecialKeyPress(key);
			}
			else if(Evnt && (key != 0))
				Evnt->KeyPress(key, event.key.keysym.mod);
			if(event.key.keysym.sym == SDLK_RIGHT) {
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
			}
			}
		break;

		case SDL_MOUSEMOTION:
			if(Evnt)
				Evnt->MouseMove(event.motion.x, event.motion.y);
		break;

		case SDL_MOUSEBUTTONDOWN:
			if(Evnt)
				Evnt->MouseDown(event.button.x, event.button.y, event.button.state, 0);
		break;

		case SDL_MOUSEBUTTONUP:
			if(Evnt)
				Evnt->MouseUp(event.button.x, event.button.y, event.button.state, 0);
		break;
		}
	}
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
	SDL_SetPalette((SDL_Surface*)p, SDL_LOGPAL, (SDL_Color*)palette, 0, 256);
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

void SDLVideoDriver::FreeSprite(Sprite2D * spr)
{
	if(spr->vptr)
		SDL_FreeSurface((SDL_Surface*)spr->vptr);
	if(spr->pixels)
		free(spr->pixels);
	delete(spr);
}

void SDLVideoDriver::BlitSpriteRegion(Sprite2D * spr, Region &size, int x, int y, bool anchor, Region * clip)
{
	//TODO: Add the destination surface and rect to the Blit Pipeline
	SDL_Rect drect;
	SDL_Rect t = {size.x, size.y, size.w, size.h};
	if(anchor) {
		drect.x = x;
		drect.y = y;
	}
	else {
		drect.x = x-Viewport.x;
		drect.y = y-Viewport.y;
	}
	if(clip) {
		if(drect.x+size.w <= clip->x)
			return;
		else {
			if(drect.x < clip->x) {
				t.x = size.x+clip->x-drect.x;
				t.w = size.w-t.x;
			}
			else {
				if(drect.x+size.w <= clip->x+clip->w) {
					t.x = size.x;
					t.w = size.w;
				}
				else {
					if(drect.x >= clip->x+clip->w) {
						return;
					}
					else {
						t.x = size.x;
						t.w = (clip->x+clip->w)-drect.x;
					}
				}
			}
		}
		if(drect.y+size.h <= clip->y)
			return;
		else {
			if(drect.y < clip->y) {
				t.y = size.y+clip->y-drect.y;
				t.h = size.h-t.y;
			}
			else {
				if(drect.y+size.h <= clip->y+clip->h) {
					t.y = 0;
					t.h = size.h;
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
	}
	SDL_BlitSurface((SDL_Surface*)spr->vptr, &t, disp, &drect);
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
		/*if(drect.x < clip->x) {
			if(clip->x >= (drect.x+spr->Width))
				return;
			t.x = clip->x-drect.x;
			t.w = spr->Width-t.x;
		}
		else {
			if(drect.x >= (clip->x+clip->w))
				return;
			t.x = 0;
			if((drect.x+spr->Width) > (clip->x+clip->w))
				t.w = (clip->x+clip->w)-drect.x;
			else
				t.w = spr->Width;
		}
		if(drect.y < clip->y) {
			if(clip->y >= (drect.y+spr->Height))
				return;
			t.y = clip->y-drect.y;
			if((drect.y+spr->Height) > (clip->y+clip->h))
				t.h = (clip->y+clip->h)-drect.y;
			else
				t.h = spr->Height-t.y;
		}
		else {
			if(drect.y >= (clip->y+clip->h))
				return;
			t.y = 0;
			t.h = spr->Height;
		}*/
		srect = &t;
	}
	SDL_BlitSurface((SDL_Surface*)spr->vptr, srect, disp, &drect);
	//Debug Addition: Draws a point to the x,y position
	/*drect.x = x;
	drect.y = y;
	drect.w = 1;
	drect.h = 1;
	SDL_FillRect(disp, &drect, 0xffffffff);*/
}

void SDLVideoDriver::SetCursor(Sprite2D * spr, int x, int y)
{
	//TODO: Implement Cursor
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

void SDLVideoDriver::MoveViewportTo(int x, int y)
{
	Viewport.x = x - Viewport.w;
	Viewport.y = y - Viewport.h;
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
	SDL_Surface * rectsurf = SDL_CreateRGBSurface(SDL_HWSURFACE, rgn.w, rgn.h, 8, 0,0,0,0);
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

	SDL_FreeSurface(rectsurf);
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
			SDL_BlitSurface((SDL_Surface*)img->vptr, &srect, disp, &drect);
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
	for(int i = 0; i < s->format->palette->ncolors; i++) {
		pal[i].r = s->format->palette->colors[i].r;
		pal[i].g = s->format->palette->colors[i].g;
		pal[i].b = s->format->palette->colors[i].b;
	}
	return pal;
}

