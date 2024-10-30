/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef FOG_RENDERER_H
#define FOG_RENDERER_H

#include "exports.h"
#include "globals.h"

#include "Bitmap.h"
#include "EnumIndex.h"
#include "Region.h"
#include "Sprite2D.h"

#include <vector>

namespace GemRB {

struct FogMapData {
	FogMapData(
		const Bitmap* exploredMask,
		const Bitmap* visibleMask,
		const Region& vp,
		const Size& mapSize,
		const Size& fogSize,
		int largeFog)
		: exploredMask(exploredMask),
		  visibleMask(visibleMask),
		  vp(vp),
		  mapSize(mapSize),
		  fogSize(fogSize),
		  largeFog(largeFog)
	{}

	const Bitmap* exploredMask;
	const Bitmap* visibleMask;
	Region vp;
	Size mapSize;
	Size fogSize;
	int largeFog;
};

class FogPoint : public BasePoint {
public:
	using BasePoint::BasePoint;
	explicit FogPoint(const Point& p)
	{
		x = p.x / 32; // FogRenderer::CELL_SIZE
		y = p.y / 32; // FogRenderer::CELL_SIZE
	}

	FogPoint operator+(const FogPoint& p) const noexcept
	{
		return FogPoint(x + p.x, y + p.y);
	}
};

class GEM_EXPORT FogRenderer {
private:
	bool videoCanRenderGeometry = false;
	std::vector<float> fogVertices;
	std::vector<Color> fogColors;

	Region vp;
	Size mapSize;
	FogPoint start;
	FogPoint end;
	Point p0;

	enum class Direction : uint8_t {
		O,
		N = 1,
		W = 2,
		NW = N | W, // 3
		S = 4,
		SW = S | W, // 6
		E = 8,
		NE = N | E, // 9
		SE = S | E, // 12
		count
	};

	// Size of Fog-Of-War shadow tile (and bitmap)
	static constexpr int CELL_SIZE = 32;
	static constexpr EnumArray<Direction, BlitFlags> BAM_FLAGS {
		BlitFlags::NONE, BlitFlags::NONE, BlitFlags::NONE, BlitFlags::NONE,
		BlitFlags::MIRRORY, BlitFlags::NONE, BlitFlags::MIRRORY,
		BlitFlags::NONE, BlitFlags::MIRRORX, BlitFlags::MIRRORX,
		BlitFlags::NONE, BlitFlags::NONE, BlitFlags::MIRRORX | BlitFlags::MIRRORY
	};

	static constexpr BlitFlags OPAQUE_FOG = BlitFlags::NONE;
	static constexpr BlitFlags TRANSPARENT_FOG = BlitFlags::HALFTRANS | BlitFlags::BLENDED;

	static EnumArray<Direction, Holder<Sprite2D>> LoadFogSprites();
	EnumArray<Direction, Holder<Sprite2D>> fogSprites;

public:
	explicit FogRenderer(bool doBAMRendering = false);

	void DrawFog(const FogMapData& mapData);

private:
	Point ConvertFogPointToScreen(int x, int y) const;
	void DrawExploredCell(FogPoint cellPoint, const Bitmap* mask);
	void DrawFogCellBAM(Point p, Direction direction, BlitFlags flags);
	void DrawFogCellVertices(Point p, Direction direction, BlitFlags flags);
	bool DrawFogCellByDirection(Point p, Direction direction, BlitFlags flags);
	bool DrawFogCellByDirectionBAMs(Point p, Direction direction, BlitFlags flags);
	bool DrawFogCellByDirectionVertices(Point p, Direction direction, BlitFlags flags);
	void DrawFogSmoothing(Point p, Direction direction, BlitFlags flags, Direction adjacentDir);
	void DrawVisibleCell(FogPoint cellPoint, const Bitmap* mask);
	void DrawVPBorder(Point p, Direction direction, const Region& r, BlitFlags flags);
	void DrawVPBorders();
	void FillFog(Point p, int numRowItems, BlitFlags flags);
	static bool IsUncovered(FogPoint cellPoint, const Bitmap* mask);
	void SetFogVerticesByOrigin(Point p);
};

}

#endif
