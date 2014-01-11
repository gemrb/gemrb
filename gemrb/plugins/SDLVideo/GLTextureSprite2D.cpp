#ifdef USE_GL
#include <GL/glew.h>
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#endif

#include "SDLVideo.h"
#include "GLTextureSprite2D.h"
#include "GLPaletteManager.h"

using namespace GemRB;

static Uint8 GetShiftValue(Uint32 value)
{
	for(unsigned int i=0; i<sizeof(value)*8; i+=8)
	{
		if(((value >> i) & 0x1) > 0) return i;
	}
	return 24;
}

GLTextureSprite2D::GLTextureSprite2D (int Width, int Height, int Bpp, void* pixels,	Uint32 rmask, Uint32 gmask, 
										Uint32 bmask, Uint32 amask) : Sprite2D(Width, Height, Bpp, pixels)
{
	currentPalette = NULL;
	attachedPalette = NULL;
	glTexture = 0;
	glPaletteTexture = 0;
	glAttachedPaletteTexture = 0;
	glMaskTexture = 0;
	colorKeyIndex = 0; // invalid index
	rMask = rmask;
	gMask = gmask;
	bMask = bmask;
	aMask = amask;
}

GLTextureSprite2D::~GLTextureSprite2D()
{
	if (glTexture != 0) glDeleteTextures(1, &glTexture);
	if (glMaskTexture != 0) glDeleteTextures(1, &glMaskTexture);
	if (glPaletteTexture != 0)
		paletteManager->RemovePaletteTexture(glPaletteTexture);
	if (glAttachedPaletteTexture != 0)
		paletteManager->RemovePaletteTexture(glAttachedPaletteTexture);
}

GLTextureSprite2D::GLTextureSprite2D(const GLTextureSprite2D &obj) : Sprite2D(obj)
{
	// copies only 8 bit sprites
	glTexture = 0;
	glMaskTexture = 0;
	glPaletteTexture = 0;
	glAttachedPaletteTexture = 0;
	currentPalette = NULL;
	attachedPalette = NULL;
	colorKeyIndex = obj.colorKeyIndex;
	paletteManager = obj.paletteManager;
	rMask = obj.rMask;
	gMask = obj.bMask;
	bMask = obj.bMask;
	aMask = obj.aMask;
	SetPalette(obj.currentPalette);
}

GLTextureSprite2D* GLTextureSprite2D::copy() const
{
	return new GLTextureSprite2D(*this);
}

void GLTextureSprite2D::SetPalette(Palette *pal)
{
	if (!IsPaletted() || pal == NULL || currentPalette == pal) return;
	pal->acquire();
	if (currentPalette != NULL) 
	{
		currentPalette->release();
	}
	if (glPaletteTexture != 0) paletteManager->RemovePaletteTexture(glPaletteTexture);
	glPaletteTexture = 0;
	currentPalette = pal;
}

Palette* GLTextureSprite2D::GetPalette() const
{
	if(!IsPaletted() || currentPalette == NULL) return NULL;
	currentPalette->acquire();
	return currentPalette;
}

void GLTextureSprite2D::SetColorKey(ieDword index)
{
	if (colorKeyIndex == index) return;
	colorKeyIndex = index;
	if(IsPaletted())
	{
		glDeleteTextures(1, &glMaskTexture);
		if (glPaletteTexture != 0) paletteManager->RemovePaletteTexture(glPaletteTexture);
		if (glAttachedPaletteTexture != 0) paletteManager->RemovePaletteTexture(glAttachedPaletteTexture);
		glPaletteTexture = 0;
		glAttachedPaletteTexture = 0;
		glMaskTexture = 0;
	}
	else
	{
		glDeleteTextures(1, &glTexture);
		glTexture = 0;
	}
}

Color GLTextureSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	if (x >= Width || y >= Height) return Color();
	if (Bpp == 8)
	{
		Uint8 pixel = ((Uint8*)pixels)[y*Width + x];
		Color color = currentPalette->col[pixel]; 
		// hack (we have a = 0 for non-transparent pixels on palette) 
		if (pixel != colorKeyIndex) color.a = 255;
		return color;
	}
	else
	{
		Uint32 pixel = ((Uint32*)pixels)[y*Width + x];
		Color color;
		color.r = (pixel & rMask) >> GetShiftValue(rMask);
		color.g = (pixel & gMask) >> GetShiftValue(gMask);
		color.b = (pixel & bMask) >> GetShiftValue(bMask);
		color.a = (pixel & aMask) >> GetShiftValue(aMask); 
		return color;
	}
}

void GLTextureSprite2D::createGlTexture()
{
	if (Bpp != 32 && Bpp != 8) return;
	if (glTexture != 0) glDeleteTextures(1, &glTexture);
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
			Uint8 a = (src & aMask) >> GetShiftValue(aMask); 
			if (a == 0) a = 0xFF; //no transparency
			if (src == colorKeyIndex) a = 0x00; // transparent
			buffer[i] = r | (g << 8) | (b << 16) | (a << 24);
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) buffer);
		delete[] buffer;
	}
	else if(Bpp == 8) // indexed
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Width, Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (GLvoid*) pixels);
	}
}

void GLTextureSprite2D::createGlTextureForPalette()
{
	glPaletteTexture = paletteManager->CreatePaletteTexture(currentPalette, colorKeyIndex);
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
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Width, Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (GLvoid*) mask);
	delete[] mask;
}

GLuint GLTextureSprite2D::GetPaletteTexture()
{
	if (!IsPaletted()) return 0;
	if (glPaletteTexture != 0) return glPaletteTexture;
	createGlTextureForPalette();
	return glPaletteTexture;
}

// use this method only for adding external palettes
GLuint GLTextureSprite2D::GetAttachedPaletteTexture(Palette* attached)
{
	if (!IsPaletted()) return 0; 
	// we already have a texture for requested palette
	if (attached == attachedPalette && glAttachedPaletteTexture != 0) return glAttachedPaletteTexture;
	attachedPalette = attached;
	if(!attachedPalette->IsShared())
	{
		attachedPalette->acquire();
	}
	glAttachedPaletteTexture = paletteManager->CreatePaletteTexture(attachedPalette, colorKeyIndex);
	return glAttachedPaletteTexture;
}

void GLTextureSprite2D::RemoveAttachedPaletteTexture()
{
	if (!attachedPalette->IsShared())
	{
		attachedPalette->release();
	}
	if (glAttachedPaletteTexture != 0) paletteManager->RemovePaletteTexture(glAttachedPaletteTexture);
	glAttachedPaletteTexture = 0;
}

GLuint GLTextureSprite2D::GetMaskTexture()
{
	if (!IsPaletted()) return 0; // nothing to do
	if (glMaskTexture != 0) return glMaskTexture;
	createGLMaskTexture();
	return glMaskTexture;
}

GLuint GLTextureSprite2D::GetTexture()
{
	if (glTexture != 0) return glTexture;
	if (Width > 0 && Height > 0)
	{
		createGlTexture();
	}
	return glTexture;
}

Uint8* GLTextureSprite2D::GetPixels()
{
	return NULL;
}

