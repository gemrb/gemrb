/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2010 The GemRB Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */


struct TRTinter_NoTint {
	Uint8 r(Uint8 v) const { return v; }
	Uint8 g(Uint8 v) const { return v; }
	Uint8 b(Uint8 v) const { return v; }
};

struct TRTinter_Tint {
	TRTinter_Tint(const Color& t) : tint(t) { }

	Uint8 r(Uint8 v) const { return (tint.r * v) >> 8; }
	Uint8 g(Uint8 v) const { return (tint.g * v) >> 8; }
	Uint8 b(Uint8 v) const { return (tint.b * v) >> 8; }

	Color tint;
};

struct TRBlender_Opaque {
	TRBlender_Opaque(const SDL_PixelFormat*) { }

	Uint32 operator()(Uint32 p, Uint32) const {
		return p;
	}
};

struct TRBlender_HalfTrans {
	TRBlender_HalfTrans(const SDL_PixelFormat* format)
	{
		mask =   (0x7F >> format->Rloss) << format->Rshift
               | (0x7F >> format->Gloss) << format->Gshift
               | (0x7F >> format->Bloss) << format->Bshift;
	}

	Uint32 operator()(Uint32 p, Uint32 v) const {
		return ((p>>1)&mask) + ((v >> 1)&mask);
	}

	Uint32 mask;
};


//the dummy variable is a hint for MSVC6, otherwise it compiles bad code
//because it cannot select between the 16 and 32 bit variants
template<typename PixelType, class Tinter, class Blender>
static void BlitTile_internal(SDL_Surface* target,
              int tx, int ty,
              int rx, int ry,
              int w, int h,
              const Uint8* data, const SDL_Color* pal,
              const Uint8* mask, Uint8 mask_key,
              Tinter& tint, Blender& blend, PixelType /*dummy*/=0)
{
	PixelType* buf_line = (PixelType*)(target->pixels) + (ty+ry)*(target->pitch / sizeof(PixelType));
	const Uint8* data_line = data + ry*64;

	PixelType opal[256];

	for (unsigned int i = 0; i < 256; ++i)
	{
		Uint8 r = tint.r(pal[i].r);
		Uint8 g = tint.g(pal[i].g);
		Uint8 b = tint.b(pal[i].b);
		opal[i] = (r >> target->format->Rloss) << target->format->Rshift
		                   | (g >> target->format->Gloss) << target->format->Gshift
		                   | (b >> target->format->Bloss) << target->format->Bshift;
	}

	if (mask) {
		const Uint8* mask_line = mask + ry*64;
		for (int y = 0; y < h; ++y) {
			PixelType* buf = buf_line + tx + rx;
			data = data_line + rx;
			mask = mask_line + rx;
			for (int x = 0; x < w; ++x) {
				Uint8 p = *data++;
				Uint8 m = *mask++;
				if (m == mask_key)
					*buf = (PixelType)blend(opal[p],*buf);
				buf++;
			}
			buf_line += target->pitch / sizeof(PixelType);
			mask_line += 64;
			data_line += 64;
		}

	} else {

		for (int y = 0; y < h; ++y) {
			PixelType* buf = buf_line + tx + rx;
			data = data_line + rx;
			for (int x = 0; x < w; ++x) {
				Uint8 p = *data++;
				*buf = (PixelType)blend(opal[p],*buf);
				buf++;
			}
			buf_line += target->pitch / sizeof(PixelType);
			data_line += 64;
		}

	}
}

