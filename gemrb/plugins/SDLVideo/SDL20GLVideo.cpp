#ifdef USE_GL
#include <GL/glew.h>
#pragma comment(lib, "glew32")
#pragma comment(lib, "opengl32")
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#pragma comment(lib, "libGLESv2")
#endif
#include "SDL20GLVideo.h"
#include "Interface.h"
#include "Game.h" // for GetGlobalTint
#include "GLTextureSprite2D.h"
#include "Shader.h"
#include "Matrix.h"

using namespace GemRB;

const char* vertexRect=
"uniform mat4 u_matrix;\n"
"attribute vec2 a_position;\n"
"attribute vec4 a_color;\n"
"varying vec4 v_color;\n"
"void main()\n"
"{\n"
"  gl_Position = u_matrix * vec4(a_position, 0.0, 1.0);\n"
"  v_color = a_color;\n"
"}\n";

const char* fragmentRect =
"varying vec4 v_color;	\n"
"void main()            \n"
"{\n"
"  gl_FragColor = v_color;\n"
"}\n";

const char* vertex =
"uniform mat4 u_matrix;\n"
"attribute vec2 a_position;\n"
"attribute vec2 a_texCoord;\n"
"attribute float a_alphaModifier;\n"
"attribute vec4 a_tint;\n"
"varying vec2 v_texCoord;\n"
"varying float v_alphaModifier;\n"
"varying vec4 v_tint;\n"
"void main()\n"
"{\n"
"  gl_Position = u_matrix * vec4(a_position, 0.0, 1.0);\n"
"  v_texCoord = a_texCoord;\n"
"  v_alphaModifier = a_alphaModifier;\n"
"  v_tint = a_tint;\n"
"}\n";


const char* fragment =
#ifndef USE_GL
"precision highp float;             \n"
#endif
"varying vec2 v_texCoord;			\n"
"uniform sampler2D s_texture;		\n"
"varying float v_alphaModifier;		\n"
"varying vec4 v_tint;				\n"
"void main()                        \n"
"{\n"
"  vec4 color = texture2D(s_texture, v_texCoord); \n"
"  gl_FragColor = vec4(color.r*v_tint.r, color.g*v_tint.g, color.b*v_tint.b, color.a * v_alphaModifier);\n"
"}\n";

const char* fragmentPal =
#ifndef USE_GL
"precision highp float;					 \n"
#endif
"uniform sampler2D s_texture;	// own texture \n"
"uniform sampler2D s_palette;	// palette 256 x 1 pixels \n"
"uniform sampler2D s_mask;		// optional mask \n"
"varying vec2 v_texCoord;\n"
"varying float v_alphaModifier;\n"
"varying vec4 v_tint;				\n"
"void main()\n"
"{\n"
"  float alphaModifier = v_alphaModifier;\n"
"  if (texture2D(s_mask, v_texCoord).a == 0.0) alphaModifier = 0.0;\n"
"  float index = texture2D(s_texture, v_texCoord).a;\n"
"  vec4 color = texture2D(s_palette, vec2(index, 0.0));\n"
"  gl_FragColor = vec4(color.r*v_tint.r, color.g*v_tint.g, color.b*v_tint.b, color.a * alphaModifier);\n"
"}\n";

const char* fragmentPalGrayed =
#ifndef USE_GL
"precision highp float;					 \n"
#endif
"uniform sampler2D s_texture;	// own texture \n"
"uniform sampler2D s_palette;	// palette 256 x 1 pixels \n"
"uniform sampler2D s_mask;		// optional mask \n"
"varying vec2 v_texCoord;\n"
"varying float v_alphaModifier;\n"
"varying vec4 v_tint;				\n"
"void main()\n"
"{\n"
"  float alphaModifier = v_alphaModifier;\n"
"  if (texture2D(s_mask, v_texCoord).a == 0.0) alphaModifier = 0.0;\n"
"  float index = texture2D(s_texture, v_texCoord).a;\n"
"  vec4 color = texture2D(s_palette, vec2(index, 0.0));\n"
"  float gray = (color.r + color.g + color.b)*0.333333;\n"
"  gl_FragColor = vec4(gray, gray, gray, color.a * alphaModifier);\n"
"}\n";

