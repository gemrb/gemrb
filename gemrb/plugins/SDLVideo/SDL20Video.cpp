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

#include "Interface.h"

using namespace GemRB;

SDL20VideoDriver::SDL20VideoDriver(void)
{
	renderer = NULL;
	window = NULL;
}

SDL20VideoDriver::~SDL20VideoDriver(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

int SDL20VideoDriver::CreateDriverDisplay(const Size& s, int bpp, const char* title)
{
	screenSize = s;
	this->bpp = bpp;

	Log(MESSAGE, "SDL 2 Driver", "Creating display");
	// TODO: scale methods can be nearest or linear, and should be settable in config
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#if TARGET_OS_IPHONE || ANDROID
	// this allows the user to flip the device upsidedown if they wish and have the game rotate.
	// it also for some unknown reason is required for retina displays
	winFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;
	// this hint is set in the wrapper for iPad at a higher priority. set it here for iPhone
	// don't know if Android makes use of this.
	SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight LandscapeLeft", SDL_HINT_DEFAULT);
#endif
	if (fullscreen) {
		winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		//This is needed to remove the status bar on Android/iOS.
		//since we are in fullscreen this has no effect outside Android/iOS
		winFlags |= SDL_WINDOW_BORDERLESS;
	}
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, s.w, s.h, winFlags);
	if (window == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create window:%s", SDL_GetError());
		return GEM_ERROR;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_ACCELERATED);
	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	Log(DEBUG, "SDL20Video", "Renderer: %s", info.name);

	if (renderer == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create renderer:%s", SDL_GetError());
		return GEM_ERROR;
	}

	// we set logical size so that platforms where the window can be a diffrent size then requested
	// function properly. eg iPhone and Android the requested size may be 640x480,
	// but the window will always be the size of the screen
	SDL_RenderSetLogicalSize(renderer, screenSize.w, screenSize.h);
	SDL_GetRendererOutputSize(renderer, &screenSize.w, &screenSize.h);

	return GEM_OK;
}

VideoBuffer* SDL20VideoDriver::NewVideoBuffer(const Region& r, BufferFormat fmt)
{
	Uint32 format = SDLPixelFormatFromBufferFormat(fmt, renderer);
	if (format == SDL_PIXELFORMAT_UNKNOWN)
		return NULL;
	
	SDL_Texture* tex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, r.w, r.h);
	if (format == RGBA8888)
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	return new SDLTextureVideoBuffer(r.Origin(), tex, fmt, renderer);
}

void SDL20VideoDriver::SwapBuffers(VideoBuffers& buffers)
{
	SDL_SetRenderTarget(renderer, NULL);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	VideoBuffers::iterator it;
	it = buffers.begin();
	for (; it != buffers.end(); ++it) {
		(*it)->RenderOnDisplay(renderer);
	}

	SDL_RenderPresent( renderer );

	it = buffers.begin();
	for (; it != buffers.end(); ++it) {
		static_cast<SDLTextureVideoBuffer*>(*it)->ClearMaskLayer();
	}
}

SDLVideoDriver::vid_buf_t* SDL20VideoDriver::CurrentRenderBuffer()
{
	assert(drawingBuffer);
	return static_cast<SDLTextureVideoBuffer*>(drawingBuffer)->GetTexture();
}

int SDL20VideoDriver::UpdateRenderTarget(const Color* color)
{
	SDL_Texture* target = CurrentRenderBuffer();

	assert(target);
	int ret = SDL_SetRenderTarget(renderer, target);
	if (ret != 0) {
		Log(ERROR, "SDLVideo", "%s", SDL_GetError());
		return ret;
	}

	if (color) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		return SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
	}

	return 0;
}

