// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_SEARCHMAPBUILDER_H
#define TESTS_SEARCHMAPBUILDER_H

#include "../../core/Region.h"
#include "../../core/TileProps.h"

#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <vector>

namespace GemRB {
namespace test {

	/**
	 * Define ASCII glyphs for terrain and actors, so that pathfinding tests
	 * won't need real data, and instead define their cases in-place.
	 *
	 * Glyphs accepted by TestSearchMap, and produced back by RenderSearchMapChar().
	 *
	 * Terrain, round-trips both ways:
	 *   '.'        passable floor
	 *   '#'        wall - impassable and blocks line of sight (SIDEWALL)
	 *   'X'        impassable, but not a sight blocker (plain IMPASSABLE)
	 *   'T'        passable travel trigger
	 *   'D'        passable floor with a closed door on it
	 *   'O'        as 'D', but the door is opaque too
	 *
	 * Actors, input only. The glyph says how big the actor is, because the footprint
	 * PaintSearchMap() stamps is a circle of radius circleSize-1 around the glyph:
	 *   '1'..'8'   a party member of that circleSize
	 *   'a'..'h'   an NPC of circleSize 1..8
	 * The tile under an actor is passable floor. Footprints are painted in reading order, so a
	 * later actor overwrites the marks of an earlier one where they overlap, same as in game.
	 *
	 * Actor footprints render back as:
	 *   'P' / 'N'  a PC / NPC mark on floor
	 *   'p' / 'n'  a PC / NPC mark on a travel tile
	 *   '!'        an actor mark on terrain that cannot be walked on - should point at bug
	 *   '?'        a flag combination with no glyph, so an unexpected one
	 */

	/**
	 * A map drawing, one string per row.
	 */
	using MapRows = std::initializer_list<std::string>;

	inline bool IsActorGlyph(const char c)
	{
		return (c >= '1' && c <= '8') || (c >= 'a' && c <= 'h');
	}

	inline PathMapFlags ActorGlyphFlag(const char c)
	{
		return (c >= '1' && c <= '8') ? PathMapFlags::PC : PathMapFlags::NPC;
	}

	inline uint16_t ActorGlyphCircleSize(const char c)
	{
		return (c >= '1' && c <= '8') ? uint16_t(c - '0') : uint16_t(c - 'a' + 1);
	}

	inline PathMapFlags ParseSearchMapChar(const char c)
	{
		switch (c) {
			case '.':
				return PathMapFlags::PASSABLE;
			case '#':
				return PathMapFlags::SIDEWALL;
			case 'X':
				return PathMapFlags::IMPASSABLE;
			case 'T':
				return PathMapFlags::PASSABLE | PathMapFlags::TRAVEL;
			case 'D':
				return PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE;
			case 'O':
				return PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::DOOR_OPAQUE;
			default:
				// actors stand on plain floor; their footprint is painted afterwards
				if (IsActorGlyph(c)) return PathMapFlags::PASSABLE;

				// anything else is a typo in the literal, not terrain - report test failure
				ADD_FAILURE() << "unknown searchmap glyph '" << c << "'";
				return PathMapFlags::IMPASSABLE;
		}
	}

	/** Inverse of ParseSearchMapChar(), so a whole map can be conveniently diffed as text. */
	inline char RenderSearchMapChar(const PathMapFlags flags)
	{
		const PathMapFlags terrain = flags & PathMapFlags::AREAMASK;
		const PathMapFlags actor = flags & PathMapFlags::ACTOR;
		const PathMapFlags door = flags & PathMapFlags::DOOR;

		if (actor != PathMapFlags::UNMARKED) {
			// an actor mark only belongs on walkable terrain; anywhere else it should be a bug
			if (door != PathMapFlags::UNMARKED || !bool(terrain & PathMapFlags::PASSABLE)) {
				return '!';
			}
			const bool travel = bool(terrain & PathMapFlags::TRAVEL);
			if (actor == PathMapFlags::PC) return travel ? 'p' : 'P';
			if (actor == PathMapFlags::NPC) return travel ? 'n' : 'N';
			return '!'; // both actor bits at once, which nothing ever paints
		}

		if (bool(door & PathMapFlags::DOOR_IMPASSABLE)) {
			return bool(door & PathMapFlags::DOOR_OPAQUE) ? 'O' : 'D';
		}

		if (terrain == PathMapFlags::IMPASSABLE) return 'X';
		if (terrain == PathMapFlags::PASSABLE) return '.';
		if (terrain == PathMapFlags::SIDEWALL) return '#';
		if (terrain == (PathMapFlags::PASSABLE | PathMapFlags::TRAVEL)) return 'T';
		return '?'; // any unhandled above combination is treated as unknown and should be investigated
	}