const char* fragmentPalSepia =
#ifndef USE_GL
"precision highp float;					 \n"
#endif
"uniform sampler2D s_texture;	// own texture \n"
"uniform sampler2D s_palette;	// palette 256 x 1 pixels \n"
"uniform sampler2D s_mask;		// optional mask \n"
"varying vec2 v_texCoord;\n"
"varying float v_alphaModifier;\n"
"varying vec4 v_tint;				\n"
"const vec3 lightColor = vec3(0.9, 0.9, 0.5);\n"
"const vec3 darkColor = vec3(0.2, 0.05, 0.0);\n"
"void main()\n"
"{\n"
"  float alphaModifier = v_alphaModifier;\n"
"  if (texture2D(s_mask, v_texCoord).a == 0.0) alphaModifier = 0.0;\n"
"  float index = texture2D(s_texture, v_texCoord).a;\n"
"  vec4 color = texture2D(s_palette, vec2(index, 0.0));\n"
"  float gray = (color.r + color.g + color.b)*0.333333;\n"
"  vec3 sepia = darkColor*(1.0 - gray) + lightColor*gray;\n"
"  gl_FragColor = vec4(sepia, color.a * alphaModifier);\n"
"}\n";

GLVideoDriver::~GLVideoDriver()
{
	glDeleteProgram(program32);
	glDeleteProgram(programPal);
	glDeleteProgram(programPalGrayed);
	glDeleteProgram(programPalSepia);
	glDeleteProgram(programRect);
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
	GLenum e = glGetError();

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
	createPrograms();
	glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
	spritesPerFrame = 0;
	return GEM_OK;
}

void GLVideoDriver::useProgram(GLuint program)
{
	if(lastUsedProgram == program) return;
	glUseProgram(program);
	lastUsedProgram = program;
}

void GLVideoDriver::createPrograms()
{
	ShaderOperationResult* result = Shader::BuildProgram(vertex, fragment);
	if(result->Id != 0) program32 = result->Id;
	delete result;
	glUseProgram(program32);
	glUniform1i(glGetUniformLocation(program32, "s_texture"), 0);
	float matrix[16];
	Matrix::SetIdentityM(matrix);
	glUniformMatrix4fv(glGetUniformLocation(program32, "u_matrix"), 1, GL_FALSE, matrix);
	glUseProgram(0);

	result = Shader::BuildProgram(vertex, fragmentPal);
	if(result->Id != 0) programPal = result->Id;
	delete result;
	glUseProgram(programPal);
	glUniform1i(glGetUniformLocation(programPal, "s_texture"), 0);
	glUniform1i(glGetUniformLocation(programPal, "s_palette"), 1);
	glUniform1i(glGetUniformLocation(programPal, "s_mask"), 2);
	glUniformMatrix4fv(glGetUniformLocation(programPal, "u_matrix"), 1, GL_FALSE, matrix);
	glUseProgram(0);

	result = Shader::BuildProgram(vertex, fragmentPalGrayed);
	if(result->Id != 0) programPalGrayed = result->Id;
	delete result;
	glUseProgram(programPalGrayed);
	glUniform1i(glGetUniformLocation(programPalGrayed, "s_texture"), 0);
	glUniform1i(glGetUniformLocation(programPalGrayed, "s_palette"), 1);
	glUniform1i(glGetUniformLocation(programPalGrayed, "s_mask"), 2);
	glUniformMatrix4fv(glGetUniformLocation(programPalGrayed, "u_matrix"), 1, GL_FALSE, matrix);
	glUseProgram(0);

	result = Shader::BuildProgram(vertex, fragmentPalSepia);
	if(result->Id != 0) programPalSepia = result->Id;
	delete result;
	glUseProgram(programPalSepia);
	glUniform1i(glGetUniformLocation(programPalSepia, "s_texture"), 0);
	glUniform1i(glGetUniformLocation(programPalSepia, "s_palette"), 1);
	glUniform1i(glGetUniformLocation(programPalSepia, "s_mask"), 2);
	glUniformMatrix4fv(glGetUniformLocation(programPalSepia, "u_matrix"), 1, GL_FALSE, matrix);
	glUseProgram(0);
	
	result = Shader::BuildProgram(vertexRect, fragmentRect);
	if(result->Id != 0) programRect = result->Id;
	delete result;
	glUseProgram(programRect);
	glUniformMatrix4fv(glGetUniformLocation(programRect, "u_matrix"), 1, GL_FALSE, matrix);
	glUseProgram(0);
	
	lastUsedProgram = 0;
}

