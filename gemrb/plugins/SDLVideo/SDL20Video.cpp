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

#ifdef BAKE_ICON
#include "gemrb-icon.h"
#endif

#include "Interface.h"

using namespace GemRB;

SDL20VideoDriver::SDL20VideoDriver(void)
{
	renderer = NULL;
	window = NULL;

	stencilAlphaBlender = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE,
													 SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO,
													 SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

	// WARNING: do _not_ call opengl here
	// all function pointers will be NULL
	// until after SDL_CreateRenderer is called

	SDL_version ver;
	SDL_GetVersion(&ver);
	sdl2_runtime_version = SDL_VERSIONNUM(ver.major, ver.minor, ver.patch);
}

SDL20VideoDriver::~SDL20VideoDriver(void)
{
	if (SDL_GameControllerGetAttached(gameController)) {
		SDL_GameControllerClose(gameController);
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	delete stencilShader;
	delete spriteShader;
}

int SDL20VideoDriver::Init()
{
	int ret = SDLVideoDriver::Init();

	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1) {
		Log(ERROR, "SDL2", "InitSubSystem failed: %s", SDL_GetError());
	} else {
		for (int i = 0; i < SDL_NumJoysticks(); ++i) {
			if (SDL_IsGameController(i)) {
				gameController = SDL_GameControllerOpen(i);
				if (gameController != nullptr) {
					break;
				}
			}
		}
	}

	return ret;
}

static void SetWindowIcon(SDL_Window* window)
{
	// gemrb_icon and gemrb_icon_size are generated in gemrb-icon.h
	SDL_RWops* const iconStream = SDL_RWFromConstMem((void *) gemrb_icon, gemrb_icon_size);
	if (!iconStream) {
		Log(WARNING, "SDL 2 Driver", "Failed to create icon stream.");
		return;
	}
	SDL_Surface* const windowIcon = SDL_LoadBMP_RW(iconStream, 1);
	if (!windowIcon) {
		Log(WARNING, "SDL 2 Driver", "Failed to read icon BMP from stream.");
		return;
	}
	SDL_SetWindowIcon(window, windowIcon);
	SDL_FreeSurface(windowIcon);
}

int SDL20VideoDriver::CreateSDLDisplay(const char* title)
{
	Log(MESSAGE, "SDL 2 Driver", "Creating display");
	// TODO: scale methods can be nearest or linear, and should be settable in config
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

#if USE_OPENGL_BACKEND
#if USE_OPENGL_API
	const char* driverName = "opengl";
#elif USE_GLES_API
	const char* driverName = "opengles2";
#endif
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, driverName);
#endif

#if SDL_VERSION_ATLEAST(2, 0, 10)
	if (sdl2_runtime_version >= SDL_VERSIONNUM(2,0,10)) {
		SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
	}
#endif

	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if USE_OPENGL_BACKEND
	winFlags |= SDL_WINDOW_OPENGL;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#endif

	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenSize.w, screenSize.h, winFlags);
	if (window == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create window:%s", SDL_GetError());
		return GEM_ERROR;
	}

#ifdef BAKE_ICON
	SetWindowIcon(window);
#endif

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_ACCELERATED);
	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	Log(DEBUG, "SDL20Video", "Renderer: %s", info.name);

	if (renderer == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create renderer:%s", SDL_GetError());
		return GEM_ERROR;
	}

#if USE_OPENGL_BACKEND
	Log(MESSAGE, "SDL 2 GL Driver", "OpenGL version: %s, renderer: %s, vendor: %s", glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_VENDOR));
	Log(MESSAGE, "SDL 2 GL Driver", "  GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

	if (strcmp(info.name, driverName) != 0) {
		Log(FATAL, "SDL 2 GL Driver", "OpenGL backend must be used instead of %s", info.name);
		return GEM_ERROR;
	}

#if defined(_WIN32) && defined(USE_OPENGL_API)
	glewInit();
#endif

	// must follow SDL_CreateRenderer
	stencilShader = GLSLProgram::CreateFromFiles("Shaders/SDLTextureV.glsl", "Shaders/StencilF.glsl");
	if (!stencilShader)
	{
		std::string msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return GEM_ERROR;
	}

	spriteShader = GLSLProgram::CreateFromFiles("Shaders/SDLTextureV.glsl", "Shaders/GameSpriteF.glsl");
	if (!spriteShader)
	{
		std::string msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return GEM_ERROR;
	}
#endif

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
}

