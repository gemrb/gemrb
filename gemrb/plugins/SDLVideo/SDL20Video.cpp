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

#include "Audio.h"
#include "Interface.h"

using namespace GemRB;

#ifdef BAKE_ICON
#include "gemrb-icon.h"

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
#endif

SDL20VideoDriver::SDL20VideoDriver() noexcept
{
	renderer = NULL;
	window = NULL;

	stencilAlphaBlender = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE,
													 SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO,
													 SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
	
	oneMinusDstBlender = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR, SDL_BLENDFACTOR_ONE,
													SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
													SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
	
	dstBlender = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE,
											SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_DST_COLOR,
											SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
	
	srcBlender = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDFACTOR_ONE,
											SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_SRC_COLOR,
											SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);

	// WARNING: do _not_ call opengl here
	// all function pointers will be NULL
	// until after SDL_CreateRenderer is called

	SDL_version ver;
	SDL_GetVersion(&ver);
	sdl2_runtime_version = SDL_VERSIONNUM(ver.major, ver.minor, ver.patch);
}

SDL20VideoDriver::~SDL20VideoDriver() noexcept
{
	if (SDL_GameControllerGetAttached(gameController)) {
		SDL_GameControllerClose(gameController);
	}

	// we must release all buffers before SDL_DestroyRenderer
	// we cant rely on the base destructor here
	scratchBuffer = nullptr;
	DestroyBuffers();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

#if USE_OPENGL_BACKEND
	delete blitRGBAShader;
#endif
}

int SDL20VideoDriver::Init()
{
	int ret = SDLVideoDriver::Init();

#ifdef USE_SDL_CONTROLLER_API
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1) {
		Log(ERROR, "SDL2", "InitSubSystem failed: {}", SDL_GetError());
		return ret;
	}

	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			gameController = SDL_GameControllerOpen(i);
			if (gameController != nullptr) {
				break;
			}
		}
	}
#endif

	return ret;
}

int SDL20VideoDriver::CreateSDLDisplay(const char* title, bool vsync)
{
	Log(MESSAGE, "SDL 2 Driver", "Creating display");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

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


#ifdef SDL_RESOLUTION_INDEPENDANCE
	SDL_DisplayMode current;

	int should_be_zero = SDL_GetCurrentDisplayMode(0, &current);

	if (should_be_zero != 0) {
		Log(ERROR, "SDL 2 Driver", "couldnt get resolution: {}", SDL_GetError());
		return GEM_ERROR;
	}

	Log(DEBUG, "SDL 2 Driver", "Game Resolution: {}x{}, Screen Resolution: {}x{}",
		screenSize.w, screenSize.h, current.w, current.h);
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, current.w, current.h, winFlags);
#else

	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenSize.w, screenSize.h, winFlags);
#endif

	if (window == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create window: {}", SDL_GetError());
		return GEM_ERROR;
	}

#ifdef BAKE_ICON
	SetWindowIcon(window);
#endif

	int rendererFlags = SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_ACCELERATED;
	if (vsync) {
		rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
	}

	renderer = SDL_CreateRenderer(window, -1, rendererFlags);

	// Moved this here to fix a weird rendering issue

	// we set logical size so that platforms where the window can be a different size then requested
	// function properly. eg iPhone and Android the requested size may be 640x480,
	// but the window will always be the size of the screen
	SDL_RenderSetLogicalSize(renderer, screenSize.w, screenSize.h);
	//SDL_GetRendererOutputSize(renderer, &screenSize.w, &screenSize.h);

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	Log(DEBUG, "SDL20Video", "Renderer: {}", info.name);

	if (renderer == NULL) {
		Log(ERROR, "SDL 2 Driver", "couldnt create renderer: {}", SDL_GetError());
		return GEM_ERROR;
	}

