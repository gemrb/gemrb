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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MOSImporter/MOSImp.h,v 1.9 2004/08/02 22:14:24 guidoj Exp $
 *
 */

#ifndef MOSIMP_H
#define MOSIMP_H

#include "../Core/ImageMgr.h"

class MOSImp : public ImageMgr {
private:
	DataStream* str;
	bool autoFree;
	ieWord Width, Height, Cols, Rows;
	ieDword BlockSize, PalOffset;
public:
	MOSImp(void);
	~MOSImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Sprite2D* GetImage();
	/** No descriptions */
	void GetPalette(int index, int colors, Color* pal);
	/** Gets a Pixel index, not used in MOS */
	unsigned int GetPixelIndex(unsigned int x, unsigned int y)
	{
		return 0;
	}
	/** Gets a Pixel from the Image, not used in MOS */
	Color GetPixel(unsigned int x, unsigned int y)
	{
		Color null = {
			0x00, 0x00, 0x00, 0x00
		};
		return null;
	}
public:
	void release(void)
	{
		delete this;
	}
};

#endif
