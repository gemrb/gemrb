#ifdef USE_GL
#include <GL/glew.h>
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#endif

#include "SDLVideo.h"
#include "GLTextureSprite2D.h"

using namespace GemRB;

static Uint8 GetShiftValue(Uint32 value)
{
	for(int i=0; i<sizeof(value)*8; i+=8)
	{
		if(((value >> i) & 0x1) > 0) return i;
	}
	return 24;
}

GLTextureSprite2D::GLTextureSprite2D (int Width, int Height, int Bpp, void* pixels,	Uint32 rmask, Uint32 gmask, 
										Uint32 bmask, Uint32 amask) : Sprite2D(Width, Height, Bpp, pixels)
{
	currentPalette = NULL;
	glPaletteTexture = 0;
	glMaskTexture = 0;
	colorKeyIndex = 256; // invalid index
	rMask = rmask;
	gMask = gmask;
	bMask = bmask;
	aMask = amask;
	if (Width > 0 && Height > 0) createGlTexture();
}

GLTextureSprite2D::~GLTextureSprite2D()
{
	glDeleteTextures(1, &glTexture);
	glDeleteTextures(1, &glPaletteTexture);
	glDeleteTextures(1, &glMaskTexture);
}

GLTextureSprite2D::GLTextureSprite2D(const GLTextureSprite2D &obj) : Sprite2D(obj)
{
	// copies only 8 bit sprites
	currentPalette = NULL;
	colorKeyIndex = obj.colorKeyIndex;
	rMask = obj.rMask;
	gMask = obj.bMask;
	bMask = obj.bMask;
	aMask = obj.aMask;
	SetPalette(obj.currentPalette);
	if (Width > 0 && Height > 0)
	{
		createGlTexture();
	}
}

GLTextureSprite2D* GLTextureSprite2D::copy() const
{
	return new GLTextureSprite2D(*this);
}


void GLTextureSprite2D::SetPalette(Palette *pal)
{
	if(!IsPaletted() || currentPalette == pal) return;
	if (pal != NULL) 
	{
		pal->acquire();
	}
	if (currentPalette != NULL) 
	{
		currentPalette->release();
	}
	currentPalette = pal;
	glDeleteTextures(1, &glPaletteTexture);
	glPaletteTexture = 0;
}

Palette* GLTextureSprite2D::GetPalette() const
{
	if(!IsPaletted() || currentPalette == NULL) return NULL;
	currentPalette->acquire();
	return currentPalette;
}

void GLTextureSprite2D::SetColorKey(ieDword index)
{
	colorKeyIndex = index;
	glDeleteTextures(1, &glPaletteTexture);
	glDeleteTextures(1, &glMaskTexture);
	glPaletteTexture = 0;
	glMaskTexture = 0;
}

Color GLTextureSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	if (x >= Width || y >= Height) return Color();
	if (Bpp == 8)
	{
		Uint8 pixel = ((Uint8*)pixels)[y*Width + x];
		Color color = currentPalette->col[pixel]; // hack (we have a = 0 for non-transparent pixels on palette) 
		if(pixel != colorKeyIndex) color.a = 255;
		return color;
	}
	return Color(); // not supported yet
}

void GLTextureSprite2D::createGlTexture()
{
	glDeleteTextures(1, &glTexture);
	glGenTextures(1, &glTexture);
	glBindTexture(GL_TEXTURE_2D, glTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if(Bpp == 32) // true color textures
	{
		int* buffer = new int[Width * Height];
		for(int i = 0; i < Width*Height; i++)
		{
			Uint32 src = ((Uint32*) pixels)[i];
			Uint8 r = (src & rMask) >> GetShiftValue(rMask);
			Uint8 g = (src & gMask) >> GetShiftValue(gMask);
			Uint8 b = (src & bMask) >> GetShiftValue(bMask);
			Uint8 a = (src & aMask) >> GetShiftValue(aMask); if (a == 0) a = 0xFF; //no transparency
			buffer[i] = r | (g << 8) | (b << 16) | (a << 24);
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) buffer);
		delete buffer;
	}
	else if(Bpp == 8) // indexed
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Width, Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (GLvoid*) pixels);
	}
}

void GLTextureSprite2D::createGlTextureForPalette()
{
	glDeleteTextures(1, &glPaletteTexture);
	glGenTextures(1, &glPaletteTexture);
	glBindTexture(GL_TEXTURE_2D, glPaletteTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	Color* colors = new Color[256];
	memcpy(colors, currentPalette->col, sizeof(Color)*256);
	for(int i=0; i<256; i++)
	{
		if(colors[i].a == 0)
		{
			colors[i].a = 0xFF;
		}
		if(i == colorKeyIndex) 
			colors[i].a = 0;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) colors);
	delete colors;
}

void GLTextureSprite2D::createGLMaskTexture()
{
	glDeleteTextures(1, &glMaskTexture);
	glGenTextures(1, &glMaskTexture);
	glBindTexture(GL_TEXTURE_2D, glMaskTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	Uint8* mask = new Uint8[Width*Height];
	for(int i=0; i<Width*Height; i++)
	{
		if (((Uint8*) pixels)[i] == colorKeyIndex) mask[i] = 0xFF;
		else mask[i] = 0x00;  
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Width, Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (GLvoid*) mask);
	delete mask;
}

GLuint GLTextureSprite2D::GetPaletteTexture()
{
	if (!IsPaletted()) return 0;
	if (glPaletteTexture != 0) return glPaletteTexture;
	createGlTextureForPalette();
	return glPaletteTexture;
}

GLuint GLTextureSprite2D::GetPaletteTexture(Palette* pal)
{
	if (pal == NULL || !IsPaletted()) return 0; // nothing to do

	// we already have a texture from requested palette
	if (pal == currentPalette && glPaletteTexture != 0) return glPaletteTexture;

	SetPalette(pal);
	createGlTextureForPalette();
	return glPaletteTexture;
}

GLuint GLTextureSprite2D::GetMaskTexture()
{
	if (!IsPaletted()) return 0; // nothing to do
	if (glMaskTexture != 0) return glMaskTexture;
	createGLMaskTexture();
	return glMaskTexture;
}