#if USE_OPENGL_BACKEND
	// glGetString can return null, fmt doesn't support const unsigned char* and std::string can handle neither
	std::string tmp[4] = { "/" };
	const int strings[4] = { GL_VERSION, GL_RENDERER, GL_VENDOR, GL_SHADING_LANGUAGE_VERSION };
	for (int i = 0; i < 4; i++) {
		auto glString = glGetString(strings[i]);
		if (glString) tmp[i] = reinterpret_cast<const char*>(glString);
	}
	Log(MESSAGE, "SDL 2 GL Driver", "OpenGL version: {}, renderer: {}, vendor: {}", tmp[0], tmp[1], tmp[2]);
	Log(MESSAGE, "SDL 2 GL Driver", "  GLSL version: {}", tmp[3]);

	if (strcmp(info.name, driverName) != 0) {
		Log(FATAL, "SDL 2 GL Driver", "OpenGL backend must be used instead of {}", info.name);
		return GEM_ERROR;
	}

#if defined(_WIN32) && defined(USE_OPENGL_API)
	glewInit();
#endif

	scratchBuffer = CreateBuffer(Region(Point(), screenSize), BufferFormat::DISPLAY_ALPHA);
	scratchBuffer->Clear();

	// Try to grab the texture shader program of SDL
	static const SDL_Rect r = {0, 0, 1, 1};
	SDL_RenderCopy(renderer, ScratchBuffer(), &r, &r);
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_RenderFlush(renderer);
#endif

	GLuint rgbaProgramID = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&rgbaProgramID));
	assert(rgbaProgramID > 0);

	// Now, replace this program's shader units
	this->blitRGBAShader =
		GLSLProgram::CreateFromFiles("Shaders/SDLTextureV.glsl", "Shaders/BlitRGBA.glsl", rgbaProgramID);
	if (!blitRGBAShader) {
		Log(ERROR, "SDL 2 GL Driver", "RGBA shader setup failed: {}", GLSLProgram::GetLastError());
		return GEM_ERROR;
	}
#endif

	if (SDL_GetNumVideoDisplays() > 1) {
		Log(WARNING, "SDL 2", "More than one display reported, possibly reporting wrong refresh rate.");
	}

	SDL_DisplayMode dm = {};
	int displayModeResult = SDL_GetCurrentDisplayMode(0, &dm);
	if (displayModeResult != 0 || dm.refresh_rate == 0) {
		Log(WARNING, "SDL 2", "Unable to fetch refresh rate.");
		return 0;
	}

	refreshRate = dm.refresh_rate;

	SDL_StopTextInput(); // for some reason this is enabled from start

	return GEM_OK;
}

VideoBuffer* SDL20VideoDriver::NewVideoBuffer(const Region& r, BufferFormat fmt)
{
	Uint32 format = SDLPixelFormatFromBufferFormat(fmt, renderer);
	if (format == SDL_PIXELFORMAT_UNKNOWN)
		return nullptr;
	
	SDL_Texture* tex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, r.w, r.h);
	if (tex == nullptr) {
		Log(ERROR, "SDL 2", "{}", SDL_GetError());
		return nullptr;
	}
	return new SDLTextureVideoBuffer(r.origin, tex, fmt, renderer);
}

void SDL20VideoDriver::SwapBuffers(VideoBuffers& buffers)
{
#if USE_OPENGL_BACKEND
	// we have coopted SDLs shader, so we need to reset uniforms to values appropriate for the render targets
	blitRGBAShader->SetUniformValue("u_greyMode", 1, 0);
	blitRGBAShader->SetUniformValue("u_stencil", 1, 0);
	blitRGBAShader->SetUniformValue("u_dither", 1, 0);
	blitRGBAShader->SetUniformValue("u_rgba", 1, 1);
#endif

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

int SDL20VideoDriver::UpdateRenderTarget(const Color* color, BlitFlags flags)
{
	// TODO: add support for BlitFlags::HALFTRANS, BlitFlags::COLOR_MOD, and others (no use for them ATM)

	SDL_Texture* target = CurrentRenderBuffer();

	assert(target);
	int ret = SDL_SetRenderTarget(renderer, target);
	if (ret != 0) {
		Log(ERROR, "SDLVideo", "{}", SDL_GetError());
		return ret;
	}

	if (screenClip.size == screenSize)
	{
		// Some SDL backends complain on having a clip rect of the entire renderer size
		// I'm not sure if it is an SDL bug; possibly its just 0 based so it is out of bounds?
		SDL_RenderSetClipRect(renderer, NULL);
	} else {
		SDL_RenderSetClipRect(renderer, reinterpret_cast<SDL_Rect*>(&screenClip));
	}

	if (color) {
		if (flags & BlitFlags::BLENDED) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		} else if (flags & BlitFlags::MOD) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);
		} else {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		}

		return SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
	}

	return 0;
}