SDLVideoDriver::vid_buf_t* SDL20VideoDriver::ScratchBuffer() const
{
	assert(scratchBuffer);
	return std::static_pointer_cast<SDLTextureVideoBuffer>(scratchBuffer)->GetTexture();
}

SDLVideoDriver::vid_buf_t* SDL20VideoDriver::CurrentRenderBuffer() const
{
	assert(drawingBuffer);
	return static_cast<SDLTextureVideoBuffer*>(drawingBuffer)->GetTexture();
}

SDLVideoDriver::vid_buf_t* SDL20VideoDriver::CurrentStencilBuffer() const
{
	assert(stencilBuffer);
	return std::static_pointer_cast<SDLTextureVideoBuffer>(stencilBuffer)->GetTexture();
}

int SDL20VideoDriver::UpdateRenderTarget(const Color* color, uint32_t flags)
{
	// TODO: add support for BLIT_HALFTRANS, BLIT_COLOR_MOD, and others (no use for them ATM)

	SDL_Texture* target = CurrentRenderBuffer();

	assert(target);
	int ret = SDL_SetRenderTarget(renderer, target);
	if (ret != 0) {
		Log(ERROR, "SDLVideo", "%s", SDL_GetError());
		return ret;
	}

	if (screenClip.Dimensions() == screenSize)
	{
		// Some SDL backends complain on having a clip rect of the entire renderer size
		// I'm not sure if it is an SDL bug; possibly its just 0 based so it is out of bounds?
		SDL_RenderSetClipRect(renderer, NULL);
	} else {
		SDL_RenderSetClipRect(renderer, reinterpret_cast<SDL_Rect*>(&screenClip));
	}

	if (color) {
		if (flags & BLIT_BLENDED) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		} else if (flags & BLIT_MULTIPLY) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);
		} else {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		}

		return SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
	}

	return 0;
}

void SDL20VideoDriver::BlitSpriteNativeClipped(const SDLTextureSprite2D* spr, const SDL_Rect& src, const SDL_Rect& dst, uint32_t flags, const SDL_Color* tint)
{
	uint32_t version = 0;
#if 1 // !USE_OPENGL_BACKEND
	// we need to isolate flags that require software rendering to use as the "version"
	version = (BLIT_GREY|BLIT_SEPIA) & flags;
#endif
	// WARNING: software fallback == slow
	if (spr->Bpp == 8 && (flags & BLIT_ALPHA_MOD)) {
		version |= BLIT_ALPHA_MOD;
		flags &= ~RenderSpriteVersion(spr, version, reinterpret_cast<const Color*>(tint));
	} else {
		flags &= ~RenderSpriteVersion(spr, version);
	}

	SDL_Texture* tex = spr->GetTexture(renderer);
	BlitSpriteNativeClipped(tex, src, dst, flags, tint);
}

