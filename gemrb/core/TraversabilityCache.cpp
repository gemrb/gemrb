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


#include "TraversabilityCache.h"

#include "Map.h"

#include "Logging/Logging.h"
#include "Scriptable/Actor.h"

namespace GemRB {

size_t TraversabilityCache::CachedActorsState::AddCachedActorState(Actor* inActor)
{
	if (!inActor) {
		return -1;
	}

	const size_t new_idx = actor.size();
	actor.push_back(inActor);
	pos.push_back(inActor->Pos);
	flags.push_back(0);
	if (inActor->ValidTarget(GA_ONLY_BUMPABLE)) {
		SetIsBumpable(new_idx);
	} else {
		ResetIsBumpable(new_idx);
	}
	if (inActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED)) {
		SetIsAlive(new_idx);
	} else {
		ResetIsAlive(new_idx);
	}
	region.push_back(CalculateRegion(inActor));
	return new_idx;
}

Region TraversabilityCache::CachedActorsState::CalculateRegion(const Actor* inActor)
{
	// code from Selectable::DrawCircle, will it be always correct for all NPCs?
	const auto baseSize = inActor->CircleSize2Radius() * inActor->sizeFactor;
	const GemRB::Size s(baseSize * 8, baseSize * 6);
	return { inActor->Pos - s.Center(), s };
}

void TraversabilityCache::CachedActorsState::ClearOldPosition(const size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, const int inWidth) const
{
	for (int x = region[i].x; x < region[i].x + region[i].w; ++x) {
		for (int y = region[i].y; y < region[i].y + region[i].h; ++y) {
			if (!actor[i]->IsOver(Point(x, y), pos[i])) {
				continue;
			}
			const auto Idx = y * inWidth * 16 + x;
			inOutTraversabilityData[Idx] = TraversabilityCellData {};
		}
	}
}

void TraversabilityCache::CachedActorsState::MarkNewPosition(const size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf)
{
	const size_t newActorStateIdx = AddCachedActorState(actor[i]);

	TraversabilityCellState newCellState { TraversabilityCellState::ACTOR_NON_TRAVERSABLE };
	if (!GetIsAlive(newActorStateIdx)) {
		newCellState = TraversabilityCellState::EMPTY;
	} else if (GetIsBumpable(newActorStateIdx)) {
		newCellState = TraversabilityCellState::ACTOR;
	}
	const TraversabilityCellData newCellData { actor[i], newCellState };

	for (int x = std::max(0, region[newActorStateIdx].x); x < region[newActorStateIdx].x + region[newActorStateIdx].w; ++x) {
		for (int y = std::max(0, region[newActorStateIdx].y); y < region[newActorStateIdx].y + region[newActorStateIdx].h; ++y) {
			if (!actor[i]->IsOver(Point { x, y })) {
				continue;
			}
			const auto Idx = y * inWidth * 16 + x;
			inOutTraversabilityData[Idx] = newCellData;
		}
	}
	if (inShouldUpdateSelf) {
		flags[i] = flags[newActorStateIdx];
		pos[i] = pos[newActorStateIdx];
		region[i] = region[newActorStateIdx];
	}
	erase(newActorStateIdx);
}

void TraversabilityCache::CachedActorsState::UpdateNewState(const size_t i)
{
	const size_t newActorStateIdx = AddCachedActorState(actor[i]);
	flags[i] = flags[newActorStateIdx];
	pos[i] = pos[newActorStateIdx];
	region[i] = region[newActorStateIdx];
	erase(newActorStateIdx);
}