void SDL20VideoDriver::BlitSpriteNativeClipped(const SDLTextureSprite2D* spr, const Region& src, const Region& dst, BlitFlags flags, const SDL_Color* tint)
{
	BlitFlags version = BlitFlags::NONE;
#if !USE_OPENGL_BACKEND
	// we need to isolate flags that require software rendering to use as the "version"
	version = (BlitFlags::GREY | BlitFlags::SEPIA) & flags;
#endif
	// WARNING: software fallback == slow
	if (spr->Format().Bpp == 1 && (flags & BlitFlags::ALPHA_MOD)) {
		version |= BlitFlags::ALPHA_MOD;
		flags &= ~spr->RenderWithFlags(version, reinterpret_cast<const Color*>(tint));
	} else {
		flags &= ~spr->RenderWithFlags(version);
	}

	SDL_Texture* tex = spr->GetTexture(renderer);
	BlitSpriteNativeClipped(tex, src, dst, flags, tint);
}

void SDL20VideoDriver::BlitSpriteNativeClipped(SDL_Texture* texSprite, const Region& srgn, const Region& drgn, BlitFlags flags, const SDL_Color* tint)
{
	SDL_Rect srect = RectFromRegion(srgn);
	SDL_Rect drect = RectFromRegion(drgn);
	
	int ret = 0;
#if USE_OPENGL_BACKEND
	UpdateRenderTarget();
	ret = RenderCopyShaded(texSprite, &srect, &drect, flags, tint);
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_RenderFlush(renderer);
#endif
#else
	if (flags & BlitFlags::STENCIL_MASK) {
		// 1. clear scratchpad segment
		// 2. blend stencil segment to scratchpad
		// 3. blend texture to scratchpad
		// 4. copy scratchpad segment to screen

		std::static_pointer_cast<SDLTextureVideoBuffer>(scratchBuffer)->Clear(drect); // sets the render target to the scratch buffer

		SDL_Texture* stencilTex = CurrentStencilBuffer();
		SDL_SetTextureBlendMode(stencilTex, stencilAlphaBlender);
		RenderCopyShaded(texSprite, &srect, &drect, flags & ~(BlitFlags::ALPHA_MOD|BlitFlags::HALFTRANS), tint);
		// alpha masking only
		SDL_Rect stencilRect = drect;
		stencilRect.x -= stencilBuffer->Origin().x;
		stencilRect.y -= stencilBuffer->Origin().y;
		SDL_RenderCopy(renderer, stencilTex, &stencilRect, &drect);

		if (flags & (BlitFlags::ALPHA_MOD | BlitFlags::HALFTRANS)) {
			Uint8 alpha = SDL_ALPHA_OPAQUE;
			if (flags & BlitFlags::ALPHA_MOD) {
				alpha = tint->a;
			}
			
			if (flags & BlitFlags::HALFTRANS) {
				alpha /= 2;
			}
			SDL_SetTextureAlphaMod(ScratchBuffer(), alpha);
		}
		SDL_SetRenderTarget(renderer, CurrentRenderBuffer());
		SDL_SetTextureBlendMode(ScratchBuffer(), SDL_BLENDMODE_BLEND);
		ret = SDL_RenderCopy(renderer, ScratchBuffer(), &drect, &drect);
	} else {
		UpdateRenderTarget();
		ret = RenderCopyShaded(texSprite, &srect, &drect, flags, tint);
	}
#endif

	if (ret != 0) {
		Log(ERROR, "SDLVideo", "{}", SDL_GetError());
	}
}

