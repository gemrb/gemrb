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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DirectXVideo/DirectXVideoDriver.h,v 1.7 2004/02/24 22:20:38 balrog994 Exp $
 *
 */

#ifndef SDLVIDEODRIVER_H
#define SDLVIDEODRIVER_H

#include "../Core/Video.h"
#include "../Core/Interface.h"
#include "d3d9.h"
#include "d3dx9.h"

typedef struct CUSTOMVERTEX {
	float x, y, z;
	D3DCOLOR color;
	float tu, tv;
} CUSTOMVERTEX;

const DWORD D3DFVF_CUSTOMVERTEX = ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 );

class DirectXVideoDriver : public Video {
private:	
	bool sceneBegin;
	char* winClassName;
	int quit;
	std::vector< Region> upd;	//Regions of the Screen to Update in the next SwapBuffer operation.
	Region Viewport;
public:
	DirectXVideoDriver(void);
	~DirectXVideoDriver(void);
	int Init(void);
	int CreateDisplay(int width, int height, int bpp, bool fullscreen);
	VideoModes GetVideoModes(bool fullscreen = false);
	bool TestVideoMode(VideoMode& vm);
	int SwapBuffers(void);
	Sprite2D* CreateSprite(int w, int h, int bpp, DWORD rMask, DWORD gMask,
		DWORD bMask, DWORD aMask, void* pixels, bool cK = false, int index = 0);
	Sprite2D* CreateSprite8(int w, int h, int bpp, void* pixels,
		void* palette, bool cK = false, int index = 0);
	void FreeSprite(Sprite2D* spr);
	void BlitSprite(Sprite2D* spr, int x, int y, bool anchor = false,
		Region* clip = NULL);
	void SetCursor(Sprite2D* spr, int x, int y);
	Region GetViewport(void);
	void SetViewport(int x, int y);
	void MoveViewportTo(int x, int y);
	void ConvertToVideoFormat(Sprite2D* sprite);
	void CalculateAlpha(Sprite2D* sprite);
	/** No descriptions */
	void SetPalette(Sprite2D* spr, Color* pal);
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(Region& rgn, Color& color);
	/** Creates a Palette from Color */
	Color* CreatePalette(Color color, Color back);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, Sprite2D* img, bool anchor = false);
	/** Send a Quit Signal to the Event Queue */
	bool Quit();
	/** Get the Palette of a Sprite */
	Color* GetPalette(Sprite2D* spr);
	void* GetVideoSurface()
	{
		return NULL;
	}
public:
	void release(void)
	{
		delete this;
	}
};

#endif
