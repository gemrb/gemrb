#ifndef SDLVIDEODRIVER_H
#define SDLVIDEODRIVER_H

#include "../Core/Video.h"
#include "../../includes/sdl/SDL.h"

class SDLVideoDriver : public Video
{
private:
	SDL_Surface * disp;
	std::vector<Region> upd;	//Regions of the Screen to Update in the next SwapBuffer operation.
	Region Viewport;
public:
	SDLVideoDriver(void);
	~SDLVideoDriver(void);
	int Init(void);
	int CreateDisplay(int width, int height, int bpp, bool fullscreen);
	VideoModes GetVideoModes(bool fullscreen = false);
	bool TestVideoMode(VideoMode & vm);
	int SwapBuffers(void);
	Sprite2D *CreateSprite(int w, int h, int bpp, DWORD rMask, DWORD gMask, DWORD bMask, DWORD aMask, void* pixels, bool cK = false, int index = 0);
	Sprite2D *CreateSprite8(int w, int h, int bpp, void* pixels, void* palette, bool cK = false, int index = 0);
	void FreeSprite(Sprite2D * spr);
	void BlitSprite(Sprite2D * spr, int x, int y, bool anchor = false, Region * clip = NULL);
	void BlitSpriteRegion(Sprite2D * spr, Region &size, int x, int y, bool anchor = true, Region * clip = NULL);
	void SetCursor(Sprite2D * spr, int x, int y);
	Region GetViewport(void);
	void SetViewport(int x, int y);
	void MoveViewportTo(int x, int y);
	void ConvertToVideoFormat(Sprite2D * sprite);
	void CalculateAlpha(Sprite2D * sprite);
	/** No descriptions */
	void SetPalette(Sprite2D * spr, Color * pal);
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(Region &rgn, Color &color);
	/** Creates a Palette from Color */
	Color * CreatePalette(Color color, Color back);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, Sprite2D * img, bool anchor = false);
	/** Send a Quit Signal to the Event Queue */
	bool Quit();
	/** Get the Palette of a Sprite */
	Color * GetPalette(Sprite2D * spr);
	void * GetVideoSurface()
	{
		return disp;
	}
};

#endif
