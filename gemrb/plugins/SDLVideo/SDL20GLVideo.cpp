#ifdef USE_GL
#include <GL/glew.h>
#ifdef _MSC_VER
	#pragma comment(lib, "glew32")
	#pragma comment(lib, "opengl32")
#endif
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#ifdef _MSC_VER
	#pragma comment(lib, "libGLESv2")
#endif
#endif
#include <algorithm>
#include "SDL20GLVideo.h"
#include "Interface.h"
#include "Game.h" // for GetGlobalTint
#include "GLTextureSprite2D.h"
#include "GLPaletteManager.h"
#include "GLSLProgram.h"
#include "Matrix.h"

using namespace GemRB;

GLVideoDriver::~GLVideoDriver()
{
	if (program32) program32->Release();
	if (programPal) programPal->Release();
	if (programPalGrayed) programPalGrayed->Release();
	if (programPalSepia) programPalSepia->Release();
	if (programRect) programRect->Release();
	if (programEllipse) programEllipse->Release();
	delete paletteManager;
	SDL_GL_DeleteContext(context);
}

int GLVideoDriver::CreateDisplay(int w, int h, int bpp, bool fs, const char* title)
{
	fullscreen=fs;
	width = w, height = h;

	Log(MESSAGE, "SDL 2 GL Driver", "Creating display");
	Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
#if TARGET_OS_IPHONE || ANDROID
	// this allows the user to flip the device upsidedown if they wish and have the game rotate.
	// it also for some unknown reason is required for retina displays
	winFlags |= SDL_WINDOW_RESIZABLE;
	// this hint is set in the wrapper for iPad at a higher priority. set it here for iPhone
	// don't know if Android makes use of this.
	SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight LandscapeLeft", SDL_HINT_DEFAULT);
#endif
	if (fullscreen) 
	{
		winFlags |= SDL_WINDOW_FULLSCREEN;
		//This is needed to remove the status bar on Android/iOS.
		//since we are in fullscreen this has no effect outside Android/iOS
		winFlags |= SDL_WINDOW_BORDERLESS;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifndef USE_GL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, winFlags);
	if (window == NULL) 
	{
		Log(ERROR, "SDL 2 GL Driver", "couldnt create window:%s", SDL_GetError());
		return GEM_ERROR;
	}

	context = SDL_GL_CreateContext(window);
	if (context == NULL) 
	{
		Log(ERROR, "SDL 2 GL Driver", "couldnt create GL context:%s", SDL_GetError());
		return GEM_ERROR;
	}
	SDL_GL_MakeCurrent(window, context);

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL) 
	{
		Log(ERROR, "SDL 2 GL Driver", "couldnt create renderer:%s", SDL_GetError());
		return GEM_ERROR;
	}
	SDL_RenderSetLogicalSize(renderer, width, height);

	Viewport.w = width;
	Viewport.h = height;

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);

	Uint32 format = SDL_PIXELFORMAT_RGBA8888;
	screenTexture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);

	int access;

	SDL_QueryTexture(screenTexture,
                     &format,
                     &access,
                     &width,
                     &height);

	Uint32 r, g, b, a;
	SDL_PixelFormatEnumToMasks(format, &bpp, &r, &g, &b, &a);
	a = 0; //force a to 0 or screenshots will be all black!

	Log(MESSAGE, "SDL 2 GL Driver", "Creating Main Surface: w=%d h=%d fmt=%s",
		width, height, SDL_GetPixelFormatName(format));
	backBuf = SDL_CreateRGBSurface( 0, width, height, bpp, r, g, b, a );
	this->bpp = bpp;

	if (!backBuf) 
	{
		Log(ERROR, "SDL 2 GL Video", "Unable to create backbuffer of %s format: %s",
			SDL_GetPixelFormatName(format), SDL_GetError());
		return GEM_ERROR;
	}
	disp = backBuf;

#ifdef USE_GL
	glewInit();
