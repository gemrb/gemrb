/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2004 The GemRB Project
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


#include "TraversabilityCache.h"
#include "Map.h"
#include "Scriptable/Actor.h"

namespace GemRB {
	TraversabilityCache::CachedActorState::CachedActorState(Actor *InActor) {
		if (!InActor) {
			return;
		}

		actor = InActor;
		pos = InActor->Pos;
		if (InActor->ValidTarget(GA_ONLY_BUMPABLE)) {
			SetIsBumpable();
		} else {
			ResetIsBumpable();
		}
		if (InActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED)) {
			SetIsAlive();
		} else {
			ResetIsAlive();
		}
		region = CalculateRegion(InActor);
	}

	Region TraversabilityCache::CachedActorState::CalculateRegion(const Actor *InActor) {
		const auto baseSize = InActor->CircleSize2Radius() * InActor->sizeFactor;
		const Size s(baseSize * 8, baseSize * 6);
		return {InActor->Pos - s.Center(), s};
	}

	void TraversabilityCache::CachedActorState::
	ClearOldPosition(std::vector<TraversabilityCellData> &InOutTraversability, int InWidth) const {
		for (int x = region.x_get(); x < region.x_get() + region.w_get(); ++x) {
			for (int y = region.y_get(); y < region.y_get() + region.h_get(); ++y) {
				if (!actor->IsOver(Point(x, y), pos)) {
					continue;
				}
				const auto Idx = y * InWidth * 16 + x;
				InOutTraversability[Idx] = TraversabilityCellData{};
			}
		}
	}

	void TraversabilityCache::CachedActorState::ClearNewPosition(std::vector<TraversabilityCellData> &InOutTraversability, int InWidth) const {
		const auto RemovedRegion = CalculateRegion(actor);
		for (int x = RemovedRegion.x_get(); x < RemovedRegion.x_get() + RemovedRegion.w_get(); ++x) {
			for (int y = RemovedRegion.y_get(); y < RemovedRegion.y_get() + RemovedRegion.h_get(); ++y) {
				const auto Idx = y * InWidth * 16 + x;
				InOutTraversability[Idx] = TraversabilityCellData{};
			}
		}
	}

	void TraversabilityCache::CachedActorState::MarkNewPosition(std::vector<TraversabilityCellData> &InOutTraversability, int InWidth,
	                                           bool bInUpdateSelf) {
		const CachedActorState newState{actor};

		TraversabilityCellState NewTraversabilityType{TraversabilityCellState::actorNonTraversable};
		if (!newState.GetIsAlive()) {
			NewTraversabilityType = TraversabilityCellState::empty;
		} else if (newState.GetIsBumpable()) {
			NewTraversabilityType = TraversabilityCellState::actor;
		}
		const TraversabilityCellData NewTraversability{NewTraversabilityType, actor};

		const auto &NewRegion = newState.region;
		for (int x = std::max(0, NewRegion.x_get()); x < NewRegion.x_get() + NewRegion.w_get(); ++x) {
			for (int y = std::max(0, NewRegion.y_get()); y < NewRegion.y_get() + NewRegion.h_get(); ++y) {
				if (!actor->IsOver(Point{x, y})) {
					continue;
				}
				const auto Idx = y * InWidth * 16 + x;
				InOutTraversability[Idx] = NewTraversability;
			}
		}
		if (bInUpdateSelf) {
			this->flags = newState.flags;
			this->pos = newState.pos;
			this->region = newState.region;
		}
	}

	void TraversabilityCache::CachedActorState::UpdateNewState() {
		const CachedActorState newState{actor};
		this->flags = newState.flags;
		this->pos = newState.pos;
		this->region = newState.region;
	}

	void TraversabilityCache::UpdateTraversabilityCache() {
		// this cache is updated once per frame and only if any path was requested
		if (bUpdatedTraversabilityThisFrame) {
			return;
		}
		bUpdatedTraversabilityThisFrame = true;

		// determine all changes in actors' state regarding their alive status, bumpable status and position, also check for new and removed actors
		std::vector<size_t> ActorsRemoved;
		std::vector<std::vector<TraversabilityCache::CachedActorState>::iterator> ActorsUpdated;
		std::vector<TraversabilityCache::CachedActorState> ActorsNew;

		// for all actors in the map...
		for (auto currentActor: map->actors) {

			// find this actor in cache
			const auto Found = std::find_if(CachedActorPosState.begin(), CachedActorPosState.end(),
			                                [currentActor](const TraversabilityCache::CachedActorState &CachedActor) {
				                                return CachedActor.actor == currentActor;
			                                });

			// if not found, it's a new actor
			if (Found == CachedActorPosState.cend()) {
				ActorsNew.emplace_back(currentActor);
				continue;
			}

			// if found, check if the position, bumpable status and alive status have been updated since last cache update
			const TraversabilityCache::CachedActorState &CachedActor = *Found;
			if (CachedActor.pos != currentActor->Pos ||
			    CachedActor.GetIsAlive() != currentActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) ||
			    CachedActor.GetIsBumpable() != currentActor->ValidTarget(GA_ONLY_BUMPABLE)) {
				ActorsUpdated.push_back(Found);
			}
		}

		// find all actors from cache which are not among the map actors and store their index to remove them later
		for (size_t i = 0; i < CachedActorPosState.size(); ++i) {
			const auto &CachedActorPos = CachedActorPosState[i];
			const auto Found = std::find_if(map->actors.cbegin(), map->actors.cend(), [&CachedActorPos](const Actor *actor) {
				return CachedActorPos.actor == actor;
			});
			if (Found == map->actors.cend()) {
				ActorsRemoved.push_back(i);
			}
		}

		// if there is no change, don't update
		if (ActorsNew.empty() && ActorsRemoved.empty() && ActorsUpdated.empty()) {
			return;
		}

		// make sure the cache is of proper size
		// todo: this probably could be done once, when the map is loaded
		ValidateTraversabilityCacheSize();

		// for all removed actors: clear in cache all the cells they were part of
		for (const auto RemovedIndex: ActorsRemoved) {
			CachedActorPosState[RemovedIndex].ClearOldPosition(Traversability, map->PropsSize().w);
		}

		// for all updated actors: make necessary changes based on the status change
		for (auto IteratorUpdated: ActorsUpdated) {
			TraversabilityCache::CachedActorState &Updated = *IteratorUpdated;

			// if the position of the actor changed...
			if (Updated.pos != Updated.actor->Pos) {
				// clear old position of this actor
				Updated.ClearOldPosition(Traversability, map->PropsSize().w);

				// check whether any other actor didn't share the same position, if so - change the bumpable state of this cell (I can't remember why did I change the bumpable flag here??)
				for (auto & cachedActorState : CachedActorPosState) {
					if (cachedActorState.actor == Updated.actor) {
						continue;
					}
					if (cachedActorState.region.IntersectsRegion(Updated.region)) {
						cachedActorState.FlipIsBumpable();
					}
				}
				// mark new position of this actor
				Updated.MarkNewPosition(Traversability, map->PropsSize().w, true);
				continue;
			}

			// if our actor did go from dead to alive...
			if (!Updated.GetIsAlive() && (Updated.GetIsAlive() != Updated.actor->ValidTarget(
				                              GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// no need to clear old position, just mark new position
				Updated.MarkNewPosition(Traversability, map->PropsSize().w, true);
			}
			// if our actor did go from alive to dead...
			else if (Updated.GetIsAlive() && (Updated.GetIsAlive() != Updated.actor->ValidTarget(
				                                  GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// just clear old position
				Updated.ClearOldPosition(Traversability, map->PropsSize().w);
				Updated.UpdateNewState();
			}

			// if our actor did change its bumpable state
			if (Updated.GetIsBumpable() != Updated.actor->ValidTarget(GA_ONLY_BUMPABLE)) {
				// clear old cells and mark new cells
				Updated.ClearOldPosition(Traversability, map->PropsSize().w);
				Updated.MarkNewPosition(Traversability, map->PropsSize().w, true);
			}
		}

		// for any new actors, just mark their new position
		for (auto &New: ActorsNew) {
			New.MarkNewPosition(Traversability, map->PropsSize().w);
		}

		// remove from cache all the actors deteced as removed from the map since last cache update
		for (auto RemovedIt = ActorsRemoved.crbegin(); RemovedIt != ActorsRemoved.crend(); ++RemovedIt) {
			size_t RemovedIdx = *RemovedIt;
			CachedActorPosState.erase(CachedActorPosState.begin() + RemovedIdx);
		}

		// add to cache all the actors detected as new on the map since last cache update
		for (auto &Added: ActorsNew) {
			CachedActorPosState.emplace_back(std::move(Added));
		}
	}

	void TraversabilityCache::ValidateTraversabilityCacheSize() {
		const auto ExpectedSize = map->PropsSize().h * 12 * map->PropsSize().w * 16;
		if (Traversability.size() != ExpectedSize) {
			Log(WARNING, "Map", "Resizing Traversability table.");
			Traversability.resize(ExpectedSize, TraversabilityCache::TraversabilityCellData{TraversabilityCache::TraversabilityCellState::empty, nullptr});
			memset(Traversability.data(), 0, sizeof(TraversabilityCache::TraversabilityCellData) * Traversability.size());
		}
	}
}