Sprite2D* GLVideoDriver::CreateSprite(int w, int h, int bpp, ieDword rMask, ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	GLTextureSprite2D* spr = new GLTextureSprite2D(w, h, bpp, pixels, rMask, gMask, bMask, aMask);
	return spr;
}

Sprite2D* GLVideoDriver::CreatePalettedSprite(int w, int h, int bpp, void* pixels, Color* palette, bool cK, int index)
{
	GLTextureSprite2D* spr = new GLTextureSprite2D(w, h, bpp, pixels);
	Palette* pal = new Palette(palette);
	spr->SetPalette(pal);
	if (cK) spr->SetColorKey(index);
	return spr;
}

Sprite2D* GLVideoDriver::CreateSprite8(int w, int h, void* pixels, Palette* palette, bool cK, int index)
{
	return CreatePalettedSprite(w, h, 8, pixels, palette->col, cK, index);
}

void GLVideoDriver::blitSprite(GLTextureSprite2D* spr, int x, int y, const Region* clip, unsigned int flags, const Color* tint, GLTextureSprite2D* mask)
{
	float hscale, vscale;
	SDL_Rect spriteRect;
	spriteRect.w = spr->Width;
	spriteRect.h = spr->Height;
	if(clip)
	{
		// test region
		if(clip->x > x) { spriteRect.x = x; spriteRect.w -= (clip->x - x); }
		if(clip->y > y) { spriteRect.y = y; spriteRect.h -= (clip->y - y); }
		if (x + spriteRect.w > clip->x + clip -> w) { spriteRect.w = clip->x + clip->w - x; }
		if (y + spriteRect.h > clip->y + clip -> h) { spriteRect.h = clip->y + clip->h - y; }
		if (spriteRect.w <= 0 || spriteRect.h <= 0) return;

		glViewport(clip->x, height - (clip->y + clip->h), clip->w, clip->h);
		glScissor(clip->x, height - (clip->y + clip->h), clip->w, clip->h);
		spriteRect.x = x - clip->x;
		spriteRect.y = y - clip->y;
		spriteRect.w = spr->Width;
		spriteRect.h = spr->Height;
		hscale = 2.0f/(float)clip->w;
		vscale = 2.0f/(float)clip->h;
	}
	else
	{
		glViewport(0, 0, width, height);
		glScissor(0, 0, width, height);
		spriteRect.x = x; 
		spriteRect.y = y;
		hscale = 2.0f/(float)width;
		vscale = 2.0f/(float)height;
	}

	// color tint
	Color colorTint;
	if (tint)
		colorTint = *tint;
	else
		colorTint.r = colorTint.b = colorTint.g = colorTint.a = 255;
	
	// we do flipping here
	bool hflip = spr->renderFlags & RENDER_FLIP_HORIZONTAL;
	bool vflip = spr->renderFlags & RENDER_FLIP_VERTICAL;
	if (flags & BLIT_MIRRORX) hflip = !hflip;
	if (flags & BLIT_MIRRORY) vflip = !vflip;
	GLfloat* textureCoords;
	if (hflip && vflip)
	{
		GLfloat coords[] = { 1.0f,1.0f, 0.0f,1.0f, 1.0f,0.0f, 0.0f,0.0f };
		textureCoords = coords;
	}
	else if (hflip)
	{
		GLfloat coords[] = { 1.0f,0.0f, 0.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f };
		textureCoords = coords;
	}
	else if (vflip)
	{
		GLfloat coords[] = { 0.0f,1.0f, 1.0f,1.0f, 0.0f,0.0f, 1.0f,0.0f };
		textureCoords = coords;
	}
	else
	{
		GLfloat coords[] = { 0.0f,0.0f, 1.0f,0.0f, 0.0f,1.0f, 1.0f,1.0f };
		textureCoords = coords;
	}

	// alpha modifier
	GLfloat alphaModifier = flags & BLIT_HALFTRANS ? 0.5f : 1.0f;

	// data
	GLfloat data[] = 
	{	    
		-1.0f + spriteRect.x*hscale, 1.0f - spriteRect.y*vscale, textureCoords[0], textureCoords[1], alphaModifier, (GLfloat)colorTint.r/255, (GLfloat)colorTint.g/255, (GLfloat)colorTint.b/255, (GLfloat)colorTint.a/255,
		-1.0f + (spriteRect.x + spriteRect.w)*hscale, 1.0f - spriteRect.y*vscale, textureCoords[2], textureCoords[3], alphaModifier, (GLfloat)colorTint.r/255, (GLfloat)colorTint.g/255, (GLfloat)colorTint.b/255, (GLfloat)colorTint.a/255,
		-1.0f + spriteRect.x*hscale, 1.0f - (spriteRect.y + spriteRect.h)*vscale, textureCoords[4], textureCoords[5], alphaModifier, (GLfloat)colorTint.r/255, (GLfloat)colorTint.g/255, (GLfloat)colorTint.b/255, (GLfloat)colorTint.a/255,
		-1.0f + (spriteRect.x + spriteRect.w)*hscale, 1.0f - (spriteRect.y + spriteRect.h)*vscale, textureCoords[6], textureCoords[7], alphaModifier, (GLfloat)colorTint.r/255, (GLfloat)colorTint.g/255, (GLfloat)colorTint.b/255, (GLfloat)colorTint.a/255
	};

	// shader program selection
	GLuint program;
	if(spr->IsPaletted())
	{
		if (flags & BLIT_GREY)
			program = programPalGrayed;
		else if (flags & BLIT_SEPIA)
			program = programPalSepia;
		else
			program = programPal;
	}
	else
		program = program32;
	useProgram(program);

	glActiveTexture(GL_TEXTURE0);
	GLuint texture = spr->GetTexture();
	glBindTexture(GL_TEXTURE_2D, texture);
	
	if (spr->IsPaletted())
	{
		glActiveTexture(GL_TEXTURE1);
		GLuint palTexture = spr->GetPaletteTexture();
		glBindTexture(GL_TEXTURE_2D, palTexture);
	}
	if (mask)
	{
		glActiveTexture(GL_TEXTURE2);
		GLuint maskTexture = ((GLTextureSprite2D*)mask)->GetMaskTexture();
		glBindTexture(GL_TEXTURE_2D, maskTexture);
	}
	else
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	GLint a_position = glGetAttribLocation(program, "a_position");
	GLint a_texCoord = glGetAttribLocation(program, "a_texCoord");
	GLint a_alphaModifier = glGetAttribLocation(program, "a_alphaModifier");
	GLint a_tint = glGetAttribLocation(program, "a_tint");

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

	glVertexAttribPointer(a_position, VERTEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*9, 0);
	glVertexAttribPointer(a_texCoord, TEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*9, BUFFER_OFFSET(sizeof(GLfloat)*VERTEX_SIZE));
	glVertexAttribPointer(a_alphaModifier, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*9, BUFFER_OFFSET(sizeof(GLfloat)*(VERTEX_SIZE + TEX_SIZE)));
	glVertexAttribPointer(a_tint, COLOR_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*9, BUFFER_OFFSET(sizeof(GLfloat)*(VERTEX_SIZE + TEX_SIZE + 1)));

	glEnableVertexAttribArray(a_position);
	glEnableVertexAttribArray(a_texCoord);
	glEnableVertexAttribArray(a_alphaModifier);
	glEnableVertexAttribArray(a_tint);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(a_tint);
	glDisableVertexAttribArray(a_alphaModifier);
	glDisableVertexAttribArray(a_texCoord);
	glDisableVertexAttribArray(a_position);

	glDeleteBuffers(1, &buffer);
	spritesPerFrame++;
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
	if (core->GetGame()) 
	{
		const Color* totint = core->GetGame()->GetGlobalTint();
		return blitSprite((GLTextureSprite2D*)spr, tx, ty, clip, blitFlags, totint, (GLTextureSprite2D*)mask);
	}
	return blitSprite((GLTextureSprite2D*)spr, tx, ty, clip, blitFlags, NULL, (GLTextureSprite2D*)mask);
}