#endif
	if (!createPrograms()) return GEM_ERROR;
	paletteManager = new GLPaletteManager();
	glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
	spritesPerFrame = 0;
	return GEM_OK;
}

void GLVideoDriver::useProgram(GLSLProgram* program)
{
	if (lastUsedProgram == program) return;
	program->Use();
	lastUsedProgram = program;
}

bool GLVideoDriver::createPrograms()
{
	std::string msg;
	float matrix[16];
	Matrix::SetIdentityM(matrix);

	program32 = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/Sprite32.glslf");
	if (!program32)
	{
		msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return false;
	}
	program32->Use();
	program32->SetUniformValue("s_texture", 1, 0);
	program32->SetUniformMatrixValue("u_matrix", 4, 1, matrix);
	
	programPal = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpritePal.glslf");
	if (!programPal)
	{
		msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program :%s", msg.c_str());
		return false;
	}
	programPal->Use();
	programPal->SetUniformValue("s_texture", 1, 0);
	programPal->SetUniformValue("s_palette", 1, 1);
	programPal->SetUniformValue("s_mask", 1, 2);
	programPal->SetUniformMatrixValue("u_matrix", 4, 1, matrix);

	programPalGrayed = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpritePalGrayed.glslf");
	if (!programPalGrayed)
	{
		msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return false;
	}
	programPalGrayed->Use();
	programPal->SetUniformValue("s_texture", 1, 0);
	programPal->SetUniformValue("s_palette", 1, 1);
	programPal->SetUniformValue("s_mask", 1, 2);
	programPalGrayed->SetUniformMatrixValue("u_matrix", 4, 1, matrix);

	programPalSepia = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpritePalSepia.glslf");
	if (!programPalSepia)
	{
		msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return false;
	}
	programPalSepia->Use();
	programPal->SetUniformValue("s_texture", 1, 0);
	programPal->SetUniformValue("s_palette", 1, 1);
	programPal->SetUniformValue("s_mask", 1, 2);;
	programPalSepia->SetUniformMatrixValue("u_matrix", 4, 1, matrix);
	
	programEllipse = GLSLProgram::CreateFromFiles("Shaders/Ellipse.glslv", "Shaders/Ellipse.glslf");
	if (!programEllipse)
	{
		msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return false;
	}
	programEllipse->Use();
	programEllipse->SetUniformMatrixValue("u_matrix", 4, 1, matrix);

	programRect = GLSLProgram::CreateFromFiles("Shaders/Rect.glslv", "Shaders/Rect.glslf");
	if (!programRect)
	{
		msg = GLSLProgram::GetLastError();
		Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", msg.c_str());
		return false;
	}
	programRect->Use();
	programRect->SetUniformMatrixValue("u_matrix", 4, 1, matrix);
	
	lastUsedProgram = NULL;
	return true;
}

Sprite2D* GLVideoDriver::CreateSprite(int w, int h, int bpp, ieDword rMask, ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	GLTextureSprite2D* spr = new GLTextureSprite2D(w, h, bpp, pixels, rMask, gMask, bMask, aMask);
	if (cK) spr->SetColorKey(index);
	return spr;
}

Sprite2D* GLVideoDriver::CreatePalettedSprite(int w, int h, int bpp, void* pixels, Color* palette, bool cK, int index)
{
	GLTextureSprite2D* spr = new GLTextureSprite2D(w, h, bpp, pixels);
	spr->SetPaletteManager(paletteManager);
	Palette* pal = new Palette(palette);
	spr->SetPalette(pal);
	pal->release();
	if (cK) spr->SetColorKey(index);
	return spr;
}

Sprite2D* GLVideoDriver::CreateSprite8(int w, int h, void* pixels, Palette* palette, bool cK, int index)
{
	return CreatePalettedSprite(w, h, 8, pixels, palette->col, cK, index);
}

