/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003 The GemRB Project
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

#include "exports.h"
#include "globals.h"

#include "Bitmap.h"
#include "FogRenderer.h"
#include "MapReverb.h"
#include "PathFinder.h"
#include "Polygon.h"
#include "WorldMap.h"

#include "Scriptable/Scriptable.h"
#include "Video/Video.h"

#include <algorithm>
#include <queue>
#include <unordered_map>

template<class V>
class FibonacciHeap;

namespace GemRB {
	class TraversabilityCache {
	public:
		enum class TraversabilityCellState: uint8_t {
			empty = 0, // is empty, meaning no actor is standing there, so is traversable; default value
			actor, // there is an actor occupying this cell, but the actor is bumpable
			actorNonTraversable, // there is an actor occupying this cell, and the actor is not bumpable

			max
		};

		struct TraversabilityCellData {
			TraversabilityCellState type = TraversabilityCellState::empty;
			Actor *actor = nullptr;
		};

		explicit TraversabilityCache(Map *inMap): map{inMap} {
		}

		void UpdateTraversabilityCache();

		TraversabilityCellData GetCellState(std::size_t Index) const {
			return Traversability[Index];
		}

		bool HasUpdatedTraversabilityThisFrame() const {
			return bUpdatedTraversabilityThisFrame;
		}

		void MarkNewFrame() {
			bUpdatedTraversabilityThisFrame = false;
		}

		size_t size() const {
			return Traversability.size();
		}

	private:
		struct CachedActorState {
			Region region;
			Actor *actor = nullptr;
			Point pos;
			uint8_t flags{};

			constexpr static uint8_t FLAG_BUMPABLE = 1;
			constexpr static uint8_t FLAG_ALIVE = 2;

			// manipulation of flags should be inlined
			void SetIsBumpable() {
				flags |= (1 << FLAG_BUMPABLE);
			}

			void ResetIsBumpable() {
				flags &= ~(1 << FLAG_BUMPABLE);
			}

			void SetIsAlive() {
				flags |= (1 << FLAG_ALIVE);
			}

			void ResetIsAlive() {
				flags &= ~(1 << FLAG_ALIVE);
			}

			void FlipIsBumpable() {
				flags ^= (1 << FLAG_BUMPABLE);
			}

			bool GetIsBumpable() const {
				return flags & (1 << FLAG_BUMPABLE);
			}

			bool GetIsAlive() const {
				return flags & (1 << FLAG_ALIVE);
			}

			explicit CachedActorState(Actor *InActor);

			void ClearOldPosition(std::vector<TraversabilityCellData> &InOutTraversability, int InWidth) const;

			void ClearNewPosition(std::vector<TraversabilityCellData> &InOutTraversability, int InWidth) const;

			void MarkNewPosition(std::vector<TraversabilityCellData> &InOutTraversability, int InWidth,
			                     bool bInUpdateSelf = false);

			void UpdateNewState();

			static Region CalculateRegion(const Actor *InActor);
		};

		Map *map;
		std::vector<TraversabilityCellData> Traversability;
		std::vector<CachedActorState> CachedActorPosState;
		bool bUpdatedTraversabilityThisFrame{false};

		void ValidateTraversabilityCacheSize();
	};
}

#endif