void SDL20VideoDriver::BlitVideoBuffer(const VideoBufferPtr& buf, const Point& p, BlitFlags flags, Color tint)
{
	auto tex = static_cast<SDLTextureVideoBuffer&>(*buf).GetTexture();
	const Region& r = buf->Rect();
	Point origin = r.origin + p;

	const Region& srect = {0, 0, r.w, r.h};
	const Region& drect = {origin, r.size};
	BlitSpriteNativeClipped(tex, srect, drect, flags, reinterpret_cast<const SDL_Color*>(&tint));
}

int SDL20VideoDriver::RenderCopyShaded(SDL_Texture* texture, const SDL_Rect* srcrect,
									   const SDL_Rect* dstrect, BlitFlags flags, const SDL_Color* tint)
{
#if USE_OPENGL_BACKEND
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_RenderFlush(renderer);
#endif

	uint32_t format = 0;
	SDL_QueryTexture(texture, &format, nullptr, nullptr, nullptr);
	blitRGBAShader->Use();
	
	blitRGBAShader->SetUniformValue("s_sprite", 1, 0);
	blitRGBAShader->SetUniformValue("s_stencil", 1, 1);
	
	bool isRGBA = SDL_ISPIXELFORMAT_ALPHA(format);
	blitRGBAShader->SetUniformValue("u_rgba", 1, isRGBA ? 1 : 0);

	GLint greyMode = 0;
	if (flags & BlitFlags::GREY) {
		greyMode = 1;
	} else if (flags & BlitFlags::SEPIA) {
		greyMode = 2;
	}

	blitRGBAShader->SetUniformValue("u_greyMode", 1, greyMode);

	GLint channel = 3;
	if (flags & BlitFlags::STENCIL_RED) {
		channel = 0;
	} else if (flags & BlitFlags::STENCIL_GREEN) {
		channel = 1;
	} else if (flags & BlitFlags::STENCIL_BLUE) {
		channel = 2;
	}

	blitRGBAShader->SetUniformValue("u_channel", 1, channel);

	bool doStencil = flags & BlitFlags::STENCIL_MASK;
	blitRGBAShader->SetUniformValue("u_stencil", 1, doStencil ? 1 : 0);

	if (doStencil) {
		assert(stencilBuffer && dstrect);

		bool doDither = flags & BlitFlags::STENCIL_DITHER;
		blitRGBAShader->SetUniformValue("u_dither", 1, doDither ? 1 : 0);

		int texW = 0;
		int texH = 0;
		SDL_QueryTexture(CurrentStencilBuffer(), nullptr, nullptr, &texW, &texH);

		GLfloat stencilTexW = 1.0f;
		GLfloat stencilTexH = 1.0f;
		GLfloat stencilTexX = 0.0f;
		GLfloat stencilTexY = 0.0f;

		float scaleX = 0.0f;
		float scaleY = 0.0f;
		SDL_RenderGetScale(renderer, &scaleX, &scaleY);
		
		SDL_Rect stencilRect = *dstrect;
		stencilRect.x -= stencilBuffer->Origin().x;
		stencilRect.y -= stencilBuffer->Origin().y;

		if (stencilRect.x < dstrect->x && stencilRect.y < dstrect->y) {
			stencilTexX = -static_cast<GLfloat>(dstrect->x);
			stencilTexY = -static_cast<GLfloat>(dstrect->y);
		}

#if !SDL_VERSION_ATLEAST(2, 0, 18)
		// In versions earlier, SDL uses a different vertex setup in case
		// of flipping: (-w/2, -h/2) to (w/2, h/2) that are transformed by
		// the OpenGL backend via matrices.
		if (flags & BlitFlags::MIRRORX) {
			stencilTexX = dstrect->w / 2.0f + stencilRect.x;
			stencilTexY = dstrect->h / 2.0f + stencilRect.y;
		}
#endif

		stencilTexW = 1.0f / (static_cast<float>(texW) * scaleX);
		stencilTexH = 1.0f / (static_cast<float>(texH) * scaleY);

		GLfloat mat[3][3] = {
			{ stencilTexW,        0.0f, 0.0f },
			{        0.0f, stencilTexH, 0.0f },
			{ stencilTexX * stencilTexW, stencilTexY * stencilTexH, 1.0f }
		};

		blitRGBAShader->SetUniformMatrixValue("u_stencilMat", 3, 1, reinterpret_cast<GLfloat*>(&mat));
		
		// Ask OpenGL about the texture handle (that lies hidden in SDL_Texture otherwise)
		auto curTexture = std::static_pointer_cast<SDLTextureVideoBuffer>(stencilBuffer)->GetTexture();
		SDL_GL_BindTexture(curTexture, nullptr, nullptr);
		GLuint stencilTextureID;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&stencilTextureID));
		SDL_GL_UnbindTexture(curTexture);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, stencilTextureID);
	}