void GLVideoDriver::GLBlitSprite(GLTextureSprite2D* spr, const Region& src, const Region& dst, Palette* attachedPal,
								 unsigned int flags, const Color* tint, GLTextureSprite2D* mask)
{
	// TODO: clip dst to the screen?
	if (dst.w <= 0 || dst.h <= 0 || src.w <= 0 || src.h <= 0)
		return; // we already know blit fails

	glViewport(dst.x, height - (dst.y + dst.h), dst.w, dst.h);
	glScissor(dst.x, height - (dst.y + dst.h), dst.w, dst.h);
	float hscale = 2.0f/(float)dst.w;
	float vscale = 2.0f/(float)dst.h;

	// color tint
	Color colorTint;
	if (tint)
		colorTint = *tint;
	else
		colorTint.r = colorTint.b = colorTint.g = colorTint.a = 255;

	// FIXME: how should we combine flags? previously we would cancel sprite flags with the flags param.
	// I think this way makes more sense, but I need to examine the behavior of the the functions passing flag parameters
	flags |= spr->renderFlags;

	GLfloat x = (GLfloat)src.x/(GLfloat)spr->Width;
	GLfloat y = (GLfloat)src.y/(GLfloat)spr->Height;
	GLfloat w = (GLfloat)src.w/(GLfloat)spr->Width;
	GLfloat h = (GLfloat)src.h/(GLfloat)spr->Height;
	GLfloat textureCoords[] = { /* lower left */ x, y, /* lower right */x + w, y,
								/* top left */ x, y + h, /* top right */ x + w, y + h };

	// FIXME: are there constants for accessing these coordinate indices?
	GLfloat tmp;
	if (flags&BLIT_MIRRORX) {
		// swap lower left X with lower right X
		tmp = textureCoords[0];
		textureCoords[0] = textureCoords[2];
		textureCoords[2] = tmp;
		// swap top left X with top right X
		tmp = textureCoords[4];
		textureCoords[4] = textureCoords[6];
		textureCoords[6] = tmp;
	}
	if (flags&BLIT_MIRRORY) {
		// swap lower left Y with top left Y
		tmp = textureCoords[1];
		textureCoords[1] = textureCoords[5];
		textureCoords[5] = tmp;
		// swap lower right Y with top right Y
		tmp = textureCoords[3];
		textureCoords[3] = textureCoords[7];
		textureCoords[7] = tmp;
	}

	// alpha modifier
	GLfloat alphaModifier = flags & BLIT_HALFTRANS ? 0.5f : 1.0f;

	// data
	GLfloat data[] = 
	{	    
		-1.0f, 1.0f, textureCoords[0], textureCoords[1],
		-1.0f + dst.w*hscale, 1.0f, textureCoords[2], textureCoords[3],
		-1.0f, 1.0f - dst.h*vscale, textureCoords[4], textureCoords[5],
		-1.0f + dst.w*hscale, 1.0f - dst.h*vscale, textureCoords[6], textureCoords[7]
	};

	// shader program selection
	GLSLProgram* program;
	GLuint palTexture;
	if(spr->IsPaletted())
	{
		if (flags & BLIT_GREY)
			program = programPalGrayed;
		else if (flags & BLIT_SEPIA)
			program = programPalSepia;
		else
			program = programPal;

		glActiveTexture(GL_TEXTURE1);
		if (attachedPal) 
			palTexture = paletteManager->CreatePaletteTexture(attachedPal, spr->GetColorKey(), true);
		else 
			palTexture = spr->GetPaletteTexture();		
		glBindTexture(GL_TEXTURE_2D, palTexture);
	}
	else
	{
		program = program32;
	}
	useProgram(program);

	glActiveTexture(GL_TEXTURE0);
	GLuint texture = spr->GetTexture();
	glBindTexture(GL_TEXTURE_2D, texture);
	
	if (mask)
	{
		glActiveTexture(GL_TEXTURE2);
		GLuint maskTexture = ((GLTextureSprite2D*)mask)->GetMaskTexture();
		glBindTexture(GL_TEXTURE_2D, maskTexture);
	}
	else
	if(flags & BLIT_EXTERNAL_MASK) {} // used with external mask
	else
	{
		// disable 3rd texture
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	program->SetUniformValue("u_tint", COLOR_SIZE, (GLfloat)colorTint.r/255, (GLfloat)colorTint.g/255, (GLfloat)colorTint.b/255, (GLfloat)colorTint.a/255);
	program->SetUniformValue("u_alphaModifier", 1, alphaModifier);

	GLint a_position = program->GetAttribLocation("a_position");
	GLint a_texCoord = program->GetAttribLocation("a_texCoord");

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

	glVertexAttribPointer(a_position, VERTEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*(VERTEX_SIZE + TEX_SIZE), 0);
	glVertexAttribPointer(a_texCoord, TEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*(VERTEX_SIZE + TEX_SIZE), BUFFER_OFFSET(sizeof(GLfloat)*VERTEX_SIZE));

	glEnableVertexAttribArray(a_position);
	glEnableVertexAttribArray(a_texCoord);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(a_texCoord);
	glDisableVertexAttribArray(a_position);
	
	glDeleteBuffers(1, &buffer);
	spritesPerFrame++;
}

void GLVideoDriver::BlitSprite(const Sprite2D* spr, const Region& src, const Region& dst, Palette* palette)
{
	GLBlitSprite((GLTextureSprite2D*)spr, src, dst, palette);
}

void GLVideoDriver::clearRect(const Region& rgn, const Color& color)
{
	if (SDL_ALPHA_TRANSPARENT == color.a) return;
	glScissor(rgn.x, height - rgn.y - rgn.h, rgn.w, rgn.h);
	glClearColor(color.r/255, color.g/255, color.b/255, color.a/255);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GLVideoDriver::drawPolygon(Point* points, unsigned int count, const Color& color, PointDrawingMode mode)
{
	if (SDL_ALPHA_TRANSPARENT == color.a) return;
	useProgram(programRect);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
	GLfloat* data = new GLfloat[count*VERTEX_SIZE];
	for(unsigned int i=0; i<count; i++)
	{
		data[i*VERTEX_SIZE] = -1.0f + (GLfloat)points[i].x*2/width;
		data[i*VERTEX_SIZE + 1] = 1.0f - (GLfloat)points[i].y*2/height;
	}

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*VERTEX_SIZE*count, data, GL_STATIC_DRAW);
	delete[] data;

	GLint a_position = programRect->GetAttribLocation("a_position");			
	glVertexAttribPointer(a_position, VERTEX_SIZE, GL_FLOAT, GL_FALSE, 0, 0);
	programRect->SetUniformValue("u_color", COLOR_SIZE, (GLfloat)color.r/255, (GLfloat)color.g/255, (GLfloat)color.b/255, (GLfloat)color.a/255);

	glEnableVertexAttribArray(a_position);
	if (mode == LineLoop)
		glDrawArrays(GL_LINE_LOOP, 0, count);
	else if (mode == LineStrip)
		glDrawArrays(GL_LINE_STRIP, 0, count);
	else if (mode == ConvexFilledPolygon)
		glDrawArrays(GL_TRIANGLE_FAN, 0, count);
	else if (mode == FilledTriangulation)
		glDrawArrays(GL_TRIANGLES, 0, count);
	glDisableVertexAttribArray(a_position);

	glDeleteBuffers(1, &buffer);
}

void GLVideoDriver::drawEllipse(int cx /*center*/, int cy /*center*/, unsigned short xr, unsigned short yr, float thickness, const Color& color)
{
	const float support = 0.75;
	useProgram(programEllipse);
    if (thickness < 1.0) thickness = 1.0;
    float dx = (int)ceilf(xr + thickness/2.0 + 2.5*support);
    float dy = (int)ceilf(yr + thickness/2.0 + 2.5*support);
	glViewport(cx - dx, height - cy - dy, dx*2, dy*2);
	GLfloat data[] = 
	{ 
		-1.0f, 1.0f, -1.0f, 1.0f,
		 1.0f, 1.0f,  1.0f, 1.0f,
		-1.0f,-1.0f, -1.0f,-1.0f,
		 1.0f,-1.0f,  1.0f,-1.0f
	};
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

	GLint a_position = programEllipse->GetAttribLocation("a_position");
	GLint a_texCoord = programEllipse->GetAttribLocation("a_texCoord");

	programEllipse->SetUniformValue("u_radiusX", 1, (GLfloat)xr/dx);
	programEllipse->SetUniformValue("u_radiusY", 1, (GLfloat)yr/dy);
	programEllipse->SetUniformValue("u_thickness", 1, (GLfloat)thickness/(dx + dy));
	programEllipse->SetUniformValue("u_support", 1, (GLfloat)support);
	programEllipse->SetUniformValue("u_color", 4, (GLfloat)color.r/255, (GLfloat)color.g/255, (GLfloat)color.b/255, (GLfloat)color.a/255);
			
	glVertexAttribPointer(a_position, VERTEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*(VERTEX_SIZE + TEX_SIZE), 0);
	glVertexAttribPointer(a_texCoord, TEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*(VERTEX_SIZE + TEX_SIZE), BUFFER_OFFSET(sizeof(GLfloat)*VERTEX_SIZE));
	
	glEnableVertexAttribArray(a_position);
	glEnableVertexAttribArray(a_texCoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(a_texCoord);
	glDisableVertexAttribArray(a_position);

	glDeleteBuffers(1, &buffer);
}

void GLVideoDriver::BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y, const Region* clip, unsigned int flags)
{
	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	tx -= Viewport.x;
	ty -= Viewport.y;
	unsigned int blitFlags = 0;
	if (flags & TILE_HALFTRANS) blitFlags |= BLIT_HALFTRANS;
	if (flags & TILE_GREY) blitFlags |= BLIT_GREY;
	if (flags & TILE_SEPIA) blitFlags |= BLIT_SEPIA;

	Region dst(tx, ty, spr->Width, spr->Height);
	if(clip)
	{
		dst = dst.Intersect(*clip);
	}

	const Color* totint = NULL;
	if (core->GetGame()) 
	{
		totint = core->GetGame()->GetGlobalTint();
	}
	return GLBlitSprite((GLTextureSprite2D*)spr, Region(0, 0, spr->Width, spr->Height), dst,
						NULL, blitFlags, totint, (GLTextureSprite2D*)mask);
}

void GLVideoDriver::BlitGameSprite(const Sprite2D* spr, int x, int y, unsigned int flags, Color tint,
								   SpriteCover* cover, Palette *palette, const Region* clip, bool anchor)
{
	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	if (!anchor) 
	{
		tx -= Viewport.x;
		ty -= Viewport.y;
	}
	GLTextureSprite2D* glSprite = (GLTextureSprite2D*)spr;
	GLuint coverTexture = 0;
	
	if (!anchor && core->GetGame()) 
	{
		const Color *totint = core->GetGame()->GetGlobalTint();
		if (totint) 
		{
			if (flags & BLIT_TINTED) 
			{
				tint.r = (tint.r * totint->r) >> 8;
				tint.g = (tint.g * totint->g) >> 8;
				tint.b = (tint.b * totint->b) >> 8;
			} 
			else
			{
				flags |= BLIT_TINTED;
				tint = *totint;
			}
		}
	}



	if(glSprite->IsPaletted())
	{
		if (cover)
		{
			int trueX = cover->XPos - glSprite->XPos;
			int trueY = cover->YPos - glSprite->YPos;
			Uint8* data = new Uint8[glSprite->Width*glSprite->Height];
			Uint8* coverPointer = &cover->pixels[trueY*glSprite->Width + trueX];
			Uint8* dataPointer = data;
			for(int h=0; h<glSprite->Height; h++)
			{
				for(int w=0; w<glSprite->Width; w++)
				{
					*dataPointer = !(*coverPointer) * 255;
					dataPointer++;
					coverPointer++;
				}
				coverPointer += cover->Width - glSprite->Width;
			}
			glActiveTexture(GL_TEXTURE2);
			glGenTextures(1, &coverTexture);
			glBindTexture(GL_TEXTURE_2D, coverTexture);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#ifdef USE_GL
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, glSprite->Width, glSprite->Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (GLvoid*) data);
			delete[] data;
			flags |= BLIT_EXTERNAL_MASK;
		}
	}

	Region src(0, 0, glSprite->Width, glSprite->Height);
	Region dst(tx, ty, glSprite->Width, glSprite->Height);
	if (clip) {
		dst = dst.Intersect(*clip);
	}
	if (tint.r == 0 && tint.g == 0 && tint.b == 0)
		GLBlitSprite(glSprite, src, dst, palette, flags);
	else
		GLBlitSprite(glSprite, src, dst, palette, flags, &tint);
	if (coverTexture != 0)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &coverTexture);
	}
}

void GLVideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill, bool clipped)
{
	if (fill && SDL_ALPHA_OPAQUE == color.a)
	{
		return clearRect(rgn, color); // possible to work faster than shader but a lot... may be disable in future
	}
	Point pt[] = { Point(rgn.x, rgn.y), Point(rgn.x + rgn.w, rgn.y), Point(rgn.x + rgn.w, rgn.y + rgn.h), Point(rgn.x, rgn.y + rgn.h) };
	if (clipped)
	{
		for(int i=0; i<4; i++)
		{
			pt[i].x += xCorr - Viewport.x;
			pt[i].y += yCorr - Viewport.y;
		}
	}
	if (fill)
		return drawPolygon(pt, 4, color, ConvexFilledPolygon);
	else
		return drawPolygon(pt, 4, color, LineLoop);
}

void GLVideoDriver::DrawHLine(short x1, short y, short x2, const Color& color, bool clipped)
{
	return DrawLine(x1, y, x2, y, color, clipped); 
}

void GLVideoDriver::DrawVLine(short x, short y1, short y2, const Color& color, bool clipped)
{
	return DrawLine(x, y1, x, y2, color, clipped); 
}

void GLVideoDriver::DrawLine(short x1, short y1, short x2, short y2, const Color& color, bool clipped)
{
	Point pt[] = { Point(x1, y1), Point(x2, y2) };
	if (clipped) 
	{
		pt[0].x += xCorr - Viewport.x;
		pt[1].x += xCorr - Viewport.x;
		pt[0].y += yCorr - Viewport.y;
		pt[1].y += yCorr - Viewport.y;
	}
	return drawPolygon(pt, 2, color, LineStrip);
}

