/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef SDL12VIDEODRIVER_H
#define SDL12VIDEODRIVER_H

#include "SDLVideo.h"

namespace GemRB {

class SDL12VideoDriver : public SDLVideoDriver {
private:
	SDL_Surface* disp;

public:
	SDL12VideoDriver(void);
	
	int Init(void);
	int CreateDisplay(int w, int h, int b, bool fs, const char* title);
	Sprite2D* GetScreenshot( Region r );

	bool SetFullscreenMode(bool set);

	bool ToggleGrabInput();
	void ShowSoftKeyboard();
	void HideSoftKeyboard();
	void SetGamma(int brightness, int contrast);

private:
	void SwapBuffers(VideoBuffers&);
	VideoBuffer* NewVideoBuffer(const Size& rgn, BufferFormat fmt);
	bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha);

	int ProcessEvent(const SDL_Event & event);
};

}

#endif
