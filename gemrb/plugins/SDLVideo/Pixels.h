/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2018 The GemRB Project
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

namespace GemRB {

struct RGBBlender {
	virtual void operator()(const Color& src, Color& dst, Uint8 mask) const=0;
};

/*
struct RGBBlend : RGBBlender {
	void operator()(const Color& src, Color& dst, Uint8 mask) const {

	}
};

struct RGBTint : RGBBlender {
	Color tint;

	RGBTint(const Color& c) : tint(c) {}

	void operator()(const Color& src, Color& dst, Uint8 mask) const {

	}
};

struct RGBGreyscale : RGBBlender {
	void operator()(const Color& src, Color& dst, Uint8 mask) const {

	}
};

struct RGBSepia : RGBBlender {
	void operator()(const Color& src, Color& dst, Uint8 mask) const {

	}
};
*/

inline void ShaderTint(const Color& tint, Color& c) {
	c.r = (tint.r * c.r) >> 8;
	c.g = (tint.g * c.g) >> 8;
	c.b = (tint.b * c.b) >> 8;
}

inline void ShaderGreyscale(Color& c) {
	c.r >>= 2;
	c.g >>= 2;
	c.b >>= 2;
	Uint8 avg = c.r + c.g + c.b;
	c.r = c.g = c.b = avg;
}

inline void ShaderSepia(Color& c) {
	c.r >>= 2;
	c.g >>= 2;
	c.b >>= 2;
	Uint8 avg = c.r + c.g + c.b;
	c.r = avg + 21; // can't overflow, since a is at most 189
	c.g = avg;
	c.b = avg < 32 ? 0 : avg - 32;
}

template <bool ALPHA>
inline void ShaderBlend(const Color& src, Color& dst) {
	// macro for a fast approximation for division by 255 (more accurate than >> 8)
	// TODO: this is probably only faster on old CPUs, should #ifdef on arch
#define DIV255(x) ((x + 1 + (x>>8)) >> 8) //((x)/255)
	dst.r = DIV255(src.a * src.r) + DIV255((255 - src.a) * dst.r);
	dst.g = DIV255(src.a * src.g) + DIV255((255 - src.a) * dst.g);
	dst.b = DIV255(src.a * src.b) + DIV255((255 - src.a) * dst.b);
#undef DIV255
	if (ALPHA)
		dst.a = src.a + dst.a * (255 - src.a);
}

enum SHADER {
	NONE,
	TINT,
	GREYSCALE,
	SEPIA
};

// using a template to avoid runtime branch evaluation
// by optimizing down to a single case
template <SHADER SHADE, bool SRCALPHA>
class RGBBlendingPipeline : RGBBlender {
	Color tint;
	unsigned int shift;

public:
	RGBBlendingPipeline()
	: tint(1,1,1,0xff) {
		shift = 0;
		if (SHADE == GREYSCALE || SHADE == SEPIA) {
			shift += 2;
		}
	}

	RGBBlendingPipeline(const Color& tint)
	: tint(tint) {
		shift = 8; // we shift by 8 as a fast aproximation of dividing by 255
		if (SHADE == GREYSCALE || SHADE == SEPIA) {
			shift += 2;
		}
	}

	void operator()(const Color& src, Color& dst, Uint8 mask) const {
		Color c = src;
		c.a = (255-mask) + (c.a * mask); // FIXME: not sure this is 100% correct, but it passes my known tests

		if (c.a == 0) {
			return;
		}

		// first apply "shader"
		switch (SHADE) {
			case TINT:
				assert(shift);
				c.r = (tint.r * c.r) >> shift;
				c.g = (tint.g * c.g) >> shift;
				c.b = (tint.b * c.b) >> shift;
				break;
			case GREYSCALE:
				{
					c.r = (tint.r * c.r) >> shift;
					c.g = (tint.g * c.g) >> shift;
					c.b = (tint.b * c.b) >> shift;
					Uint8 avg = c.r + c.g + c.b;
					c.r = c.g = c.b = avg;
				}
				break;
			case SEPIA:
				{
					c.r = (tint.r * c.r) >> shift;
					c.g = (tint.g * c.g) >> shift;
					c.b = (tint.b * c.b) >> shift;
					Uint8 avg = c.r + c.g + c.b;
					c.r = avg + 21; // can't overflow, since a is at most 189
					c.g = avg;
					c.b = avg < 32 ? 0 : avg - 32;
				}
				break;
			case NONE:
				// fallthough
			default:
				break;
		}

		ShaderBlend<SRCALPHA>(c, dst);
	}
};

struct IPixelIterator
{
	enum Direction {
		Reverse = -1,
		Forward = 1
	};

