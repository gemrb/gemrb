/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DirectXVideo/Texture.cpp,v 1.3 2003/11/25 13:48:02 balrog994 Exp $
 *
 */

#include "Texture.h"

extern LPDIRECT3D9			lpD3D;
extern LPDIRECT3DDEVICE9	lpD3DDevice;
extern int					ScreenWidth;
extern int					ScreenHeight;

void Texture::Init (void * pixels, int w, int h, int bpp, void * palette, bool cK , int index)
{
	HRESULT ddrval;

	m_Width=w;
	m_Height=h;
	m_Pitch=w*4;
	this->palette = NULL;

	hasCK = cK;
	colorKey = index;

	if(pTexture)
		pTexture->Release();
	
	ddrval = lpD3DDevice->CreateTexture( w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL );
	
	if(ddrval != D3D_OK)
		return;

	D3DLOCKED_RECT d3dlr;
	ddrval = pTexture->LockRect( 0, &d3dlr, 0, 0 );
	if(ddrval != D3D_OK)
		return;
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = m_Width;

	switch(bpp) {
		case 8:
			{
			this->palette = palette;
			Color * pal = (Color*)palette;
			unsigned char * pSrc = (unsigned char*)pixels;
			for (int i=0; i<m_Height; ++i)
				for(int j=0; j<m_Width; ++j) {
					int srcPos = i*m_Width + j;
					int desPos = i*DPitch + j;
					if(pSrc[srcPos] != index || !cK)
						pDst[desPos] = 255 << 24;
					else
						pDst[desPos] = 0;
					pDst[desPos] += pal[pSrc[srcPos]].r << 16;
					pDst[desPos] += pal[pSrc[srcPos]].g << 8;
					pDst[desPos] += pal[pSrc[srcPos]].b;
				}
			}
		break;

		case 32:
			{
			DWORD * pSrc = (DWORD *)pixels;
			int SPitch = m_Width;

			for (int i=0; i<m_Height; ++i)
				for (int j=0; j<m_Width; ++j) {
					int srcPos = i*SPitch + j;
					int desPos = i*DPitch + j;
					DWORD col = pSrc[srcPos] >> 8;
					if(pSrc[srcPos] != index || !cK)
						col += 255 << 24;
					pDst[desPos] = col;
				}
			}
		break;
	}			

	pTexture->UnlockRect (0);

}

void Texture::Clear()
{
	if(pTexture)
		pTexture->Release();
}
