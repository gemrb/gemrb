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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DirectXVideo/Poly.h,v 1.2 2003/11/25 13:48:02 balrog994 Exp $
 *
 */

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