void SDL20VideoDriver::BlitSpriteNativeClipped(SDL_Texture* texSprite, const SDL_Rect& srect, const SDL_Rect& drect, uint32_t flags, const SDL_Color* tint)
{
	int ret = 0;
	if (flags&BLIT_STENCIL_MASK) {
		// 1. clear scratchpad segment
		// 2. blend stencil segment to scratchpad
		// 3. blend texture to scratchpad
		// 4. copy scratchpad segment to screen

		SDL_Texture* stencilTex = CurrentStencilBuffer();
		SDL_SetTextureBlendMode(stencilTex, stencilAlphaBlender);

		std::static_pointer_cast<SDLTextureVideoBuffer>(scratchBuffer)->Clear(drect); // sets the render target to the scratch buffer

		RenderCopyShaded(texSprite, &srect, &drect, flags, tint);

#if USE_OPENGL_BACKEND
#if SDL_VERSION_ATLEAST(2, 0, 10)
		SDL_RenderFlush(renderer);
#endif

		GLint previous_program;
		glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);

		GLint channel = 3;
		if (flags&BLIT_STENCIL_RED) {
			channel = 0;
		} else if (flags&BLIT_STENCIL_GREEN) {
			channel = 1;
		} else if (flags&BLIT_STENCIL_BLUE) {
			channel = 2;
		}

		stencilShader->Use();
		stencilShader->SetUniformValue("u_channel", 1, channel);
		if (flags & BLIT_STENCIL_DITHER) {
			stencilShader->SetUniformValue("u_dither", 1, 1);
		} else {
			stencilShader->SetUniformValue("u_dither", 1, 0);
		}
		
		stencilShader->SetUniformValue("s_stencil", 1, 0);
		glActiveTexture(GL_TEXTURE0);
		SDL_GL_BindTexture(stencilTex, nullptr, nullptr);

		SDL_Rect stencilRect = drect;
		stencilRect.x -= stencilBuffer->Origin().x;
		stencilRect.y -= stencilBuffer->Origin().y;
		SDL_RenderCopy(renderer, stencilTex, &stencilRect, &drect);

		SDL_GL_UnbindTexture(stencilTex);
		glUseProgram(previous_program);
#else
		// alpha masking only
		SDL_RenderCopy(renderer, stencilTex, &drect, &drect);
#endif

		SDL_SetRenderTarget(renderer, CurrentRenderBuffer());
		SDL_SetTextureBlendMode(ScratchBuffer(), SDL_BLENDMODE_BLEND);
		ret = SDL_RenderCopy(renderer, ScratchBuffer(), &drect, &drect);
	} else {
		UpdateRenderTarget();
		ret = RenderCopyShaded(texSprite, &srect, &drect, flags, tint);
	}

	if (ret != 0) {
		Log(ERROR, "SDLVideo", "%s", SDL_GetError());
	}
}

void SDL20VideoDriver::BlitVideoBuffer(const VideoBufferPtr& buf, const Point& p, uint32_t flags, const Color* tint, const Region* clip)
{
	auto tex = static_cast<SDLTextureVideoBuffer&>(*buf).GetTexture();
	const Region& r = buf->Rect();
	Point origin = r.Origin() + p;

	if (clip) {
		SDL_Rect drect = {origin.x, origin.y, clip->w, clip->h};
		BlitSpriteNativeClipped(tex, RectFromRegion(*clip), drect, flags, reinterpret_cast<const SDL_Color*>(tint));
	} else {
		SDL_Rect srect = {0, 0, r.w, r.h};
		SDL_Rect drect = {origin.x, origin.y, r.w, r.h};
		BlitSpriteNativeClipped(tex, srect, drect, flags, reinterpret_cast<const SDL_Color*>(tint));
	}
}

int SDL20VideoDriver::RenderCopyShaded(SDL_Texture* texture, const SDL_Rect* srcrect,
									   const SDL_Rect* dstrect, Uint32 flags, const SDL_Color* tint)
{
#if 0 // USE_OPENGL_BACKEND
	GLint previous_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);

	spriteShader->Use();
	if (flags&BLIT_GREY) {
		spriteShader->SetUniformValue("u_greyMode", 1, 1);
	} else if (flags&BLIT_SEPIA) {
		spriteShader->SetUniformValue("u_greyMode", 1, 2);
	} else {
		spriteShader->SetUniformValue("u_greyMode", 1, 0);
	}

	spriteShader->SetUniformValue("s_sprite", 1, 0);
#else
	// "shaders" were already applied via software (RenderSpriteVersion)
	// they had to be applied very first so we could create a texture from the software rendering