	void* pixel;
	//int offset; // current pixel distance from 'pixel'
	int pitch; // in bytes

	Direction xdir;
	Direction ydir;

	IPixelIterator(void* px, int pitch, Direction x, Direction y)
	: pixel(px), pitch(pitch), xdir(x), ydir(y) {}

	virtual ~IPixelIterator() {};

	virtual IPixelIterator* Clone() const=0;
	virtual void Advance(int)=0;

	IPixelIterator& operator++() {
		Advance(1);
		return *this;
	}

	bool operator!=(const IPixelIterator& rhs) const {
		return pixel != rhs.pixel;
	}
};

template <typename PIXEL>
struct PixelIterator : IPixelIterator
{
	int w, xpos;

	PixelIterator(PIXEL* p, int w, int pitch)
	: IPixelIterator(p, pitch, Forward, Forward), w(w) {
		assert(pitch >= w);
		xpos = 0;
	}

	PixelIterator(PIXEL* p, Direction x, Direction y, int w, int pitch)
	: IPixelIterator(p, pitch, x, y), w(w) {
		assert(pitch >= w);
		xpos = (x == Reverse) ? w-1 : 0;
	}

	virtual PIXEL& operator*() const {
		return *static_cast<PIXEL*>(pixel);
	}

	virtual PIXEL* operator->() {
		return static_cast<PIXEL*>(pixel);
	}

	IPixelIterator* Clone() const {
		return new PixelIterator<PIXEL>(*this);
	}

	void Advance(int dx) {
		Uint8* ptr = static_cast<Uint8*>(pixel);
		int amt = xdir * dx;
		int tmp = xpos + amt;
		// FIXME: if |amt| is > w things will blow up
		if (tmp < 0) {
			assert((xdir == Reverse && dx > 0) || (xdir == Forward && dx < 0));

			xpos = w + tmp;
			ptr += pitch * ydir; // shift rows
			ptr += (w + amt) * sizeof(PIXEL);
		} else if (tmp >= w) {
			assert((xdir == Forward && dx > 0) || (xdir == Reverse && dx < 0));

			xpos = tmp - w;
			ptr += pitch * ydir; // shift rows
			ptr += int((amt - w) * sizeof(PIXEL));
		} else {
			xpos += amt;
			ptr += amt * sizeof(PIXEL);
		}

		pixel = ptr;
	}
};

struct IColorIterator
{
	virtual const Color& operator*() const=0;
	virtual const Color* operator->() const=0;

	virtual IColorIterator& operator++()=0;
};

struct PaletteIterator : public IColorIterator
{
	Color* pal;
	Uint8* pixel;

	PaletteIterator(Color* c, Uint8* p) {
		assert(c && p);
		pal = c;
		pixel = p;
	}

	const Color& operator*() const {
		return pal[*pixel];
	}

	const Color* operator->() const {
		return &pal[*pixel];
	}

	IColorIterator& operator++() {
		++pixel;
		return *this;
	}
};

// an endless iterator that always returns 'color' when dereferenced
struct StaticIterator : public IColorIterator
{
	Color color;

	StaticIterator(const Color& c) : color(c) {}

	const Color& operator*() const {
		return color;
	}

	const Color* operator->() const {
		return &color;
	}

	IColorIterator& operator++() {
		return *this;
	}
};

struct SDLPixelIterator : IPixelIterator
{
//private:
	IPixelIterator* imp;

	void InitImp(void* pixel, int pitch, int bpp) {
		switch (bpp) {
			case 4:
				imp = new PixelIterator<Uint32>(static_cast<Uint32*>(pixel), xdir, ydir, clip.w, pitch);
				break;
			case 3:
				assert(false); // FIXME: could handle this with a packed struct
				break;
			case 2:
				imp = new PixelIterator<Uint16>(static_cast<Uint16*>(pixel), xdir, ydir, clip.w, pitch);
				break;
			case 1:
				imp = new PixelIterator<Uint8>(static_cast<Uint8*>(pixel), xdir, ydir, clip.w, pitch);
				break;
			default:
				assert(false);
				break;
		}
	}