void SDL20VideoDriver::BlitSpriteNativeClipped(const Sprite2D* spr, const Sprite2D* mask, const SDL_Rect& srect, const SDL_Rect& drect, unsigned int flags, const SDL_Color* tint)
{
	UpdateRenderTarget();

	// we need to isolate flags that require software rendering to use as the "version"
	unsigned int version = (BLIT_GREY|BLIT_SEPIA) & flags;

	const SDLTextureSprite2D* texSprite = static_cast<const SDLTextureSprite2D*>(spr);
	unsigned int currentVer = texSprite->GetVersion();
	SDL_Texture* tex = NULL;

	if (currentVer == version) {
		tex = texSprite->GetTexture(renderer);
	}

	// TODO: handle "shadow" (BLIT_NOSHADOW|BLIT_TRANSSHADOW). I'm not even sure when "shadow" is used.
	// regular lightmap shadows are actually handled via tinting
	// its part of blending, not tinting, so maybe we could handle them with the SpriteCover
	// and simplify things at the same time (now that SpriteCover supports full alpha)

	if (currentVer != version) {
		// WARNING: software fallback == slow

		// TODO: this is still an extra blit
		// making a new surface and tinting pixels while we manually copy them would be cheaper
		SDL_Surface* newV = texSprite->NewVersion(version);
		SDL_LockSurface(newV);
		if (flags & BLIT_GREY) {
			
		} else if (flags & BLIT_SEPIA) {

		}
		SDL_UnlockSurface(newV);

		tex = texSprite->GetTexture(renderer);
	} else if (tint && flags&BLIT_TINTED) {
		SDL_SetTextureColorMod(tex, tint->r, tint->g, tint->b);
		//SDL_SetTextureAlphaMod(tex, tint->a);
	} else {
		SDL_SetTextureColorMod(tex, 0xff, 0xff, 0xff);
	}

	if (flags & BLIT_HALFTRANS) {
		SDL_SetTextureAlphaMod(tex, 255/2);
	} else {
		SDL_SetTextureAlphaMod(tex, SDL_ALPHA_OPAQUE);
	}

	SDL_RendererFlip flipflags = (spr->renderFlags&BLIT_MIRRORY) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
	flipflags = static_cast<SDL_RendererFlip>(flipflags | ((spr->renderFlags&BLIT_MIRRORX) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));

	int ret = 0;
	if (mask) {
		static SDL_BlendMode blender = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

		SDL_Texture* maskLayer = static_cast<SDLTextureVideoBuffer*>(drawingBuffer)->GetMaskLayer();
		SDL_Texture* maskTex = static_cast<const SDLTextureSprite2D*>(mask)->GetTexture(renderer);
		SDL_SetRenderTarget(renderer, maskLayer);
		SDL_SetTextureBlendMode(maskLayer, SDL_BLENDMODE_BLEND);
		SDL_SetTextureBlendMode(maskTex, blender);
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
		SDL_RenderCopyEx(renderer, tex, &srect, &drect, 0.0, NULL, flipflags);
		ret = SDL_RenderCopyEx(renderer, maskTex, &srect, &drect, 0.0, NULL, flipflags);
	} else {
		ret = SDL_RenderCopyEx(renderer, tex, &srect, &drect, 0.0, NULL, flipflags);
	}

	if (ret != 0) {
		Log(ERROR, "SDLVideo", "%s", SDL_GetError());
	}
}

void SDL20VideoDriver::DrawPoints(const std::vector<SDL_Point>& points, const SDL_Color& color)
{
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color));
	SDL_RenderDrawPoints(renderer, &points[0], points.size());
}

void SDL20VideoDriver::DrawPoint(const Point& p, const Color& color)
{
	UpdateRenderTarget(&color);
	SDL_RenderDrawPoint(renderer, p.x, p.y);
}

void SDL20VideoDriver::DrawLines(const std::vector<SDL_Point>& points, const SDL_Color& color)
{
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color));
	SDL_RenderDrawLines(renderer, &points[0], points.size());
}

void SDL20VideoDriver::DrawLine(const Point& p1, const Point& p2, const Color& color)
{
	UpdateRenderTarget(&color);
	SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
}

void SDL20VideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill)
{
	UpdateRenderTarget(&color);
	if (fill) {
		SDL_RenderFillRect(renderer, reinterpret_cast<const SDL_Rect*>(&rgn));
	} else {
		SDL_RenderDrawRect(renderer, reinterpret_cast<const SDL_Rect*>(&rgn));
	}
}

Sprite2D* SDL20VideoDriver::GetScreenshot( Region r )
{
	SDL_Rect rect = RectFromRegion(r);

	unsigned int Width = r.w ? r.w : screenSize.w;
	unsigned int Height = r.h ? r.h : screenSize.h;

	SDLTextureSprite2D* screenshot = new SDLTextureSprite2D(Width, Height, 24,
															0x00ff0000, 0x0000ff00, 0x000000ff, 0);

	SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_RGB24, screenshot->LockSprite(), Width);
	screenshot->UnlockSprite();

	return screenshot;
}

