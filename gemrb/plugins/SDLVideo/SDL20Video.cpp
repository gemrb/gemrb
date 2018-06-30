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
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
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

	SDL_StopTextInput(); // for some reason this is enabled from start

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
		if (drawingBuffer->blend) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		} else {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		}

		return SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
	}

	return 0;
}

void SDL20VideoDriver::Flush()
{
	SDLTextureVideoBuffer* buffer = static_cast<SDLTextureVideoBuffer*>(drawingBuffer);

	UpdateRenderTarget();
	buffer->RenderOnDisplay(renderer);
	buffer->ClearMaskLayer();
}

void SDL20VideoDriver::BlitSpriteNativeClipped(const Sprite2D* spr, const Sprite2D* mask, const SDL_Rect& srect, const SDL_Rect& drect, unsigned int flags, const SDL_Color* tint)
{
	UpdateRenderTarget();

	// we need to isolate flags that require software rendering to use as the "version"
	unsigned int version = (BLIT_GREY|BLIT_SEPIA|BLIT_NOSHADOW|BLIT_TRANSSHADOW) & flags;

	const SDLTextureSprite2D* texSprite = static_cast<const SDLTextureSprite2D*>(spr);
	SDL_Texture* tex = NULL;

	// TODO: handle "shadow" (BLIT_NOSHADOW|BLIT_TRANSSHADOW). I'm not even sure when "shadow" is used.
	// regular lightmap shadows are actually handled via tinting
	// its part of blending, not tinting, so maybe we could handle them with the SpriteCover
	// and simplify things at the same time (now that SpriteCover supports full alpha)

	// WARNING: software fallback == slow
	RenderSpriteVersion(texSprite, version);

	tex = texSprite->GetTexture(renderer);

	if (flags & BLIT_HALFTRANS) {
		SDL_SetTextureAlphaMod(tex, 255/2);
	} else {
		SDL_SetTextureAlphaMod(tex, SDL_ALPHA_OPAQUE);
	}

	if (tint && flags&BLIT_TINTED) {
		if (tint->a != 0xff) {
			SDL_SetTextureAlphaMod(tex, 255-tint->a);
		}
		SDL_SetTextureColorMod(tex, tint->r, tint->g, tint->b);
	} else {
		SDL_SetTextureColorMod(tex, 0xff, 0xff, 0xff);
	}

	SDL_RendererFlip flipflags = (flags&BLIT_MIRRORY) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
	flipflags = static_cast<SDL_RendererFlip>(flipflags | ((flags&BLIT_MIRRORX) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));

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

void SDL20VideoDriver::DrawPoints(const std::vector<Point>& points, const Color& color)
{
	// TODO: refactor Point to use int so this is not needed
	std::vector<SDL_Point> sdlpoints;
	sdlpoints.reserve(points.size());

	for (size_t i = 0; i < points.size(); ++i) {
		const Point& point = points[i];
		SDL_Point sdlpoint = {point.x, point.y};
		sdlpoints.push_back(sdlpoint);
	}

	DrawPoints(sdlpoints, reinterpret_cast<const SDL_Color&>(color));
}

void SDL20VideoDriver::DrawPoints(const std::vector<SDL_Point>& points, const SDL_Color& color)
{
	if (points.empty()) {
		return;
	}
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color));
	SDL_RenderDrawPoints(renderer, &points[0], points.size());
}

void SDL20VideoDriver::DrawPoint(const Point& p, const Color& color)
{
	UpdateRenderTarget(&color);
	SDL_RenderDrawPoint(renderer, p.x, p.y);
}

void SDL20VideoDriver::DrawLines(const std::vector<Point>& points, const Color& color)
{
	// TODO: refactor Point to use int so this is not needed
	std::vector<SDL_Point> sdlpoints;
	sdlpoints.reserve(points.size());

	for (size_t i = 0; i < points.size(); ++i) {
		const Point& point = points[i];
		SDL_Point sdlpoint = {point.x, point.y};
		sdlpoints.push_back(sdlpoint);
	}

	DrawLines(sdlpoints, reinterpret_cast<const SDL_Color&>(color));
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

	SDL_Surface* surface = screenshot->GetSurface();
	SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_BGR24, surface->pixels, surface->pitch);

	return screenshot;
}