void GLVideoDriver::BlitSprite(const Sprite2D* spr, int x, int y, bool anchor, const Region* clip, Palette* palette)
{
	// x, y is a position on screen (if anchor) or viewport (if !anchor)
	GLTextureSprite2D* glSprite = (GLTextureSprite2D*)spr;
	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	if (!anchor) 
	{
		tx -= Viewport.x;
		ty -= Viewport.y;
	}
	if (palette || glSprite->IsPaletted())
	{
		if (palette) glSprite->SetPalette(palette);
	}
	
	return blitSprite(glSprite, tx, ty, clip);
}

void GLVideoDriver::BlitGameSprite(const Sprite2D* spr, int x, int y, unsigned int flags, Color tint, SpriteCover* cover, Palette *palette,	const Region* clip, bool anchor)
{
	int tx = x - spr->XPos;
	int ty = y - spr->YPos;
	if (!anchor) 
	{
		tx -= Viewport.x;
		ty -= Viewport.y;
	}
	GLTextureSprite2D* glSprite = (GLTextureSprite2D*)spr;
	if(glSprite->IsPaletted())
	{
		if (palette)
		{
			if (palette) glSprite->SetPalette(palette);
		}
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
				blitSprite((GLTextureSprite2D*)spr, tx, ty, clip, flags, &tint);
			}
		}
	}
	else
		blitSprite((GLTextureSprite2D*)spr, tx, ty, clip, flags);
	if (cover)
	{
		int i = 0;
	}
}

