#ifndef POLY_H
#define POLY_H

#include "DirectXVideoDriver.h"
#include "Texture.h"
#include "d3d9.h"

class Poly
{
public:
	Poly() : m_VertexBuffer(NULL) { Init(); }
	~Poly() { Clear(); }
	void Clear();
	void Init();
	void SetVertRect(int x, int y, int w, int h, float d); // Sets the rectangle used by the Verticies.

	LPDIRECT3DVERTEXBUFFER9	 m_VertexBuffer;
	Texture					*m_Texture;
};

#endif
