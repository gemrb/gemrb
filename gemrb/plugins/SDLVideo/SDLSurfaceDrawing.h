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

// clang-format off
// order matters
#include <SDL.h>
// clang-format on

#include "globals.h"

#include "Polygon.h"
#include "SDLPixelIterator.h"

#include "Logging/Logging.h"

#include <cmath>

namespace GemRB {

static_assert(std::is_trivially_destructible<Point>::value, "Expected Point to be trivially destructable.");

inline bool PointClipped(const SDL_Surface* surf, const BasePoint& p)
{
	if (p.x < 0 || p.x >= surf->w) {
		return true;
	}

	if (p.y < 0 || p.y >= surf->h) {
		return true;
	}

	return false;
}

#if SDL_VERSION_ATLEAST(1, 3, 0)
template<typename T>
inline const SDL_Rect& RectFromRegion(T& rgn)
{
	return reinterpret_cast<const SDL_Rect&>(rgn);
}
#else
inline SDL_Rect RectFromRegion(const Region& rgn)
{
	SDL_Rect rect = { Sint16(rgn.x), Sint16(rgn.y), Uint16(rgn.w), Uint16(rgn.h) };
	return rect;
}
#endif

template<SHADER SHADE = SHADER::NONE>
void DrawPointSurface(SDL_Surface* dst, BasePoint p, const Region& clip, const Color& color)
{
	assert(dst->format->BitsPerPixel == 32); // we could easily support others if we have to

	p = Clamp<BasePoint>(p, clip.origin, clip.Maximum());
	if (PointClipped(dst, p)) return;

	Uint32* px = ((Uint32*) dst->pixels) + (p.y * dst->pitch / 4) + p.x;

	if (SHADE != SHADER::NONE) {
		Color dstc;
		SDL_GetRGB(*px, dst->format, &dstc.r, &dstc.g, &dstc.b);
		if (SHADE == SHADER::TINT) {
			ShaderTint(color, dstc);
		} else {
			ShaderBlend<false>(color, dstc);
		}
		*px = SDL_MapRGBA(dst->format, dstc.r, dstc.g, dstc.b, dstc.a);
	} else {
		*px = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);
	}
}

template<SHADER SHADE = SHADER::NONE>
void DrawPointsSurface(SDL_Surface* surface, const std::vector<BasePoint>& points, const Region& clip, const Color& srcc)
{
	const SDL_PixelFormat* fmt = surface->format;
	SDL_LockSurface(surface);

	auto it = points.cbegin();
	for (; it != points.cend(); ++it) {
		auto p = *it;
		if (!clip.PointInside(p) || PointClipped(surface, p)) continue;

		unsigned char* start = static_cast<unsigned char*>(surface->pixels);
		unsigned char* dst = start + ((p.y * surface->pitch) + (p.x * fmt->BytesPerPixel));

		Color dstc;
		switch (fmt->BytesPerPixel) {
			case 1:
				if (SHADE != SHADER::NONE) {
					SDL_GetRGB(*dst, surface->format, &dstc.r, &dstc.g, &dstc.b);
					if (SHADE == SHADER::TINT) {
						ShaderTint(srcc, dstc);
					} else {
						ShaderBlend<false>(srcc, dstc);
					}
					*dst = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				} else {
					*dst = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
				}
				break;
			case 2:
				if (SHADE != SHADER::NONE) {
					SDL_GetRGB(*reinterpret_cast<Uint16*>(dst), surface->format, &dstc.r, &dstc.g, &dstc.b);
					if (SHADE == SHADER::TINT) {
						ShaderTint(srcc, dstc);
					} else {
						ShaderBlend<false>(srcc, dstc);
					}
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
				if (SHADE != SHADER::NONE) {
					SDL_GetRGB(*reinterpret_cast<Uint32*>(dst), surface->format, &dstc.r, &dstc.g, &dstc.b);
					if (SHADE == SHADER::TINT) {
						ShaderTint(srcc, dstc);
					} else {
						ShaderBlend<false>(srcc, dstc);
					}
					*reinterpret_cast<Uint32*>(dst) = SDL_MapRGB(surface->format, dstc.r, dstc.g, dstc.b);
				} else {
					*reinterpret_cast<Uint32*>(dst) = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
				}
				break;
			default:
				ERROR_UNKNOWN_BPP;
				break;
		}
	}

	SDL_UnlockSurface(surface);
}

template<SHADER SHADE = SHADER::NONE>
void DrawHLineSurface(SDL_Surface* dst, Point p, int x2, const Region& clip, const Color& color)
{
	assert(clip.x >= 0 && clip.w <= dst->w);
	assert(clip.y >= 0 && clip.h <= dst->h);
	assert(dst->format->BitsPerPixel == 32); // we could easily support others if we have to, but this is optimized for our needs

	if (p.y < clip.y || p.y >= clip.y + clip.h) {
		return;
	}

	if (x2 < p.x) {
		std::swap(x2, p.x);
	}

	if (p.x >= clip.x + clip.w) return;
	if (x2 < clip.x) return;

	if (p.x < clip.x) p.x = clip.x;
	x2 = Clamp<int>(x2, p.x, clip.x + clip.w);

	if (p.x == x2)
		return DrawPointSurface<SHADE>(dst, p, clip, color);

	assert(p.x < x2);
	if (p.y >= dst->h || p.x >= dst->w) {
		// when we are drawing stencils it is possible to get a dest that is smaller than the clip
		return;
	}

	if (SHADE != SHADER::NONE) {
		Region r = Region::RegionFromPoints(p, Point(x2, p.y));
		r.h = 1;
		auto dstit = MakeSDLPixelIterator(dst, r.Intersect(clip));
		auto dstend = SDLPixelIterator::end(dstit);

		if (SHADE == SHADER::TINT) {
			const static TintDst<false> blender;
			ColorFill(color, dstit, dstend, blender);
		} else {
			const static OneMinusSrcA<false, false> blender;
			ColorFill(color, dstit, dstend, blender);
		}
	} else {
		Uint32* px = ((Uint32*) dst->pixels) + (p.y * dst->pitch / 4) + p.x;
		Uint32 c = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);

		int numPx = std::min(x2 - p.x, dst->w - p.x);
		std::fill_n(px, numPx, c);
	}
}