void GLVideoDriver::DrawPolyline(Gem_Polygon* poly, const Color& color, bool fill)
{
	if (poly->count == 0) return;
	if (poly->BBox.x > Viewport.x + Viewport.w) return;
	if (poly->BBox.y > Viewport.y + Viewport.h) return;
	if (poly->BBox.x + poly->BBox.w < Viewport.x) return;
	if (poly->BBox.y + poly->BBox.h < Viewport.y) return;

	Point* ajustedPoints = new Point[poly->count];
	for (unsigned int i=0; i<poly->count; i++)
	{
		ajustedPoints[i] = Point(poly->points[i].x + xCorr - Viewport.x, poly->points[i].y + yCorr - Viewport.y);
	}
	drawPolygon(ajustedPoints, poly->count, color, LineLoop);
	delete[] ajustedPoints;
	if (fill)
	{
		// not a good to do this here, will be right to do it in game
		Color c = color;
		c.a = c.a/2;
		// end of bad code
		std::vector<Point> triangulation;
		std::list<Trapezoid>::iterator iter;
		for (iter = poly->trapezoids.begin(); iter != poly->trapezoids.end(); ++iter)
		{
			int y_top = iter->y1;
			int y_bot = iter->y2;

			int ledge = iter->left_edge;
			int redge = iter->right_edge;
			Point& a = poly->points[ledge];
			Point& b = poly->points[(ledge+1)%(poly->count)];
			Point& c = poly->points[redge];
			Point& d = poly->points[(redge+1)%(poly->count)];

			Point topleft, topright, bottomleft, bottomright;
			topleft.y = topright.y = y_top + yCorr - Viewport.y;
			bottomleft.y = bottomright.y = y_bot + yCorr - Viewport.y;

			int lt, rt, py;
			py = y_top;
			lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
			rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y);
			topleft.x = lt + xCorr - Viewport.x;
			topright.x = rt + xCorr - Viewport.x;

			py = y_bot;
			lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
			rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y);
			bottomleft.x = lt + xCorr - Viewport.x;
			bottomright.x = rt + xCorr - Viewport.x;

			triangulation.push_back(bottomleft);
			triangulation.push_back(topleft);
			triangulation.push_back(topright);
			triangulation.push_back(bottomleft);
			triangulation.push_back(topright);
			triangulation.push_back(bottomright);
		}
		drawPolygon(&triangulation[0], triangulation.size(), c, FilledTriangulation);
	}
}