void GLVideoDriver::DrawRect(const Region& rgn, const Color& color, bool fill, bool clipped)
{
	if (fill) return drawColoredRect(rgn, color);
	else
	{
		DrawHLine(rgn.x, rgn.y, rgn.x + rgn.w - 1, color, clipped);
		DrawVLine(rgn.x, rgn.y, rgn.y + rgn.h - 1, color, clipped);
		DrawHLine(rgn.x, rgn.y + rgn.h - 1, rgn.x + rgn.w - 1, color, clipped);
		DrawVLine(rgn.x + rgn.w - 1, rgn.y, rgn.y + rgn.h - 1, color, clipped);
	}
}

void GLVideoDriver::DrawHLine(short x1, short y, short x2, const Color& color, bool clipped)
{
	Region rgn;
	rgn.x = x1;
	rgn.y = y;
	rgn.h = 1;
	rgn.w = x2 - x1;
	return drawColoredRect(rgn, color);
}

void GLVideoDriver::DrawVLine(short x, short y1, short y2, const Color& color, bool clipped)
{
	Region rgn;
	rgn.x = x;
	rgn.y = y1;
	rgn.w = 1;
	rgn.h = y2 - y1;
	return drawColoredRect(rgn, color);
}

void GLVideoDriver::drawColoredRect(const Region& rgn, const Color& color)
{
	if (SDL_ALPHA_TRANSPARENT == color.a) return;

	glScissor(rgn.x, height - rgn.y - rgn.h, rgn.w, rgn.h);
	if (SDL_ALPHA_OPAQUE == color.a) // possible to work faster than shader but a lot... may be disable in future
	{
		glClearColor(color.r/255, color.g/255, color.b/255, color.a/255);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		useProgram(programRect);
		glViewport(rgn.x, height - rgn.y - rgn.h, rgn.w, rgn.h);
		GLfloat data[] = 
		{ 
			-1.0f,  1.0f, (GLfloat)color.r/255, (GLfloat)color.g/255, (GLfloat)color.b/255, (GLfloat)color.a/255,
				1.0f,  1.0f, (GLfloat)color.r/255, (GLfloat)color.g/255, (GLfloat)color.b/255, (GLfloat)color.a/255,
			-1.0f, -1.0f, (GLfloat)color.r/255, (GLfloat)color.g/255, (GLfloat)color.b/255, (GLfloat)color.a/255,
				1.0f, -1.0f, (GLfloat)color.r/255, (GLfloat)color.g/255, (GLfloat)color.b/255, (GLfloat)color.a/255
		};
		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		GLint a_position = glGetAttribLocation(programRect, "a_position");
		GLint a_color = glGetAttribLocation(programRect, "a_color");
			
		glVertexAttribPointer(a_position, VERTEX_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, 0);
		glVertexAttribPointer(a_color, COLOR_SIZE, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, BUFFER_OFFSET(sizeof(GLfloat)*VERTEX_SIZE));

		glEnableVertexAttribArray(a_position);
		glEnableVertexAttribArray(a_color);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableVertexAttribArray(a_color);
		glDisableVertexAttribArray(a_position);

		glDeleteBuffers(1, &buffer);
	}
}

int GLVideoDriver::SwapBuffers()
{	
	int val = SDLVideoDriver::SwapBuffers();
	SDL_GL_SwapWindow(window);
	core->RedrawAll();
	spritesPerFrame = 0;
	return val;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB50, "SDL Video Driver")
PLUGIN_DRIVER(GLVideoDriver, "sdl")
END_PLUGIN()