template<SHADER SHADE = SHADER::NONE>
inline void DrawVLineSurface(SDL_Surface* dst, Point p, int y2, const Region& clip, const Color& color)
{
	assert(clip.x >= 0 && clip.w <= dst->w);
	assert(clip.y >= 0 && clip.h <= dst->h);

	if (p.x < clip.x || p.x >= clip.x + clip.w) {
		return;
	}

	if (y2 < p.y) {
		std::swap(y2, p.y);
	}

	if (p.y >= clip.y + clip.h) return;
	if (y2 < clip.y) return;

	if (p.y < clip.y) p.y = clip.y;
	y2 = Clamp<int>(y2, p.y, clip.y + clip.h);

	if (p.y == y2)
		return DrawPointSurface<SHADE>(dst, p, clip, color);

	Region r = Region::RegionFromPoints(p, Point(p.x, y2));
	r.w = 1;
	auto dstit = MakeSDLPixelIterator(dst, r.Intersect(clip));
	auto dstend = SDLPixelIterator::end(dstit);

	if (SHADE == SHADER::BLEND) {
		const static OneMinusSrcA<false, false> blender;
		ColorFill(color, dstit, dstend, blender);
	} else if (SHADE == SHADER::TINT) {
		const static TintDst<false> blender;
		ColorFill(color, dstit, dstend, blender);
	} else {
		const static SrcRGBA<false> blender;
		ColorFill(color, dstit, dstend, blender);
	}
}