int SDL20VideoDriver::ProcessEvent(const SDL_Event & event)
{
	int modstate = GetModState(SDL_GetModState());
	Event e;

	switch (event.type) {
		case SDL_FINGERDOWN: // fallthough
		case SDL_FINGERUP:
			{
				TouchEvent::Finger fingers[1] = { };
				fingers[0].x = event.tfinger.x * screenSize.w;
				fingers[0].y = event.tfinger.y * screenSize.h;
				fingers[0].deltaX = event.tfinger.dx * screenSize.w;
				fingers[0].deltaY = event.tfinger.dy * screenSize.h;
				fingers[0].id = event.tfinger.fingerId;

				e = EvntManager->CreateTouchEvent(fingers, 1, event.type == SDL_FINGERDOWN, event.tfinger.pressure);
				e.mod = modstate;
				EvntManager->DispatchEvent(e);
			}
			break;
		// For swipes only. gestures requireing pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
		case SDL_FINGERMOTION:
			{
				TouchEvent::Finger fingers[1] = { };
				fingers[0].x = event.tfinger.x * screenSize.w;
				fingers[0].y = event.tfinger.y * screenSize.h;
				fingers[0].deltaX = event.tfinger.dx * screenSize.w;
				fingers[0].deltaY = event.tfinger.dy * screenSize.h;
				fingers[0].id = event.tfinger.fingerId;

				Event touch = EvntManager->CreateTouchEvent(fingers, 1, true, event.tfinger.pressure);
				// TODO: it may make more sense to calculate a pinch/rotation from screen center?
				e = EvntManager->CreateTouchGesture(touch.touch, 0.0, 0.0);
				e.mod = modstate;
				EvntManager->DispatchEvent(e);
			}
			break;
		case SDL_DOLLARGESTURE:
			// TODO: this could be useful for predefining gestures
			// might work better than manually programming everything
			break;
		case SDL_MULTIGESTURE:// use this for pinch or rotate gestures. see also SDL_DOLLARGESTURE
			{
				Uint16 numf = Clamp<Uint16>(event.mgesture.numFingers, 0, FINGER_MAX);

				TouchEvent::Finger fingers[FINGER_MAX] = { }; // 0 init
				for (Uint16 i = 0; i < numf; ++i) {
					SDL_Finger* finger = SDL_GetTouchFinger(event.mgesture.touchId, i);
					if (finger == NULL) {
						// FIXME: not sure the best way to handle this
						// it does happen and for now we assume it means the touch ended between the event being enqued and now
						// im not sure the impact to UX by discarding this
						numf = i;
						break;
					}
					
					fingers[i].x = finger->x;
					fingers[i].y = finger->y;

					const TouchEvent::Finger* current = EvntManager->FingerState(i);
					if (current) {
						fingers[i].deltaX = finger->x - current->x;
						fingers[i].deltaY = finger->y - current->y;
					}
				}

				if (numf == 0) {
					// see FIXME above
					break;
				}

				// TODO: it may make more sense to calculate the pressure as an avg?
				Event touch = EvntManager->CreateTouchEvent(fingers, numf, true, 0.0);
				e = EvntManager->CreateTouchGesture(touch.touch, event.mgesture.dTheta, event.mgesture.dDist);
				e.mod = modstate;
				EvntManager->DispatchEvent(e);
			}
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
			e = EvntManager->CreateTextEvent(event.text.text);
			EvntManager->DispatchEvent(e);
			break;
		/* not user input events */

		// TODO: these events will be sent by the D3D renderer and we will need to handle them
		case SDL_RENDER_DEVICE_RESET:
			// TODO: must destroy all SDLTextureSprite2D textures

			// fallthough
		case SDL_RENDER_TARGETS_RESET:
			// TODO: must destroy all SDLTextureVideoBuffer textures
			break;
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
					SDL_Delay(1000);
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

		case SDL_KEYDOWN:
			if (SDL_GetModState() & KMOD_CTRL) {
				switch (event.key.keysym.sym) {
					case SDLK_v:
						if (SDL_HasClipboardText()) {
							char* text = SDL_GetClipboardText();
							e = EvntManager->CreateTextEvent(text);
							SDL_free(text);
							EvntManager->DispatchEvent(e);
							return GEM_OK;
						}
						break;
					default:
						break;
				}
			}
			return SDLVideoDriver::ProcessEvent(event);
		case SDL_MOUSEMOTION: // fallthough
		case SDL_MOUSEBUTTONUP: // fallthough
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.which == SDL_TOUCH_MOUSEID) {
				// ignoring mouse events from touch devices
				// because we handle touch input at the view level
				break;
			}
			// fallthrough
		default:
			return SDLVideoDriver::ProcessEvent(event);
	}
	return GEM_OK;
}

void SDL20VideoDriver::StopTextInput()
{
	SDL_StopTextInput();
}

void SDL20VideoDriver::StartTextInput()
{
	// FIXME: we probably dont need this ANDROID code
	// UseSoftKeyboard probably has no effect since SDL delegates SDL_StartTextInput to the OS
	// on iOS this is going to be a user preference and depends on a physical keyboard presence
#if ANDROID
	if (core->UseSoftKeyboard){
		SDL_StartTextInput();
	} else {
		Event e = EvntManager->CreateTextEvent(L"");
		EvntManager->DispatchEvent(e);
	}
#else
	SDL_StartTextInput();
#endif
}

bool SDL20VideoDriver::InTextInput()
{
	return SDL_IsTextInputActive();
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
		flags = SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_BORDERLESS;
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

#ifndef USE_OPENGL
#include "plugindef.h"


GEMRB_PLUGIN(0xDBAAB51, "SDL2 Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()

#endif
