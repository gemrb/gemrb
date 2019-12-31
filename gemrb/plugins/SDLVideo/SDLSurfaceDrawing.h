/* GemRB - Infinity Engine Emulator
* Copyright (C) 2019 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*
*/

#ifndef SURFACE_DRAWING_H
#define SURFACE_DRAWING_H

#include <SDL.h>

#include "Pixels.h"
#include "Polygon.h"

namespace GemRB {

inline bool PointClipped(SDL_Surface* surf, const Point& p)
{
	if (p.x < 0 || p.x > surf->w) {
		return true;
	}

	if (p.y < 0 || p.y > surf->h) {
		return true;
	}

	return false;
}

#if SDL_VERSION_ATLEAST(1,3,0)
template<typename T>
inline const SDL_Rect& RectFromRegion(T&& rgn)
{
	return reinterpret_cast<const SDL_Rect&>(rgn);
}
#else
inline SDL_Rect RectFromRegion(const Region& rgn)
{
	SDL_Rect rect = {Sint16(rgn.x), Sint16(rgn.y), Uint16(rgn.w), Uint16(rgn.h)};
	return rect;
}
#endif

template<bool BLENDED=false>
void DrawPointSurface(SDL_Surface* dst, Point p, const Region& clip, const Color& color)
{
	assert(dst->format->BitsPerPixel == 32); // we could easily support others if we have to

	p = Clamp(p, clip.Origin(), clip.Maximum());
	if (PointClipped(dst, p)) return;

	Uint32* px = ((Uint32*)dst->pixels) + p.y * dst->pitch + p.x;

	if (BLENDED) {
		Color dstc;
		SDL_GetRGB( *px, dst->format, &dstc.r, &dstc.g, &dstc.b );
		ShaderBlend<false>(color, dstc);
		*px = SDL_MapRGBA(dst->format, dstc.r, dstc.g, dstc.b, dstc.a);
	} else {
		*px = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);
	}
}

template<bool BLENDED=false>
void DrawPointsSurface(SDL_Surface* surface, const std::vector<Point>& points, const Region& clip, const Color& srcc)
{
	SDL_PixelFormat* fmt = surface->format;
	SDL_LockSurface( surface );

	std::vector<Point>::const_iterator it;
	it = points.begin();
	for (; it != points.end(); ++it) {
		Point p = Clamp(*it, clip.Origin(), clip.Maximum());
		if (PointClipped(surface, p)) continue;

		unsigned char* start = static_cast<unsigned char*>(surface->pixels);
		unsigned char* dst = start + ((p.y * surface->pitch) + (p.x * fmt->BytesPerPixel));

		Color dstc;
		switch (fmt->BytesPerPixel) {
			case 1:
				if (BLENDED) {
					SDL_GetRGB( *dst, surface->format, &dstc.r, &dstc.g, &dstc.b );
					ShaderBlend<false>(srcc, dstc);
					*dst = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				} else {
					*dst = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
				}
				break;
			case 2:
				if (BLENDED) {
					SDL_GetRGB( *reinterpret_cast<Uint16*>(dst), surface->format, &dstc.r, &dstc.g, &dstc.b );
					ShaderBlend<false>(srcc, dstc);
					*reinterpret_cast<Uint16*>(dst) = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				} else {
					*reinterpret_cast<Uint16*>(dst) = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
				}
				break;
			case 3:
			{
				// FIXME: implement alpha blending for this... or nix it
				// is this even used?
				/*
				Uint32 val = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				pixel[0] = val & 0xff;
				pixel[1] = (val >> 8) & 0xff;
				pixel[2] = (val >> 16) & 0xff;
	#else
				pixel[2] = val & 0xff;
				pixel[1] = (val >> 8) & 0xff;
				pixel[0] = (val >> 16) & 0xff;
	#endif
				*/
			}
				break;
			case 4:
				if (BLENDED) {
					SDL_GetRGB( *reinterpret_cast<Uint32*>(dst), surface->format, &dstc.r, &dstc.g, &dstc.b );
					ShaderBlend<false>(srcc, dstc);
					*reinterpret_cast<Uint32*>(dst) = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				} else {
					*reinterpret_cast<Uint32*>(dst) = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
				}
				break;
			default:
				Log(ERROR, "sprite_t", "Working with unknown pixel format: %s", SDL_GetError());
				break;
		}
	}

	SDL_UnlockSurface( surface );
}

template<bool BLENDED=false>
void DrawHLineSurface(SDL_Surface* dst, Point p, int x2, const Region& clip, const Color& color)
{
	assert(dst->format->BitsPerPixel == 32); // we could easily support others if we have to, but this is optimized for our needs

	assert(p.x < x2);
	p.x = Clamp<int>(p.x, clip.x, clip.x + clip.w);
	p.y = Clamp<int>(p.y, clip.y, clip.y + clip.h);

	if (p.y < 0 || p.y > dst->h) return;
	if (p.x < 0 || x2 > dst->w) return;

	if (p.x == x2)
		return DrawPointSurface<false>(dst, p, clip, color);

	if (BLENDED) {
		Region r = Region::RegionFromPoints(p, Point(x2, p.y));
		SDLPixelIterator dstit(RectFromRegion(r.Intersect(clip)), dst);
		SDLPixelIterator dstend = SDLPixelIterator::end(dstit);
		static StaticIterator alpha(Color(0,0,0,0));
		const static OneMinusSrcA<false, false> blender;

		WriteColor(color, dstit, dstend, alpha, blender);
	} else {
		Uint32* row = (Uint32*)dst->pixels + (dst->pitch/4 * p.y);
		Uint32 c = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);

		int numPx = std::min(x2 - p.x, dst->w - p.x);
		SDL_memset4(row + p.x, c, numPx);
	}
}