int SDL20VideoDriver::ProcessEvent(const SDL_Event & event)
{
	Event e;
	switch (event.type) {
		// For swipes only. gestures requireing pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
		case SDL_FINGERMOTION:
			break;
		case SDL_FINGERDOWN:
			break;
		case SDL_FINGERUP:
			break;
		case SDL_MULTIGESTURE:// use this for pinch or rotate gestures. see also SDL_DOLLARGESTURE
			break;
		case SDL_MOUSEWHEEL:
			/*
			 TODO: need a preference for inverting these
			 sdl 2.0.4 autodetects (SDL_MOUSEWHEEL_FLIPPED in SDL_MouseWheelEvent)
			 */
			e = EvntManager->CreateMouseWheelEvent(Point(event.wheel.x, event.wheel.y));
			EvntManager->DispatchEvent(e);
			break;
		/* not user input events */
		case SDL_TEXTINPUT:
			for (size_t i=0; i < strlen(event.text.text); i++) {
				//EvntManager->KeyPress( event.text.text[i], GetModState(event.key.keysym.mod));
			}
			break;
		/* not user input events */
		case SDL_WINDOWEVENT://SDL 1.2
			switch (event.window.event) {
				case SDL_WINDOWEVENT_MINIMIZED://SDL 1.3
					// We pause the game and audio when the window is minimized.
					// on iOS/Android this happens when leaving the application or when play is interrupted (ex phone call)
					// if win/mac/linux has a problem with this behavior we can work something out.
					core->GetAudioDrv()->Pause();//this is for ANDROID mostly
					core->SetPause(PAUSE_ON);
					break;
				case SDL_WINDOWEVENT_RESTORED: //SDL 1.3
					/*
					 reset all input variables as if no events have happened yet
					 restoring from "minimized state" should be a clean slate.
					 */

#if TARGET_OS_IPHONE
					// FIXME:
					// sleep for a short while to avoid some unknown Apple threading issue with OpenAL threads being suspended
					// even using Apple examples of how to properly suspend an OpenAL context and resume on iOS are falling flat
					// it could be this bug affects only the simulator.
					sleep(1);
#endif
					core->GetAudioDrv()->Resume();//this is for ANDROID mostly
					break;
					/*
				case SDL_WINDOWEVENT_RESIZED: //SDL 1.2
					// this event exists in SDL 1.2, but this handler is only getting compiled under 1.3+
					Log(WARNING, "SDL 2 Driver",  "Window resized so your window surface is now invalid.");
					break;
					 */
			}
			break;
		default:
			return SDLVideoDriver::ProcessEvent(event);
	}
	return GEM_OK;
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::HideSoftKeyboard()
{
	/*
	if(core->UseSoftKeyboard){
		SDL_StopTextInput();
		if(core->ConsolePopped) core->PopupConsole();
	}
	*/
}

/*
 This method is intended for devices with no physical keyboard or with an optional soft keyboard (iOS/Android)
 */
void SDL20VideoDriver::ShowSoftKeyboard()
{
	/*
	if(core->UseSoftKeyboard){
		SDL_StartTextInput();
	}
	*/
}

bool SDL20VideoDriver::TouchInputEnabled()
{
	// note from upstream: on some platforms a device may become seen only after use
	return SDL_GetNumTouchDevices() > 0;
}

void SDL20VideoDriver::SetGamma(int brightness, int /*contrast*/)
{
	// FIXME: hardcoded hack. in in Interface our default brigtness value is 10
	// so we assume that to be "normal" (1.0) value.
	SDL_SetWindowBrightness(window, (float)brightness/10.0);
}

bool SDL20VideoDriver::SetFullscreenMode(bool set)
{
	Uint32 flags = 0;
	if (set) {
	flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	if (SDL_SetWindowFullscreen(window, flags) == GEM_OK) {
		fullscreen = set;
		return true;
	}
	return false;
}

bool SDL20VideoDriver::ToggleGrabInput()
{
	bool isGrabbed = SDL_GetWindowGrab(window);
	SDL_SetWindowGrab(window, (SDL_bool)!isGrabbed);
	return (isGrabbed != SDL_GetWindowGrab(window));
}

// Private methods

bool SDL20VideoDriver::SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)
{
	bool ret = SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	if (ret == GEM_OK) {
		ret = SDL_SetSurfaceAlphaMod(surface, alpha);
	}
	// don't RLE with SDL 2
	// this only benifits SDL_BlitSurface which we don't use. its a slowdown for us.
	// SDL_SetSurfaceRLE(surface, SDL_TRUE);
	return ret;
}

#ifndef USE_OPENGL
#include "plugindef.h"


GEMRB_PLUGIN(0xDBAAB51, "SDL2 Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()

#endif