	/**
	 * Builds TileProps out of an ASCII drawing, so pathfinder tests can state their terrain
	 * inline instead of shipping a searchmap BMP. Only the searchmap byte is filled in; the
	 * pathfinder never reads the material, height or light channels.
	 *
	 * One character is one searchmap tile, which is 16x12 navmap pixels.
	 */
	class TestSearchMap {
	public:
		TestSearchMap(const MapRows inMapRows)
		{
			const int rowCount = static_cast<int>(inMapRows.size());
			const int rowWidth = rowCount ? static_cast<int>(inMapRows.begin()->size()) : 0;

			// verify we have all lines the same length
			for (const std::string& row : inMapRows) {
				if (static_cast<int>(row.size()) != rowWidth) {
					ADD_FAILURE() << "every map row must be " << rowWidth
						      << " characters wide, got " << row;
					return;
				}
			}

			height = rowCount;
			width = rowWidth;

			props = OwningTileProps::MakeEmpty(Size(width, height));
			std::vector<std::pair<SearchmapPoint, char>> actors;

			int y = 0;
			// parse chars one by one and write it as flags
			// It builds the terrain first, actors are not marked here yet
			for (const std::string& row : inMapRows) {
				for (int x = 0; x < width; ++x) {
					const SearchmapPoint tile(x, y);
					props.SetTileProp(tile, TileProps::Property::SEARCH_MAP,
							  uint8_t(ParseSearchMapChar(row[x])));
					if (IsActorGlyph(row[x])) {
						actors.emplace_back(tile, row[x]);
					}
				}
				++y;
			}

			// only after the terrain is ready, mark the actors
			for (const auto& actor : actors) {
				props.PaintSearchMap(actor.first, ActorGlyphCircleSize(actor.second),
						     ActorGlyphFlag(actor.second));
			}
		}

		TileProps& Props() noexcept { return props; }
		const TileProps& Props() const noexcept { return props; }

		int Width() const noexcept { return width; }
		int Height() const noexcept { return height; }

		PathMapFlags At(int x, int y) const noexcept
		{
			return props.QuerySearchMap(SearchmapPoint(x, y));
		}

		/** The map as it stands now, one string per row, in the glyphs listed above. */
		std::vector<std::string> Render() const
		{
			std::vector<std::string> out;
			out.reserve(height);
			for (int y = 0; y < height; ++y) {
				std::string row;
				row.reserve(width);
				for (int x = 0; x < width; ++x) {
					row.push_back(RenderSearchMapChar(At(x, y)));
				}
				out.push_back(std::move(row));
			}
			return out;
		}

		/**
		 * Compares the current state against a drawing of the expected one. Meant for
		 * EXPECT_TRUE(map.Matches({...})) - this allows to:
		 * a) assert on expected ASCII drawing of the map, instead of series of asserts on coords,
		 * b) print nice message on failure.
		 */
		testing::AssertionResult Matches(MapRows expected) const;

		// centre of a tile in navmap coordinates, which is what the Point-taking overloads want
		static Point Nav(int x, int y) noexcept
		{
			return Point(x * 16 + 8, y * 12 + 6);
		}

	private:
		OwningTileProps props;
		int width = 0;
		int height = 0;
	};

	inline testing::AssertionResult TestSearchMap::Matches(const MapRows expected) const
	{
		const std::vector<std::string> actual = Render();
		const std::vector<std::string> want(expected.begin(), expected.end());

		if (want.size() != actual.size()) {
			return testing::AssertionFailure()
				<< "expected " << want.size() << " rows, the map has " << actual.size();
		}

		std::vector<int> badRows;
		for (size_t y = 0; y < want.size(); ++y) {
			if (want[y] != actual[y]) badRows.push_back(static_cast<int>(y));
		}
		if (badRows.empty()) return testing::AssertionSuccess();

		const size_t column = std::max<size_t>(size_t(width), 8) + 6;
		std::string msg = "\nsearchmap mismatch on " + std::to_string(badRows.size()) + " row(s)\n";
		msg += "  expected" + std::string(column - 8, ' ') + "actual\n";
		for (size_t y = 0; y < want.size(); ++y) {
			std::string line = "  " + want[y];
			line.resize(column + 2, ' ');
			line += actual[y];
			if (want[y] != actual[y]) line += "   <-- differs";
			msg += line + "\n";
		}

		bool sawBug = false;
		bool sawUnknown = false;
		for (const std::string& row : actual) {
			sawBug = sawBug || row.find('!') != std::string::npos;
			sawUnknown = sawUnknown || row.find('?') != std::string::npos;
		}
		if (sawBug) msg += "  '!' = actor mark on terrain that cannot be walked on\n";
		if (sawUnknown) msg += "  '?' = unexpected flag combination, needs investigation\n";

		return testing::AssertionFailure() << msg;
	}

}
}

#endif // TESTS_SEARCHMAPBUILDER_H
