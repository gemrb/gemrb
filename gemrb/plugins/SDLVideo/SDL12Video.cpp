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

SDL12VideoDriver::SDL12VideoDriver(void)
{
	overlay = NULL;
}

SDL12VideoDriver::~SDL12VideoDriver(void)
{
	if (overlay) SDL_FreeYUVOverlay(overlay);
}

int SDL12VideoDriver::Init(void)
{
	int ret = SDLVideoDriver::Init();
	if (ret) {
		SDL_EnableUNICODE( 1 );
		SDL_EnableKeyRepeat( 500, 50 );
	}
	return ret;
}

int SDL12VideoDriver::CreateDisplay(int w, int h, int b, bool fs, const char* title)
{
	bpp=b;
	fullscreen=fs;
	printMessage( "SDLVideo", "Creating display\n", WHITE );
	ieDword flags = SDL_SWSURFACE;
	if (fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	printMessage( "SDLVideo", "SDL_SetVideoMode...", WHITE );
	disp = SDL_SetVideoMode( w, h, bpp, flags );
	SDL_WM_SetCaption( title, 0 );
	if (disp == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		print("%s\n", SDL_GetError());
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
	width = disp->w;
	height = disp->h;
	Viewport.w = width;
	Viewport.h = height;
	printMessage( "SDLVideo", "Creating Main Surface...", WHITE );
	SDL_Surface* tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height,
						bpp, 0, 0, 0, 0 );
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

int SDL12VideoDriver::SwapBuffers(void)
{
	int ret = SDLVideoDriver::SwapBuffers();

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

	if (Cursor[CursorIndex] && !(MouseFlags & (MOUSE_DISABLED | MOUSE_HIDDEN))) {
		SDL_Surface* temp = backBuf;
		backBuf = disp; // FIXME: UGLY HACK!
		if (MouseFlags&MOUSE_GRAYED) {
			//used for greyscale blitting, fadeColor is unused
			BlitGameSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, BLIT_GREY, fadeColor, NULL, NULL, NULL, true);
		} else {
			BlitSprite(Cursor[CursorIndex], CursorPos.x, CursorPos.y, true);
		}
		backBuf = temp;
	}
	if (!(MouseFlags & MOUSE_NO_TOOLTIPS)) {
		//handle tooltips
		unsigned int delay = core->TooltipDelay;
		// The multiplication by 10 is there since the last, disabling slider position is the eleventh
		if (!core->ConsolePopped && (delay<TOOLTIP_DELAY_FACTOR*10) ) {
			unsigned long time = GetTickCount();
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
	}
	SDL_Flip( disp );
	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL Video Driver")
PLUGIN_DRIVER(SDL12VideoDriver, "sdl")
END_PLUGIN()