#endif
	Uint8 alpha = SDL_ALPHA_OPAQUE;
	if (flags & BLIT_ALPHA_MOD) {
		alpha = tint->a;
	}
	
	if (flags & BLIT_HALFTRANS) {
		alpha /= 2;
	}
	
	SDL_SetTextureAlphaMod(texture, alpha);

	if (flags & BLIT_COLOR_MOD) {
		SDL_SetTextureColorMod(texture, tint->r, tint->g, tint->b);
	} else {
		SDL_SetTextureColorMod(texture, 0xff, 0xff, 0xff);
	}
	
	if (flags & BLIT_ADD) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
	} else if (flags & BLIT_MULTIPLY) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
	} else if (flags & (BLIT_BLENDED | BLIT_HALFTRANS)) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	} else {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	}
	
	SDL_RendererFlip flipflags = (flags&BLIT_MIRRORY) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
	flipflags = static_cast<SDL_RendererFlip>(flipflags | ((flags&BLIT_MIRRORX) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));

#if 0 // USE_OPENGL_BACKEND
	int ret = SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, 0.0, NULL, flipflags);

	return ret;
#else
	return SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, 0.0, NULL, flipflags);
#endif
}

void SDL20VideoDriver::DrawPointsImp(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	// TODO: refactor Point to use int so this is not needed
	std::vector<SDL_Point> sdlpoints;
	sdlpoints.reserve(points.size());

	for (size_t i = 0; i < points.size(); ++i) {
		const Point& point = points[i];
		SDL_Point sdlpoint = {point.x, point.y};
		sdlpoints.push_back(sdlpoint);
	}

	DrawSDLPoints(sdlpoints, reinterpret_cast<const SDL_Color&>(color), flags);
}

void SDL20VideoDriver::DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, uint32_t flags)
{
	if (points.empty()) {
		return;
	}
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color), flags);
	SDL_RenderDrawPoints(renderer, &points[0], int(points.size()));
}

void SDL20VideoDriver::DrawPointImp(const Point& p, const Color& color, uint32_t flags)
{
	UpdateRenderTarget(&color, flags);
	SDL_RenderDrawPoint(renderer, p.x, p.y);
}

void SDL20VideoDriver::DrawLinesImp(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	// TODO: refactor Point to use int so this is not needed
	std::vector<SDL_Point> sdlpoints;
	sdlpoints.reserve(points.size());

	for (size_t i = 0; i < points.size(); ++i) {
		const Point& point = points[i];
		SDL_Point sdlpoint = {point.x, point.y};
		sdlpoints.push_back(sdlpoint);
	}

	DrawSDLLines(sdlpoints, reinterpret_cast<const SDL_Color&>(color), flags);
}

void SDL20VideoDriver::DrawSDLLines(const std::vector<SDL_Point>& points, const SDL_Color& color, uint32_t flags)
{
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color), flags);
	SDL_RenderDrawLines(renderer, &points[0], int(points.size()));
}

void SDL20VideoDriver::DrawLineImp(const Point& p1, const Point& p2, const Color& color, uint32_t flags)
{
	UpdateRenderTarget(&color, flags);
	SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
}

void SDL20VideoDriver::DrawRectImp(const Region& rgn, const Color& color, bool fill, uint32_t flags)
{
	UpdateRenderTarget(&color, flags);
	if (fill) {
		SDL_RenderFillRect(renderer, reinterpret_cast<const SDL_Rect*>(&rgn));
	} else {
		SDL_RenderDrawRect(renderer, reinterpret_cast<const SDL_Rect*>(&rgn));
	}
}

void SDL20VideoDriver::DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, uint32_t flags)
{
	if (fill) {
		UpdateRenderTarget(&color, flags);

		for (const auto& lineSegments : poly->rasterData)
		{
			for (const auto& segment : lineSegments) {
				// SDL_RenderDrawLines actually is for drawing polygons so it is, ironically, not what we want
				// when drawing the "rasterized" data. doing so would work ok most of the time, but other times
				// the reconnection of the last to first point (done by SDL) will be visible
				Point p1(segment.first + origin);
				Point p2(segment.second + origin);
				SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
			}
		}
	} else {
		std::vector<SDL_Point> points(poly->Count() + 1);
		size_t i = 0;
		for (; i < poly->Count(); ++i) {
			const Point& p = poly->vertices[i] - poly->BBox.Origin() + origin;
			points[i].x = p.x;
			points[i].y = p.y;
		}

		// close the polygon with first point
		points[i] = points[0];

		DrawSDLLines(points, reinterpret_cast<const SDL_Color&>(color), flags);
	}
}