template<bool BLENDED=false>
inline void DrawVLineSurface(SDL_Surface* dst, const Point& p, int y2, const Region& clip, const Color& color)
{
	Region r = Region::RegionFromPoints(p, Point(p.x, y2));
	SDLPixelIterator dstit(RectFromRegion(r.Intersect(clip)), dst);
	SDLPixelIterator dstend = SDLPixelIterator::end(dstit);
	static StaticIterator alpha(Color(0,0,0,0));

	if (BLENDED) {
		const static OneMinusSrcA<false, false> blender;
		WriteColor(color, dstit, dstend, alpha, blender);
	} else {
		const static SrcRGBA<false> blender;
		WriteColor(color, dstit, dstend, alpha, blender);
	}
}

template<bool BLENDED=false>
void DrawLineSurface(SDL_Surface* surface, const Point& start, const Point& end, const Region& clip, const Color& color)
{
	if (start.y == end.y) return DrawHLineSurface<BLENDED>(surface, start, end.x, clip, color);
	if (start.x == end.x) return DrawVLineSurface<BLENDED>(surface, start, end.y, clip, color);

	// clamp the points to clip
	Point min = clip.Origin();
	Point max = min + Point(clip.w - 1, clip.h - 1);
	Point p1 = Clamp(start, min, max);
	Point p2 = Clamp(end, min, max);

	bool yLonger = false;
	int shortLen = p2.y - p1.y;
	int longLen = p2.x - p1.x;
	if (abs( shortLen ) > abs( longLen )) {
		std::swap(shortLen, longLen);
		yLonger = true;
	}

	int decInc;
	if (longLen == 0) {
		decInc = 0;
	} else {
		decInc = ( shortLen * 65536 ) / longLen;
	}

	std::vector<Point> points;

	do { // TODO: rewrite without loop
		if (yLonger) {
			if (longLen > 0) {
				longLen += p1.y;
				for (int j = 0x8000 + ( p1.x << 16 ); p1.y <= longLen; ++p1.y) {
					points.push_back(Point( j >> 16, p1.y ));
					j += decInc;
				}
				break;
			}
			longLen += p1.y;
			for (int j = 0x8000 + ( p1.x << 16 ); p1.y >= longLen; --p1.y) {
				points.push_back(Point( j >> 16, p1.y ));
				j -= decInc;
			}
			break;
		}

		if (longLen > 0) {
			longLen += p1.x;
			for (int j = 0x8000 + ( p1.y << 16 ); p1.x <= longLen; ++p1.x) {
				points.push_back(Point( p1.x, j >> 16 ));
				j += decInc;
			}
			break;
		}
		longLen += p1.x;
		for (int j = 0x8000 + ( p1.y << 16 ); p1.x >= longLen; --p1.x) {
			points.push_back(Point( p1.x, j >> 16 ));
			j -= decInc;
		}
	} while (false);

	DrawPointsSurface<BLENDED>(surface, points, clip, color);
}

template<bool BLENDED=false>
void DrawLinesSurface(SDL_Surface* surface, const std::vector<Point>& points, const Region& clip, const Color& color)
{
	size_t count = points.size();
	assert(count % 2 == 0);
	for (size_t i = 0; i < count; i+=2)
	{
		DrawLineSurface<BLENDED>(surface, points[i], points[i+1], clip, color);
	}
}

template<bool BLENDED=false>
void DrawPolygonSurface(SDL_Surface* surface, Gem_Polygon* poly, const Point& origin, const Region& clip, const Color& color, bool fill)
{
	if (fill) {
		const std::vector<Point>& lines = poly->rasterData;
		size_t count = lines.size();
		for (size_t i = 0; i < count; i+=2)
		{
			DrawHLineSurface<BLENDED>(surface, lines[i] + origin, (lines[i+1] + origin).x, clip, color);
		}
	} else {
		std::vector<Point> points(poly->Count()*2);

		const Point& p = poly->vertices[0] - origin;
		points[0].x = p.x;
		points[0].y = p.y;

		size_t j = 1;
		for (size_t i = 1; i < poly->Count(); ++i, ++j) {
			// this is not a typo. one point ends the previous line, the next begins the next line
			const Point& p = poly->vertices[i] - origin;
			points[j].x = p.x;
			points[j].y = p.y;
			points[++j] = points[i];
		}
		// reconnect with start point
		points[j].x = p.x;
		points[j].y = p.y;

		DrawLinesSurface<BLENDED>(surface, points, clip, color);
	}
}

}

#endif /* SURFACE_DRAWING_H */