template<SHADER SHADE = SHADER::NONE>
void DrawLineSurface(SDL_Surface* surface, const BasePoint& start, const BasePoint& end, const Region& clip, const Color& color)
{
	if (start.y == end.y) return DrawHLineSurface<SHADE>(surface, { start.x, start.y }, end.x, clip, color);
	if (start.x == end.x) return DrawVLineSurface<SHADE>(surface, { start.x, start.y }, end.y, clip, color);

	assert(clip.x >= 0 && clip.w <= surface->w);
	assert(clip.y >= 0 && clip.h <= surface->h);

	auto p1 = start;
	auto p2 = end;

	bool yLonger = false;
	int shortLen = p2.y - p1.y;
	int longLen = p2.x - p1.x;
	if (std::abs(shortLen) > std::abs(longLen)) {
		std::swap(shortLen, longLen);
		yLonger = true;
	}

	int decInc;
	if (longLen == 0) {
		decInc = 0;
	} else {
		decInc = (shortLen * 65536) / longLen;
	}

	// the numper of "points" is the length of the hypotenuse
	// however, since we are only aproximating a straight line (because pixels)
	// and sqrts are expensive and mallocs larger mallocs arent more expensive than smaller ones
	// we will just overestimate by reserving shortLen + longLen Points
	// we continually recycle this vector
	// this prevents constant allocation/deallocation
	// while drawing. Yes, its permanently used memory, but we do
	// enough drawing that this is not a problem (its a tiny amount anyway)
	// Point is trivial and clear() should be constant
	static std::vector<BasePoint> s_points;
	s_points.clear();
	s_points.reserve(std::abs(longLen) + std::abs(shortLen));
	Point newp;

	do { // TODO: rewrite without loop
		if (yLonger) {
			if (longLen > 0) {
				longLen += p1.y;
				for (int j = 0x8000 + (p1.x << 16); p1.y <= longLen; ++p1.y) {
					newp = Point(j >> 16, p1.y);
					if (clip.PointInside(newp))
						s_points.push_back(newp);
					j += decInc;
				}
				break;
			}
			longLen += p1.y;
			for (int j = 0x8000 + (p1.x << 16); p1.y >= longLen; --p1.y) {
				newp = Point(j >> 16, p1.y);
				if (clip.PointInside(newp))
					s_points.push_back(newp);
				j -= decInc;
			}
			break;
		}

		if (longLen > 0) {
			longLen += p1.x;
			for (int j = 0x8000 + (p1.y << 16); p1.x <= longLen; ++p1.x) {
				newp = Point(p1.x, j >> 16);
				if (clip.PointInside(newp))
					s_points.push_back(newp);
				j += decInc;
			}
			break;
		}
		longLen += p1.x;
		for (int j = 0x8000 + (p1.y << 16); p1.x >= longLen; --p1.x) {
			newp = Point(p1.x, j >> 16);
			if (clip.PointInside(newp))
				s_points.push_back(newp);
			j -= decInc;
		}
	} while (false);

	DrawPointsSurface<SHADE>(surface, s_points, clip, color);
}

template<SHADER SHADE = SHADER::NONE>
void DrawLinesSurface(SDL_Surface* surface, const std::vector<Point>& points, const Region& clip, const Color& color)
{
	size_t count = points.size();
	assert(count % 2 == 0);
	for (size_t i = 0; i < count; i += 2) {
		DrawLineSurface<SHADE>(surface, points[i], points[i + 1], clip, color);
	}
}

template<SHADER SHADE = SHADER::NONE>
void DrawPolygonSurface(SDL_Surface* surface, const Gem_Polygon* poly, const Point& origin, const Region& clip, const Color& color, bool fill)
{
	if (fill) {
		for (const auto& lineSegments : poly->rasterData) {
			for (const auto& segment : lineSegments) {
				DrawHLineSurface<SHADE>(surface, segment.first + origin, (segment.second + origin).x, clip, color);
			}
		}
	} else {
		// we continually recycle this vector
		// this prevents constant allocation/deallocation
		// while drawing. Yes, its permanently used memory, but we do
		// enough drawing that this is not a problem (its a tiny amount anyway)
		// Point is trivial and clear() should be constant
		static std::vector<Point> s_points;
		s_points.clear();
		s_points.resize(poly->Count() * 2); // resize, not reserve! (it won't shrink the capacity FYI)

		const Point& p = poly->vertices[0] - poly->BBox.origin + origin;
		s_points[0].x = p.x;
		s_points[0].y = p.y;

		size_t j = 1;
		for (size_t i = 1; i < poly->Count(); ++i, j += 2) {
			// this is not a typo. one point ends the previous line, the next begins the next line
			const Point& v = poly->vertices[i] - poly->BBox.origin + origin;
			s_points[j].x = v.x;
			s_points[j].y = v.y;
			s_points[j + 1] = s_points[j];
		}
		// reconnect with start point
		s_points[j].x = p.x;
		s_points[j].y = p.y;

		DrawLinesSurface<SHADE>(surface, s_points, clip, color);
	}
}

}

#endif /* SURFACE_DRAWING_H */
