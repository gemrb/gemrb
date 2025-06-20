/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef TRAVERSABILITY_CACHE_H
#define TRAVERSABILITY_CACHE_H

#include "Scriptable/Actor.h"


namespace GemRB {

/**
 * This class manages the cached data of actors on a navmap, to be used for speed up the FindPath implementation.
 */
class TraversabilityCache {
public:
	/**
	 * Enum describing traversability state of a single navmap point in terms of occupying them actors.
	 */
	enum class TraversabilityCellState : std::uint8_t {
		EMPTY = 0, // cell is empty, meaning no actor is standing there, so is traversable; default value
		ACTOR, // there is an actor occupying this cell, but the actor is bumpable
		ACTOR_NON_TRAVERSABLE, // there is an actor occupying this cell, and the actor is not bumpable

		MAX
	};

	/**
	 * Struct holding data describing traversability of a navmap point: its state and potential actor data.
	 */
	struct TraversabilityCellData {
		Actor* occupyingActor = nullptr;
		TraversabilityCellState state = TraversabilityCellState::EMPTY;
	};

	explicit TraversabilityCache(Map* inMap)
		: map { inMap }
	{
	}

	TraversabilityCellData GetCellData(const std::size_t inIndex) const
	{
		return traversabilityData[inIndex];
	}

	bool HasUpdatedTraversabilityThisFrame() const
	{
		return hasBeenUpdatedThisFrame;
	}

	void MarkNewFrame()
	{
		hasBeenUpdatedThisFrame = false;
	}

	size_t Size() const
	{
		return traversabilityData.size();
	}

	void Update();

private:
	/**
	 * Struct for storing cached state of actors on the map: position, occupied region on the navmap,
	 * bumpable state and alive state.
	 */
	struct CachedActorState {
		constexpr static uint8_t FLAG_BUMPABLE = 1;
		constexpr static uint8_t FLAG_ALIVE = 2;

		Region region;
		Actor* actor = nullptr;
		Point pos;
		uint8_t flags {};

		explicit CachedActorState(Actor* inActor);

		void ClearOldPosition(std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth) const;

		void MarkNewPosition(std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf = false);

		void UpdateNewState();

		static Region CalculateRegion(const Actor* inActor);

		// flags manipulation should be inlined
		void SetIsBumpable()
		{
			flags |= (1 << FLAG_BUMPABLE);
		}

		void ResetIsBumpable()
		{
			flags &= ~(1 << FLAG_BUMPABLE);
		}

		void SetIsAlive()
		{
			flags |= (1 << FLAG_ALIVE);
		}

		void ResetIsAlive()
		{
			flags &= ~(1 << FLAG_ALIVE);
		}

		void FlipIsBumpable()
		{
			flags ^= (1 << FLAG_BUMPABLE);
		}

		bool GetIsBumpable() const
		{
			return flags & (1 << FLAG_BUMPABLE);
		}

		bool GetIsAlive() const
		{
			return flags & (1 << FLAG_ALIVE);
		}
	};

	Map* map;
	std::vector<TraversabilityCellData> traversabilityData;
	std::vector<CachedActorState> cachedActorsState;
	bool hasBeenUpdatedThisFrame { false };

	void ValidateTraversabilityCacheSize();
};
}

#endif