void TraversabilityCache::Update()
{
	// this cache is updated once per frame and only if any path was requested
	if (hasBeenUpdatedThisFrame) {
		return;
	}
	hasBeenUpdatedThisFrame = true;

	// determine all changes in actors' state regarding their alive status, bumpable status and position, also check for new and removed actors
	static std::vector<size_t> actorsRemoved;
	static std::vector<size_t> actorsUpdated;
	static CachedActorsState actorsNew(0);

	const size_t numberOfActorsOnMap = map->actors.size();
	if (actorsRemoved.capacity() < numberOfActorsOnMap) {
		actorsRemoved.reserve(numberOfActorsOnMap);
		actorsUpdated.reserve(numberOfActorsOnMap);
		actorsNew.reserve(numberOfActorsOnMap);
	}
	actorsRemoved.clear();
	actorsUpdated.clear();
	actorsNew.clear();

	// for all actors in the map...
	for (auto currentActor : map->actors) {
		// find this actor in cache
		const auto foundCachedActor = std::find_if(cachedActorsState.actor.begin(), cachedActorsState.actor.end(),
							   [currentActor](const Actor* cachedActor) {
								   return cachedActor == currentActor;
							   });

		// if not found, it's a new actor
		if (foundCachedActor == cachedActorsState.actor.cend()) {
			actorsNew.AddCachedActorState(currentActor);
			continue;
		}

		// if found, check whether the position, bumpable status and alive status has been updated since last cache update
		// const CachedActorsState& cachedActor = *foundCachedActor;
		const size_t cachedActorIdx = foundCachedActor - cachedActorsState.actor.begin();
		if (cachedActorsState.pos[cachedActorIdx] != currentActor->Pos ||
		    cachedActorsState.GetIsAlive(cachedActorIdx) != currentActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) ||
		    cachedActorsState.GetIsBumpable(cachedActorIdx) != currentActor->ValidTarget(GA_ONLY_BUMPABLE)) {
			actorsUpdated.push_back(cachedActorIdx);
		}
	}

	// find all actors from cache which are not among the map actors and store their index to remove them later
	for (size_t i = 0; i < cachedActorsState.actor.size(); ++i) {
		const Actor* const cachedActor = cachedActorsState.actor[i];
		const auto foundActor = std::find_if(map->actors.cbegin(), map->actors.cend(), [&cachedActor](const Actor* actor) {
			return cachedActor == actor;
		});
		if (foundActor == map->actors.cend()) {
			actorsRemoved.push_back(i);
		}
	}

	// if there is no change, don't update
	if (actorsNew.actor.empty() && actorsRemoved.empty() && actorsUpdated.empty()) {
		return;
	}

	// todo: this probably could be done once, when the map is loaded
	// make sure the cache is of proper size
	ValidateTraversabilityCacheSize();

	// for all removed actors: clear in cache all the cells they were part of
	for (const auto removedCachedActorIndex : actorsRemoved) {
		cachedActorsState.ClearOldPosition(removedCachedActorIndex, traversabilityData, map->PropsSize().w);
	}

	// for all updated actors: make necessary changes based on the status change
	for (auto updatedCachedIdx : actorsUpdated) {
		// CachedActorsState& updatedCachedActor = *updatedIdx;

		// if the position of the actor changed...
		if (cachedActorsState.pos[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->Pos) {
			// clear old position of this actor
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w);

			// check whether any other actor didn't share the same position - if so, re-draw their position on the cache
			for (size_t cachedActorIdx = 0; cachedActorIdx < cachedActorsState.actor.size(); ++cachedActorIdx) {
				if (cachedActorsState.actor[cachedActorIdx] == cachedActorsState.actor[updatedCachedIdx]) {
					continue;
				}
				if (cachedActorsState.region[cachedActorIdx].IntersectsRegion(cachedActorsState.region[updatedCachedIdx])) {
					cachedActorsState.MarkNewPosition(cachedActorIdx, traversabilityData, map->PropsSize().w, false);
				}
			}
			// mark new position of this actor
			cachedActorsState.MarkNewPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w, true);
			continue;
		}

		// if our actor did go from dead to alive...
		if (!cachedActorsState.GetIsAlive(updatedCachedIdx) && (cachedActorsState.GetIsAlive(updatedCachedIdx) != cachedActorsState.actor[updatedCachedIdx]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
			// no need to clear old position, just mark new position
			cachedActorsState.MarkNewPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w, true);
		}
		// if our actor did go from alive to dead...
		else if (cachedActorsState.GetIsAlive(updatedCachedIdx) && (cachedActorsState.GetIsAlive(updatedCachedIdx) != cachedActorsState.actor[updatedCachedIdx]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
			// just clear old position
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w);
			cachedActorsState.UpdateNewState(updatedCachedIdx);
		}

		// if our actor did change its bumpable state
		if (cachedActorsState.GetIsBumpable(updatedCachedIdx) != cachedActorsState.actor[updatedCachedIdx]->ValidTarget(GA_ONLY_BUMPABLE)) {
			// clear old cells and mark new cells
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w);
			cachedActorsState.MarkNewPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w, true);
		}
	}

	// for any new actors, just mark their new position
	for (size_t i = 0; i < actorsNew.actor.size(); ++i) {
		actorsNew.MarkNewPosition(i, traversabilityData, map->PropsSize().w);
	}

	// remove from cache all the actors deteced as removed from the map since last cache update
	for (auto iteratorRemoved = actorsRemoved.crbegin(); iteratorRemoved != actorsRemoved.crend(); ++iteratorRemoved) {
		const size_t removedIdx = *iteratorRemoved;
		cachedActorsState.erase(removedIdx);
	}

	// add to cache all the actors detected as new on the map since last cache update
	cachedActorsState.emplace_back(std::move(actorsNew));
}

TraversabilityCache::CachedActorsState::CachedActorsState(const size_t reserve)
{
	if (reserve != 0) {
		this->reserve(reserve);
	}
}

void TraversabilityCache::CachedActorsState::reserve(const size_t reserve)
{
	region.reserve(reserve);
	actor.reserve(reserve);
	pos.reserve(reserve);
	flags.reserve(reserve);
}

void TraversabilityCache::CachedActorsState::clear()
{
	region.clear();
	actor.clear();
	pos.clear();
	flags.clear();
}

void TraversabilityCache::CachedActorsState::erase(const size_t idx)
{
	region.erase(region.begin() + idx);
	actor.erase(actor.begin() + idx);
	pos.erase(pos.begin() + idx);
	flags.erase(flags.begin() + idx);
}

void TraversabilityCache::CachedActorsState::emplace_back(CachedActorsState&& another)
{
	reserve(region.size() + another.region.size());
	region.insert(region.end(), another.region.begin(), another.region.end());
	actor.insert(actor.end(), another.actor.begin(), another.actor.end());
	pos.insert(pos.end(), another.pos.begin(), another.pos.end());
	flags.insert(flags.end(), another.flags.begin(), another.flags.end());
}

void TraversabilityCache::ValidateTraversabilityCacheSize()
{
	const size_t expectedSize = map->PropsSize().h * 12 * map->PropsSize().w * 16;
	if (traversabilityData.size() != expectedSize) {
		Log(DEBUG, "TraversabilityCache", "Resizing traversabilityData cache.");
		traversabilityData.resize(expectedSize, TraversabilityCellData { nullptr, TraversabilityCellState::EMPTY });
		memset(static_cast<void*>(traversabilityData.data()), 0, sizeof(TraversabilityCellData) * traversabilityData.size());
	}
}
}
