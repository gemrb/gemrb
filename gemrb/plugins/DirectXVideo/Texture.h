#ifndef TEXTURE_H
#define TEXTURE_H

#include "DirectXVideoDriver.h"
#include "../../includes/RGBAColor.h"
#include "d3d9.h"

class Texture
{
	
public:
	Texture() : pTexture(NULL) { Clear(); }
	Texture(void * pixels, int w, int h, int bpp, void * palette = NULL, bool cK = false, int index = 0) : pTexture(NULL) { Init(pixels, w, h, bpp, palette, cK, index); }
	~Texture() { Clear(); }
	void Init (void * pixels, int w, int h, int bpp, void * palette = NULL, bool cK = false, int index = 0);
	void Clear();

	LPDIRECT3DTEXTURE9	pTexture;
	int m_Width;
	int m_Height;
	int m_Pitch;
	void * palette;
	bool hasCK;
	int colorKey;
	int paletteIndex;

};

#endif