void GLVideoDriver::DrawEllipse(short cx, short cy, unsigned short xr, unsigned short yr, const Color& color, bool clipped)
{
	if (clipped) 
	{
		cx += xCorr;
		cy += yCorr;
		if ((cx >= xCorr + Viewport.w || cy >= yCorr + Viewport.h) || (cx < xCorr || cy < yCorr)) 
			return;
	} 
	else 
	{
		if ((cx >= disp->w || cy >= disp->h) || (cx < 0 || cy < 0))
			return;
	}
	return drawEllipse(cx, cy, xr, yr, 3, color);
}
/*
void GLVideoDriver::DrawEllipseSegment(short cx, short cy, unsigned short xr, unsigned short yr, const Color& color, double anglefrom, double angleto, bool drawlines, bool clipped)
{}*/

void GLVideoDriver::DrawCircle(short cx, short cy, unsigned short r, const Color& color, bool clipped)
{
	return DrawEllipse(cx, cy, r, r, color, clipped); 
}

int GLVideoDriver::SwapBuffers()
{	
	int val = SDLVideoDriver::SwapBuffers();
	SDL_GL_SwapWindow(window);
	paletteManager->ClearUnused(true);
	core->RedrawAll();
	spritesPerFrame = 0;
	return val;
}

void GLVideoDriver::DestroyMovieScreen()
{
	SDL20VideoDriver::DestroyMovieScreen();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
}

Sprite2D* GLVideoDriver::GetScreenshot(Region r)
{
	unsigned int w = r.w ? r.w : width - r.x;
	unsigned int h = r.h ? r.h : height - r.y;
	
	Uint32* glPixels = (Uint32*)malloc( w * h * 4 );
	Uint32* pixels = (Uint32*)malloc( w * h * 4 );
#ifdef USE_GL
	glReadBuffer(GL_BACK);
#endif
	glReadPixels(r.x, r.y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, glPixels);
	// flip pixels vertical
	Uint32* pixelDstPointer = pixels;
	Uint32* pixelSrcPointer = glPixels + (h-1)*w;
	for(unsigned int i=0; i<h; i++)
	{
		memcpy(pixelDstPointer, pixelSrcPointer, w*4);
		pixelDstPointer += w;
		pixelSrcPointer -= w;
	}
	free(glPixels);
	Sprite2D* screenshot = new GLTextureSprite2D(w, h, 32, pixels, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	return screenshot;
}


#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB53, "SDL2 GL Video Driver")
PLUGIN_DRIVER(GLVideoDriver, "sdl")
END_PLUGIN()