#endif
	
	Uint8 alpha = SDL_ALPHA_OPAQUE;
	if (flags & BlitFlags::ALPHA_MOD) {
		alpha = tint->a;
	}
	
	if (flags & BlitFlags::HALFTRANS) {
		alpha /= 2;
	}
	
	SDL_SetTextureAlphaMod(texture, alpha);

	if (flags & BlitFlags::COLOR_MOD) {
		SDL_SetTextureColorMod(texture, tint->r, tint->g, tint->b);
	} else {
		SDL_SetTextureColorMod(texture, 0xff, 0xff, 0xff);
	}
	
	if (flags & BlitFlags::ADD) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
	} else if (flags & BlitFlags::MOD) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
	} else if (flags & BlitFlags::MUL) {
#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MUL);
#endif
	} else if (flags & BlitFlags::SRC) {
		SDL_SetTextureBlendMode(texture, srcBlender);
	} else if (flags & BlitFlags::ONE_MINUS_DST) {
		SDL_SetTextureBlendMode(texture, oneMinusDstBlender);
	} else if (flags & BlitFlags::DST) {
		SDL_SetTextureBlendMode(texture, dstBlender);
	} else if (flags & (BlitFlags::BLENDED | BlitFlags::HALFTRANS)) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	} else {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	}

	SDL_RendererFlip flipflags = (flags & BlitFlags::MIRRORY) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
	flipflags = static_cast<SDL_RendererFlip>(flipflags | ((flags & BlitFlags::MIRRORX) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));

	return SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, 0.0, nullptr, flipflags);
}

void SDL20VideoDriver::DrawRawGeometry(
	const std::vector<float>& vertices,
	const std::vector<Color>& colors,
	BlitFlags blitFlags
) {
#if SDL_VERSION_ATLEAST(2, 0, 18)
	if (blitFlags & BlitFlags::BLENDED) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	}

	SDL_RenderGeometryRaw(
		renderer,
		nullptr,
		vertices.data(),
		2 * sizeof(float),
		#if SDL_VERSION_ATLEAST(2, 0, 22)
		reinterpret_cast<const SDL_Color*>(colors.data()),
		#else
		reinterpret_cast<const int*>(colors.data()),
		#endif
		sizeof(Color),
		nullptr,
		0,
		vertices.size() / 2,
		nullptr,
		0,
		0
	);
#else
	(void)vertices;
	(void)colors;
	(void)blitFlags;
#endif
}

void SDL20VideoDriver::DrawPointsImp(const std::vector<Point>& points, const Color& color, BlitFlags flags)
{
	DrawSDLPoints(reinterpret_cast<const std::vector<SDL_Point>&>(points), reinterpret_cast<const SDL_Color&>(color), flags);
}

void SDL20VideoDriver::DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, BlitFlags flags)
{
	if (points.empty()) {
		return;
	}
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color), flags);
	SDL_RenderDrawPoints(renderer, &points[0], int(points.size()));
}