	inline static Uint8* FindStart(Uint8* pixels, int pitch, int bpp, const SDL_Rect& clip, Direction xdir, Direction ydir) {
		if (xdir == Reverse) {
			pixels += bpp * (clip.w-1);
		}
		if (ydir == Reverse) {
			pixels += pitch * (clip.h-1);
		}

		pixels += (clip.y * pitch) + (clip.x * bpp);
		return pixels;
	}

public:
	SDL_PixelFormat* format;
	SDL_Rect clip;

	SDLPixelIterator(const SDL_Rect& clip, SDL_Surface* surf)
	: IPixelIterator(NULL, surf->pitch, Forward, Forward), imp(NULL), format(surf->format), clip(clip)
	{
		Uint8* pixels = static_cast<Uint8*>(surf->pixels);
		pixels = FindStart(pixels, surf->pitch, format->BytesPerPixel, clip, xdir, ydir);

		InitImp(pixels, surf->pitch, surf->format->BytesPerPixel);

		pixel = surf->pixels;
	}

	SDLPixelIterator(Direction x, Direction y, const SDL_Rect& clip, SDL_Surface* surf)
	: IPixelIterator(NULL, surf->pitch, x, y), format(surf->format), clip(clip)
	{
		Uint8* pixels = static_cast<Uint8*>(surf->pixels);
		pixels = FindStart(pixels, surf->pitch, format->BytesPerPixel, clip, xdir, ydir);

		InitImp(pixels, surf->pitch, surf->format->BytesPerPixel);

		pixel = surf->pixels; // always here regardless of direction
	}

	SDLPixelIterator(const SDLPixelIterator& orig)
	: IPixelIterator(orig)
	{
		clip = orig.clip;
		format = orig.format;
		imp = orig.imp->Clone();
	}

	~SDLPixelIterator() {
		delete imp;
	}
/*
	template <typename PIXEL>
	operator PixelIterator<PIXEL>() const {
		assert(format->BytesPerPixel == sizeof(PIXEL));
		return *static_cast<const PixelIterator<PIXEL>*>(imp);
	}

	template <typename PIXEL>
	PixelIterator<PIXEL> Imp() const {
		assert(format->BytesPerPixel == sizeof(PIXEL));
		return *static_cast<const PixelIterator<PIXEL>*>(imp);
	}
*/
	static SDLPixelIterator end(const SDLPixelIterator& beg)
	{
		Direction xdir = (beg.xdir == Forward) ? Reverse : Forward;
		Direction ydir = (beg.ydir == Forward) ? Reverse : Forward;
		SDLPixelIterator it(beg);

		Uint8* pixels = static_cast<Uint8*>(it.pixel);
		pixels = FindStart(pixels, beg.pitch, beg.format->BytesPerPixel, beg.clip, xdir, ydir);
		it.xdir = xdir; // hack for InitImp
		it.InitImp(pixels, beg.pitch, beg.format->BytesPerPixel);
		it.xdir = beg.xdir; // reset for Advance
		it.imp->xdir = beg.xdir;

		// 'end' iterators are one past the end
		it.Advance(1);
		return it;
	}

	SDLPixelIterator& operator++() {
		imp->Advance(1);
		return *this;
	}

	bool operator!=(const SDLPixelIterator& rhs) const {
		return imp->pixel != rhs.imp->pixel;
	}

	Uint8& operator*() const {
		return *static_cast<Uint8*>(imp->pixel);
	}

	Uint8* operator->() const {
		return static_cast<Uint8*>(imp->pixel);
	}

	IPixelIterator* Clone() const {
		return new SDLPixelIterator(*this);
	}

	void Advance(int amt) {
		imp->Advance(amt);
	}