Holder<Sprite2D> SDL20VideoDriver::GetScreenshot(Region r, const VideoBufferPtr& buf)
{
	SDL_Rect rect = RectFromRegion(r);

	unsigned int Width = r.w ? r.w : screenSize.w;
	unsigned int Height = r.h ? r.h : screenSize.h;

	SDLTextureSprite2D* screenshot = new SDLTextureSprite2D(Region(0,0, Width, Height), 24,
															0x00ff0000, 0x0000ff00, 0x000000ff, 0);

	SDL_Texture* target = SDL_GetRenderTarget(renderer);
	if (buf) {
		auto texture = static_cast<SDLTextureVideoBuffer*>(drawingBuffer)->GetTexture();
		SDL_SetRenderTarget(renderer, texture);
	} else {
		SDL_SetRenderTarget(renderer, nullptr);
	}

	SDL_Surface* surface = screenshot->GetSurface();
	SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_BGR24, surface->pixels, surface->pitch);

	SDL_SetRenderTarget(renderer, target);

	return screenshot;
}

int SDL20VideoDriver::GetTouchFingers(TouchEvent::Finger(&fingers)[FINGER_MAX], SDL_TouchID device) const
{
	int numf = SDL_GetNumTouchFingers(device);

	for (int i = 0; i < numf; ++i) {
		SDL_Finger* finger = SDL_GetTouchFinger(device, i);
		assert(finger);

		fingers[i].id = finger->id;
		fingers[i].x = finger->x * screenSize.w;
		fingers[i].y = finger->y * screenSize.h;

		const TouchEvent::Finger* current = EvntManager->FingerState(finger->id);
		if (current) {
			fingers[i].deltaX = fingers[i].x - current->x;
			fingers[i].deltaY = fingers[i].y - current->y;
		}
	}

	return numf;
}

