#ifndef VIDEO_H
#define VIDEO_H

#include "../../includes/globals.h"
#include "Plugin.h"
#include "EventMgr.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Video : public Plugin
{
public:
	Video(void);
	virtual ~Video(void);
	virtual int Init(void) = 0;
	virtual int CreateDisplay(int width, int height, int bpp, bool fullscreen) = 0;
	virtual VideoModes GetVideoModes(bool fullscreen = false) = 0;
	virtual bool TestVideoMode(VideoMode & vm) = 0;
	virtual int SwapBuffers(void) = 0;
	virtual Sprite2D *CreateSprite(int w, int h, int bpp, DWORD rMask, DWORD gMask, DWORD bMask, DWORD aMask, void* pixels, bool cK = false, int index = 0) = 0;
	virtual Sprite2D *CreateSprite8(int w, int h, int bpp, void* pixels, void* palette, bool cK = false, int index = 0) = 0;
	virtual void FreeSprite(Sprite2D * spr) = 0;
	virtual void BlitSprite(Sprite2D * spr, int x, int y, bool anchor = false) = 0;
	virtual void SetCursor(Sprite2D * spr, int x, int y) = 0;
	virtual Region GetViewport(void) = 0;
	virtual void SetViewport(int x, int y) = 0;
	virtual void MoveViewportTo(int x, int y) = 0;
	virtual void ConvertToVideoFormat(Sprite2D * sprite) = 0;
	virtual void CalculateAlpha(Sprite2D * sprite) = 0;
	/** No descriptions */
	virtual void SetPalette(Sprite2D * spr, Color * pal) = 0;
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	virtual void DrawRect(Region &rgn, Color &color) = 0;
	/** Creates a Palette from Color */
	virtual Color * CreatePalette(Color color, Color back) = 0;
	/** Blits a Sprite filling the Region */
	virtual void BlitTiled(Region rgn, Sprite2D * img, bool anchor = false) = 0;
	/** Set Event Manager */
	void SetEventMgr(EventMgr * evnt);
	/** Event Manager Pointer */
	EventMgr * Evnt;
};

#endif