	void ReadRGBA(Uint8& r, Uint8& g, Uint8& b, Uint8& a) const {
		Uint32 pixel = 0;
		switch (format->BytesPerPixel) {
			case 4:
				pixel = *reinterpret_cast<Uint32*>(imp->pixel);
				break;
			case 3:
				assert(false); // FIXME: could handle this
				break;
			case 2:
				pixel = *reinterpret_cast<Uint16*>(imp->pixel);
				break;
			case 1:
				pixel = *reinterpret_cast<Uint8*>(imp->pixel);
				r = format->palette->colors[pixel].r;
				g = format->palette->colors[pixel].g;
				b = format->palette->colors[pixel].b;

#if SDL_VERSION_ATLEAST(1,3,0)
				// FIXME: I guess we dont support this (no surface to query)... do we need it?
				//Uint32 ck;
				//if (SDL_GetColorKey(surface, &ck) != -1 && ck == pixel) c.a = SDL_ALPHA_TRANSPARENT;
				a = SDL_ALPHA_OPAQUE;
#else
				if (format->colorkey == pixel) a = SDL_ALPHA_TRANSPARENT;
				else a = SDL_ALPHA_OPAQUE;
#endif
				return;
			default:
				assert(false);
				break;
		}

		unsigned v;
		v = (pixel & format->Rmask) >> format->Rshift;
		r = (v << format->Rloss) + (v >> (8 - (format->Rloss << 1)));
		v = (pixel & format->Gmask) >> format->Gshift;
		g = (v << format->Gloss) + (v >> (8 - (format->Gloss << 1)));
		v = (pixel & format->Bmask) >> format->Bshift;
		b = (v << format->Bloss) + (v >> (8 - (format->Bloss << 1)));
		if(format->Amask) {
			v = (pixel & format->Amask) >> format->Ashift;
			a = (v << format->Aloss) + (v >> (8 - (format->Aloss << 1)));
		} else {
			a = SDL_ALPHA_OPAQUE;
		}
	}

	void WriteRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
		if (format->BytesPerPixel == 1) {
			Uint32 pixel = SDL_MapRGBA(format, r, g, b, a);
			*reinterpret_cast<Uint8*>(imp->pixel) = pixel;
			return;
		}

		Uint32 pixel = (r >> format->Rloss) << format->Rshift
		| (g >> format->Gloss) << format->Gshift
		| (b >> format->Bloss) << format->Bshift
		| ((a >> format->Aloss) << format->Ashift & format->Amask);

		switch (format->BytesPerPixel) {
			case 4:
				*reinterpret_cast<Uint32*>(imp->pixel) = pixel;
				break;
			case 3:
				assert(false); // FIXME: could handle this
				break;
			case 2:
				*reinterpret_cast<Uint16*>(imp->pixel) = pixel;
				break;
			default:
				assert(false);
				break;
		}
	}
};

template<class BLENDER>
static void Blit(SDLPixelIterator src,
				 SDLPixelIterator dst, SDLPixelIterator dstend,
				 IColorIterator& mask,
				 const BLENDER& blender)
{
	for (; dst != dstend; ++dst, ++src, ++mask) {
		assert(dst.imp->pixel < dstend.imp->pixel);
		Color srcc, dstc;
		src.ReadRGBA(srcc.r, srcc.g, srcc.b, srcc.a);
		dst.ReadRGBA(dstc.r, dstc.g, dstc.b, dstc.a);

		blender(srcc, dstc, mask->a);

		dst.WriteRGBA(dstc.r, dstc.g, dstc.b, dstc.a);
	}
}

template <typename BLENDER>
static void BlitBlendedRect(SDL_Surface* src, SDL_Surface* dst,
							const SDL_Rect& srcrgn, const SDL_Rect& dstrgn,
							BLENDER blender, Uint32 flags, SDL_Surface* mask)
{
	assert(src && dst);
	assert(srcrgn.h == dstrgn.h && srcrgn.w == dstrgn.w);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	SDLPixelIterator::Direction xdir = (flags&BLIT_MIRRORX) ? SDLPixelIterator::Reverse : SDLPixelIterator::Forward;
	SDLPixelIterator::Direction ydir = (flags&BLIT_MIRRORY) ? SDLPixelIterator::Reverse : SDLPixelIterator::Forward;

	SDLPixelIterator dstbeg(SDLPixelIterator::Forward, SDLPixelIterator::Forward, dstrgn, dst);
	SDLPixelIterator dstend = SDLPixelIterator::end(dstbeg);
	SDLPixelIterator srcbeg(xdir, ydir, srcrgn, src);

	if (mask) {
		SDL_LockSurface(mask);
		PaletteIterator alpha(reinterpret_cast<Color*>(mask->format->palette->colors), static_cast<Uint8*>(mask->pixels));
		Blit(srcbeg, dstbeg, dstend, alpha, blender);
		SDL_UnlockSurface(mask);
	} else {
		StaticIterator alpha(Color(0,0,0,0));
		Blit(srcbeg, dstbeg, dstend, alpha, blender);
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
}

}
