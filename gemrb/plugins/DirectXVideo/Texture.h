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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DirectXVideo/Texture.h,v 1.4 2004/02/24 22:20:38 balrog994 Exp $
 *
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include "DirectXVideoDriver.h"
#include "../../includes/RGBAColor.h"
#include "d3d9.h"

class Texture {
public:
	Texture()
		: pTexture( NULL )
	{
		Clear();
	}
	Texture(void* pixels, int w, int h, int bpp, void* palette = NULL,
		bool cK = false, int index = 0)
		: pTexture( NULL )
	{
		Init( pixels, w, h, bpp, palette, cK, index );
	}
	~Texture()
	{
		Clear();
	}
	void Init(void* pixels, int w, int h, int bpp, void* palette = NULL,
		bool cK = false, int index = 0);
	void Clear();

	LPDIRECT3DTEXTURE9 pTexture;
	int m_Width;
	int m_Height;
	int m_Pitch;
	void* palette;
	bool hasCK;
	int colorKey;
	int paletteIndex;
};

#endif