int SDL20VideoDriver::ProcessEvent(const SDL_Event & event)
{
	int modstate = GetModState(SDL_GetModState());
	Event e;

	switch (event.type) {
		case SDL_CONTROLLERDEVICEREMOVED:
			if (gameController != nullptr) {
				SDL_GameController *removedController = SDL_GameControllerFromInstanceID(event.jdevice.which);
				if (removedController == gameController) {
					SDL_GameControllerClose(gameController);
					gameController = nullptr;
				}
			}
			break;
		case SDL_CONTROLLERDEVICEADDED:
			// I guess we assume that if you plug in a device while play that is the one to use?
			if (gameController == nullptr) {
				gameController = SDL_GameControllerOpen(event.jdevice.which);
			}
			break;
		case SDL_CONTROLLERAXISMOTION:
			{
				float pct = event.caxis.value / float(sizeof(Sint16));
				bool xaxis = event.caxis.axis % 2;
				// FIXME: I'm sure this delta needs to be scaled
				int delta = (xaxis) ? pct * screenSize.w : pct * screenSize.h;
				InputAxis axis = InputAxis(event.caxis.axis);
				e = EvntManager->CreateControllerAxisEvent(axis, delta, pct);
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			{
				bool down = (event.type == SDL_JOYBUTTONDOWN) ? true : false;
				EventButton btn = EventButton(event.cbutton.button);
				e = EvntManager->CreateControllerButtonEvent(btn, down);
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
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
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		// For swipes only. gestures requireing pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
		case SDL_FINGERMOTION:
			{
				TouchEvent::Finger fingers[FINGER_MAX] = { }; // 0 init
				int numf = GetTouchFingers(fingers, event.mgesture.touchId);

				Event touch = EvntManager->CreateTouchEvent(fingers, numf, true, event.tfinger.pressure);
				// TODO: it may make more sense to calculate a pinch/rotation from screen center?
				e = EvntManager->CreateTouchGesture(touch.touch, 0.0, 0.0);
				e.mod = modstate;
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		case SDL_DOLLARGESTURE:
			// TODO: this could be useful for predefining gestures
			// might work better than manually programming everything
			break;
		case SDL_MULTIGESTURE:// use this for pinch or rotate gestures. see also SDL_DOLLARGESTURE
			{
				TouchEvent::Finger fingers[FINGER_MAX] = { }; // 0 init
				int numf = GetTouchFingers(fingers, event.mgesture.touchId);

				// TODO: it may make more sense to calculate the pressure as an avg?
				Event touch = EvntManager->CreateTouchEvent(fingers, numf, true, 0.0);
				e = EvntManager->CreateTouchGesture(touch.touch, event.mgesture.dTheta, event.mgesture.dDist);
				if (e.gesture.deltaX != 0 || e.gesture.deltaY != 0)
				{
					e.mod = modstate;
					EvntManager->DispatchEvent(std::move(e));
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			{
				if (SDL_TOUCH_MOUSEID == event.wheel.which) {
					break;
				}
				
				// HACK: some mouse devices report the delta in pixels, but others (like regular mouse wheels) are going to be in "clicks"
				// there is no good way to find which is the case so heuristically we will just switch if we see a delta larger than one
				// hopefully no devices will be merging several repeated whell clicks together
				static bool unitIsPixels = false;
				if (event.wheel.y > 1 || event.wheel.x > 1) {
					unitIsPixels = true;
				}
				
				int speed = (unitIsPixels) ? 1 : core->GetMouseScrollSpeed();
				if (SDL_GetModState() & KMOD_SHIFT) {
					e = EvntManager->CreateMouseWheelEvent(Point(event.wheel.y * speed, event.wheel.x * speed));
				} else {
					e = EvntManager->CreateMouseWheelEvent(Point(event.wheel.x * speed, event.wheel.y * speed));
				}
				
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		/* not user input events */
		case SDL_TEXTINPUT:
			e = EvntManager->CreateTextEvent(event.text.text);
			EvntManager->DispatchEvent(std::move(e));
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

		// conditionally handle mouse events
		// discard them if they are produced by touch events
		// do NOT discard mouse wheel events
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (event.button.which == SDL_TOUCH_MOUSEID) {
				// ignoring mouse events from touch devices
				// because we handle touch input at the view level
				break;
			} else {
				/**
				 * As being SDL2-only, try to query the clipboard state to
				 * paste when middle clicking the mouse.
				 */
				
				if (event.button.button == SDL_BUTTON_MIDDLE
					&& event.type == SDL_MOUSEBUTTONDOWN
					&& SDL_HasClipboardText()
				) {
					char *pasteValue = SDL_GetClipboardText();

					if (pasteValue != NULL) {
						e = EvntManager->CreateTextEvent(pasteValue);
						EvntManager->DispatchEvent(std::move(e));
						SDL_free(pasteValue);
					}
				}
				// we do not want these events to cascade down to SDL_KEYDOWN, so we return here instead of at default .
				return SDLVideoDriver::ProcessEvent(event);
			}
		case SDL_KEYDOWN:
			if (SDL_GetModState() & KMOD_CTRL) {
				switch (event.key.keysym.sym) {
					case SDLK_v:
						if (SDL_HasClipboardText()) {
							char* text = SDL_GetClipboardText();
							e = EvntManager->CreateTextEvent(text);
							SDL_free(text);
							EvntManager->DispatchEvent(std::move(e));
							return GEM_OK;
						}
						break;
					default:
						break;
				}
			}
			return SDLVideoDriver::ProcessEvent(event);
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

void SDL20VideoDriver::CaptureMouse(bool enabled)
{
	SDL_CaptureMouse(SDL_bool(enabled));
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB51, "SDL2 Video Driver")
PLUGIN_DRIVER(SDL20VideoDriver, "sdl")
END_PLUGIN()
