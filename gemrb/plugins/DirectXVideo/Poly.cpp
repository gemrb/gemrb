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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DirectXVideo/Poly.cpp,v 1.3 2004/02/24 22:20:38 balrog994 Exp $
 *
 */

#include "Poly.h"

extern LPDIRECT3D9 lpD3D;
extern LPDIRECT3DDEVICE9 lpD3DDevice;
extern int ScreenWidth;
extern int ScreenHeight;

void Poly::Clear()
{
	if (m_VertexBuffer) {
		m_VertexBuffer->Release();
	}
	if (m_Texture) {
		delete( m_Texture );
	}
}

void Poly::Init()
{
	// Create a 4-point vertex buffer
	if (FAILED( lpD3DDevice->CreateVertexBuffer( 4 * sizeof( CUSTOMVERTEX ),
								0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT,
								&m_VertexBuffer, NULL ) )) {
		return ;
	}

	CUSTOMVERTEX* pVertices;
	if (FAILED( m_VertexBuffer->Lock( 0, 4 * sizeof( CUSTOMVERTEX ),
									( void * * ) &pVertices, 0 ) )) {
		return ;
	}

	pVertices[0].x = -1.0f;
	pVertices[0].y = 1.0f;
	pVertices[0].z = 0.5f;
	pVertices[0].color = 0xffffffff;
	pVertices[0].tu = 0.0;
	pVertices[0].tv = 0.0;
	pVertices[1].x = 1.0f;
	pVertices[1].y = 1.0f;
	pVertices[1].z = 0.5f;
	pVertices[1].color = 0xffffffff;
	pVertices[1].tu = 1.0;
	pVertices[1].tv = 0.0;
	pVertices[2].x = -1.0f;
	pVertices[2].y = -1.0f;
	pVertices[2].z = 0.5f;
	pVertices[2].color = 0xffffffff;
	pVertices[2].tu = 0.0;
	pVertices[2].tv = 1.0;
	pVertices[3].x = 1.0f;
	pVertices[3].y = -1.0f;
	pVertices[3].z = 0.5f;
	pVertices[3].color = 0xffffffff;
	pVertices[3].tu = 1.0;
	pVertices[3].tv = 1.0;

	m_VertexBuffer->Unlock();
}

void Poly::SetVertRect(int x, int y, int w, int h, float d)
{
	float left, top, right, bottom;
	left = ( ( float ) x / ( float ) ScreenWidth ) * 2.0f - 1.0f;
	right = ( ( float ) ( x + w ) / ( float ) ScreenWidth ) * 2.0f - 1.0f;
	bottom = ( ( float ) y / ( float ) ScreenHeight ) * 2.0f - 1.0f;
	top = ( ( float ) ( y + h ) / ( float ) ScreenHeight ) * 2.0f - 1.0f;
	top = -top;
	bottom = -bottom;
	//left = x-(ScreenWidth/2);
	//right = left+w;
	//top = y-(ScreenHeight/2);
	//bottom = top+h;

	CUSTOMVERTEX* pVertices;
	m_VertexBuffer->Lock( 0, 4 * sizeof( CUSTOMVERTEX ),
						( void * * ) &pVertices, 0 );

	pVertices[0].x = left;
	pVertices[0].y = bottom;
	pVertices[0].z = d;
	pVertices[1].x = right;
	pVertices[1].y = bottom;
	pVertices[1].z = d;
	pVertices[2].x = left;
	pVertices[2].y = top;
	pVertices[2].z = d;
	pVertices[3].x = right;
	pVertices[3].y = top;
	pVertices[3].z = d;

	m_VertexBuffer->Unlock();
}