void SDL20VideoDriver::DrawPointImp(const Point& p, const Color& color, BlitFlags flags)
{
	UpdateRenderTarget(&color, flags);
	SDL_RenderDrawPoint(renderer, p.x, p.y);
}

void SDL20VideoDriver::DrawLinesImp(const std::vector<Point>& points, const Color& color, BlitFlags flags)
{
	DrawSDLLines(reinterpret_cast<const std::vector<SDL_Point>&>(points), reinterpret_cast<const SDL_Color&>(color), flags);
}

void SDL20VideoDriver::DrawSDLLines(const std::vector<SDL_Point>& points, const SDL_Color& color, BlitFlags flags)
{
	UpdateRenderTarget(reinterpret_cast<const Color*>(&color), flags);
	SDL_RenderDrawLines(renderer, &points[0], int(points.size()));
}

void SDL20VideoDriver::DrawLineImp(const Point& p1, const Point& p2, const Color& color, BlitFlags flags)
{
	UpdateRenderTarget(&color, flags);
	SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
}

void SDL20VideoDriver::DrawRectImp(const Region& rgn, const Color& color, bool fill, BlitFlags flags)
{
	UpdateRenderTarget(&color, flags);
	if (fill) {
		SDL_RenderFillRect(renderer, reinterpret_cast<const SDL_Rect*>(&rgn));
	} else {
		SDL_RenderDrawRect(renderer, reinterpret_cast<const SDL_Rect*>(&rgn));
	}
}

void SDL20VideoDriver::DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, BlitFlags flags)
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
			const Point& p = poly->vertices[i] - poly->BBox.origin + origin;
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

	static const PixelFormat fmt(3, 0x00ff0000, 0x0000ff00, 0x000000ff, 0);
	SDLTextureSprite2D* screenshot = new SDLTextureSprite2D(Region(0,0, Width, Height), fmt);

	SDL_Texture* target = SDL_GetRenderTarget(renderer);
	if (buf) {
		auto texture = static_cast<SDLTextureVideoBuffer*>(buf.get())->GetTexture();
		SDL_SetRenderTarget(renderer, texture);
	} else {
		SDL_SetRenderTarget(renderer, nullptr);
	}

	SDL_Surface* surface = screenshot->GetSurface();
	SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_BGR24, surface->pixels, surface->pitch);

	SDL_SetRenderTarget(renderer, target);

	return Holder<Sprite2D>(screenshot);
}

