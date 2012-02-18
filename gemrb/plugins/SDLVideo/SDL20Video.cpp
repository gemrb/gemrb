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

#include "SDL20Video.h"

#include "TileRenderer.inl"

#include "AnimationFactory.h"
#include "Audio.h"
#include "Game.h" // for GetGlobalTint
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Polygon.h"
#include "SpriteCover.h"
#include "GUI/Console.h"
#include "GUI/GameControl.h" // for TargetMode (contextual information for touch inputs)
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#include <cmath>
#include <cassert>
#include <cstdio>

#if TARGET_OS_IPHONE
extern "C" {
	#include "SDL_sysvideo.h"
	#include <SDL/uikit/SDL_uikitkeyboard.h>
}
#endif
#ifdef ANDROID
#include "SDL_screenkeyboard.h"
#endif

//touch gestures
#define MIN_GESTURE_DELTA_PIXELS 10
#define TOUCH_RC_NUM_TICKS 500

SDL20VideoDriver::SDL20VideoDriver(void)
{
	assert( core->NumFingScroll > 1 && core->NumFingKboard > 1 && core->NumFingInfo > 1);
	assert( core->NumFingScroll < 5 && core->NumFingKboard < 5 && core->NumFingInfo < 5);
	assert( core->NumFingScroll != core->NumFingKboard );

	renderer = NULL;
	window = NULL;
	videoPlayer = NULL;
}

SDL20VideoDriver::~SDL20VideoDriver(void)
{
	SDL_DestroyTexture(videoPlayer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

int SDL20VideoDriver::CreateDisplay(int w, int h, int b, bool fs, const char* title)
{
	bpp=b;
	fullscreen=fs;
	printMessage( "SDL20Video", "Creating display\n", WHITE );
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
#if TARGET_OS_IPHONE || ANDROID
	// this allows the user to flip the device upsidedown if they wish and have the game rotate.
	// it also for some unknown reason is required for retina displays
	winFlags |= SDL_WINDOW_RESIZABLE;
	// this hint is set in the wrapper for iPad at a higher priority. set it here for iPhone
	// don't know if Android makes use of this.
	SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight LandscapeLeft", SDL_HINT_DEFAULT);
#endif
	if (fullscreen) {
		winFlags |= SDL_WINDOW_FULLSCREEN;
		//This is needed to remove the status bar on Android/iOS.
		//since we are in fullscreen this has no effect outside Android/iOS
		winFlags |= SDL_WINDOW_BORDERLESS;
	}
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, winFlags);
	renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		print("couldnt create renderer:%s\n", SDL_GetError());
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	Viewport.x = Viewport.y = 0;
	width = window->w;
	height = window->h;
	Viewport.w = width;
	Viewport.h = height;
	print("%s\n", SDL_GetError());
	printMessage( "SDLVideo", "Creating Main Surface...", WHITE );
	SDL_Surface* tmp = SDL_CreateRGBSurface( 0, width, height,
											bpp, 0, 0, 0, 0 );

	backBuf = SDL_ConvertSurfaceFormat(tmp, SDL_GetWindowPixelFormat(window), 0);
	disp = backBuf;

	printStatus( "OK", LIGHT_GREEN );
	SDL_FreeSurface( tmp );
	printMessage( "SDLVideo", "CreateDisplay...", WHITE );
	printStatus( "OK", LIGHT_GREEN );
	return GEM_OK;
}

int SDL20VideoDriver::SwapBuffers(void)
{
	int ret = SDLVideoDriver::SwapBuffers();

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, backBuf);
	SDL_SetRenderTarget(renderer, tex);
	ret = PollEvents();

	if (fadeColor.a) {
		SDL_Rect dst = {
			Viewport.x, Viewport.y, Viewport.w, Viewport.h
		};
		SDL_SetRenderDrawColor(renderer, fadeColor.r, fadeColor.g, fadeColor.b, fadeColor.a);
		SDL_RenderFillRect(renderer, &dst);
	}

	SDL_RenderCopy(renderer, tex, NULL, NULL);

	SDL_RenderPresent( renderer );
	SDL_DestroyTexture(tex);
	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()
