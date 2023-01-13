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

#include <vector>

#include "exports.h"
#include "globals.h"

#include "Bitmap.h"
#include "Region.h"
#include "Sprite2D.h"

namespace GemRB {

class Video;

struct FogMapData {
	FogMapData(
		const Bitmap *exploredMask,
		const Bitmap *visibleMask,
		Region vp,
		Size mapSize,
		Size fogSize,
		int largeFog
	) : exploredMask(exploredMask),
		visibleMask(visibleMask),
		vp(vp),
		mapSize(mapSize),
		fogSize(fogSize),
		largeFog(largeFog)
	{}

	const Bitmap *exploredMask;
	const Bitmap *visibleMask;
	Region vp;
	Size mapSize;
	Size fogSize;
	int largeFog;
};

class GEM_EXPORT FogRenderer {
	private:
		Video* video;
		bool videoCanRenderGeometry = false;
		std::vector<float> fogVertices;
		std::vector<Color> fogColors;

		Region vp;
		Size mapSize;
		Point start;
		Point end;
		Point p0;

		enum FogDirection : uint8_t {
			O,
			N = 1,
			W = 2,
			NW = N|W, // 3
			S = 4,
			SW = S|W, // 6
			E = 8,
			NE = N|E, // 9
			SE = S|E  // 12
		};

		// Size of Fog-Of-War shadow tile (and bitmap)
		static constexpr int CELL_SIZE = 32;
		static constexpr BlitFlags BAM_FLAGS[] = {
			BlitFlags::NONE, BlitFlags::NONE, BlitFlags::NONE, BlitFlags::NONE,
			BlitFlags::MIRRORY, BlitFlags::NONE, BlitFlags::MIRRORY,
			BlitFlags::NONE, BlitFlags::MIRRORX, BlitFlags::MIRRORX,
			BlitFlags::NONE, BlitFlags::NONE, static_cast<BlitFlags>(BlitFlags::MIRRORX | BlitFlags::MIRRORY)
		};

		static constexpr BlitFlags OPAQUE_FOG = BlitFlags::NONE;
		static constexpr BlitFlags TRANSPARENT_FOG = static_cast<BlitFlags>(BlitFlags::HALFTRANS | BlitFlags::BLENDED);
	public:
		FogRenderer(Video*, bool doBAMRendering = false);

		void DrawFog(const FogMapData& mapData);

	private:
		Point ConvertPointToScreen(int x, int y) const;
		static Point ConvertPointToFog(Point p);
		void DrawExploredCell(Point cellPoint, const Bitmap *mask);
		void DrawFogCellBAM(Point p, FogDirection direction, BlitFlags flags);
		void DrawFogCellVertices(Point p, FogDirection direction, BlitFlags flags);
		bool DrawFogCellByDirection(Point p, FogDirection direction, BlitFlags flags);
		bool DrawFogCellByDirectionBAMs(Point p, FogDirection direction, BlitFlags flags);
		bool DrawFogCellByDirectionVertices(Point p, FogDirection direction, BlitFlags flags);
		void DrawFogSmoothing(Point p, FogDirection direction, BlitFlags flags, FogDirection adjacentDir);
		void DrawVisibleCell(Point cellPoint, const Bitmap *mask);
		void DrawVPBorder(Point p, FogDirection direction, const Region& r, BlitFlags flags);
		void DrawVPBorders();
		void FillFog(Point p, int numRowItems, BlitFlags flags);
		static bool IsUncovered(Point cellPoint, const Bitmap *mask);
		void SetFogVerticesByOrigin(Point p);
};

}

#endif