int SDL20VideoDriver::GetTouchFingers(TouchEvent::Finger(&fingers)[FINGER_MAX], SDL_TouchID device) const
{
	int numf = SDL_GetNumTouchFingers(device);

	for (int i = 0; i < numf; ++i) {
		const SDL_Finger* finger = SDL_GetTouchFinger(device, i);
		assert(finger);

		fingers[i].id = finger->id;
		fingers[i].x = finger->x * screenSize.w;
		fingers[i].y = finger->y * screenSize.h;

		const TouchEvent::Finger* current = EventMgr::FingerState(finger->id);
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
#ifdef USE_SDL_CONTROLLER_API
		case SDL_CONTROLLERDEVICEREMOVED:
			if (gameController != nullptr) {
				const SDL_GameController *removedController = SDL_GameControllerFromInstanceID(event.jdevice.which);
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
				int delta = xaxis ? pct * screenSize.w : pct * screenSize.h;
				InputAxis axis = InputAxis(event.caxis.axis);
				e = EventMgr::CreateControllerAxisEvent(axis, delta, pct);
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			{
				bool down = (event.type == SDL_JOYBUTTONDOWN) ? true : false;
				EventButton btn = EventButton(event.cbutton.button);
				e = EventMgr::CreateControllerButtonEvent(btn, down);
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
#endif
		case SDL_FINGERDOWN: // fallthrough
		case SDL_FINGERUP:
			{
				TouchEvent::Finger fingers[1] = { };
				fingers[0].x = event.tfinger.x * screenSize.w;
				fingers[0].y = event.tfinger.y * screenSize.h;
				fingers[0].deltaX = event.tfinger.dx * screenSize.w;
				fingers[0].deltaY = event.tfinger.dy * screenSize.h;
				fingers[0].id = event.tfinger.fingerId;

				e = EventMgr::CreateTouchEvent(fingers, 1, event.type == SDL_FINGERDOWN, event.tfinger.pressure);
				e.mod = modstate;
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		// For swipes only. Gestures requiring pinch or rotate need to use SDL_MULTIGESTURE or SDL_DOLLARGESTURE
		case SDL_FINGERMOTION:
			{
				TouchEvent::Finger fingers[FINGER_MAX] = { }; // 0 init
				int numf = GetTouchFingers(fingers, event.mgesture.touchId);

				Event touch = EventMgr::CreateTouchEvent(fingers, numf, true, event.tfinger.pressure);
				// TODO: it may make more sense to calculate a pinch/rotation from screen center?
				e = EventMgr::CreateTouchGesture(touch.touch, 0.0, 0.0);
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
				Event touch = EventMgr::CreateTouchEvent(fingers, numf, true, 0.0);
				e = EventMgr::CreateTouchGesture(touch.touch, event.mgesture.dTheta, event.mgesture.dDist);
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
				// hopefully no devices will be merging several repeated wheel clicks together
				static bool unitIsPixels = false;
				if (event.wheel.y > 1 || event.wheel.x > 1) {
					unitIsPixels = true;
				}
				
				int speed = unitIsPixels ? 1 : core->GetMouseScrollSpeed();
				if (SDL_GetModState() & KMOD_SHIFT) {
					e = EventMgr::CreateMouseWheelEvent(Point(event.wheel.y * speed, event.wheel.x * speed));
				} else {
					e = EventMgr::CreateMouseWheelEvent(Point(event.wheel.x * speed, event.wheel.y * speed));
				}
				
				EvntManager->DispatchEvent(std::move(e));
			}
			break;
		/* not user input events */
		case SDL_TEXTINPUT:
			e = EventMgr::CreateTextEvent(event.text.text);
			EvntManager->DispatchEvent(std::move(e));
			break;
		/* not user input events */

		// TODO: these events will be sent by the D3D renderer and we will need to handle them
		case SDL_RENDER_DEVICE_RESET:
			// TODO: must destroy all SDLTextureSprite2D textures

			// fallthrough
		case SDL_APP_DIDENTERFOREGROUND:
		case SDL_RENDER_TARGETS_RESET:
			e = EventMgr::CreateRedrawRequestEvent();
			EvntManager->DispatchEvent(std::move(e));
			break;
		case SDL_WINDOWEVENT://SDL 1.2
			switch (event.window.event) {
				case SDL_WINDOWEVENT_LEAVE:
					if (core->config.GUIEnhancements & 8) core->DisableGameControl(true);
					break;
				case SDL_WINDOWEVENT_ENTER:
					if (core->config.GUIEnhancements & 8) core->DisableGameControl(false);
					break;
				case SDL_WINDOWEVENT_MINIMIZED://SDL 1.3
					// We pause the game and audio when the window is minimized.
					// on iOS/Android this happens when leaving the application or when play is interrupted (ex phone call)
					// but it's annoying on desktops, so we try to detect them
					if (TouchInputEnabled()) {
						core->GetAudioDrv()->Pause();//this is for ANDROID mostly
						core->SetPause(PauseState::On);
					}
					break;
				case SDL_WINDOWEVENT_RESTORED: //SDL 1.3
					core->GetAudioDrv()->Resume();//this is for ANDROID mostly
					break;
				// SDL_WINDOWEVENT_RESIZED and SDL_WINDOWEVENT_SIZE_CHANGED are handled automatically
				default:
					break;
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
						e = EventMgr::CreateTextEvent(pasteValue);
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
							e = EventMgr::CreateTextEvent(text);
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

bool SDL20VideoDriver::CanDrawRawGeometry() const {
#if SDL_VERSION_ATLEAST(2, 0, 18)
	return true;
#else
	return false;
#endif
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
