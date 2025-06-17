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

#include "Scriptable/Actor.h"

namespace GemRB {

TraversabilityCache::CachedActorState::CachedActorState(Actor* inActor)
{
	if (!inActor) {
		return;
	}

	actor = inActor;
	pos = inActor->Pos;
	if (inActor->ValidTarget(GA_ONLY_BUMPABLE)) {
		SetIsBumpable();
	} else {
		ResetIsBumpable();
	}
	if (inActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED)) {
		SetIsAlive();
	} else {
		ResetIsAlive();
	}
	region = CalculateRegion(inActor);
}

Region TraversabilityCache::CachedActorState::CalculateRegion(const Actor* inActor)
{
	// code from Selectable::DrawCircle, will it be always correct for all NPCs?
	const auto baseSize = inActor->CircleSize2Radius() * inActor->sizeFactor;
	const GemRB::Size s(baseSize * 8, baseSize * 6);
	return { inActor->Pos - s.Center(), s };
}

void TraversabilityCache::CachedActorState::ClearOldPosition(std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth) const
{
	for (int x = region.x; x < region.x + region.w; ++x) {
		for (int y = region.y; y < region.y + region.h; ++y) {
			if (!actor->IsOver(Point(x, y), pos)) {
				continue;
			}
			const auto Idx = y * inWidth * 16 + x;
			inOutTraversabilityData[Idx] = TraversabilityCellData {};
		}
	}
}

void TraversabilityCache::CachedActorState::MarkNewPosition(std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf)
{
	const CachedActorState newActorState { actor };

	TraversabilityCellState newCellState { TraversabilityCellState::ACTOR_NON_TRAVERSABLE };
	if (!newActorState.GetIsAlive()) {
		newCellState = TraversabilityCellState::EMPTY;
	} else if (newActorState.GetIsBumpable()) {
		newCellState = TraversabilityCellState::ACTOR;
	}
	const TraversabilityCellData newCellData { actor, newCellState };

	for (int x = std::max(0, newActorState.region.x); x < newActorState.region.x + newActorState.region.w; ++x) {
		for (int y = std::max(0, newActorState.region.y); y < newActorState.region.y + newActorState.region.h; ++y) {
			if (!actor->IsOver(Point { x, y })) {
				continue;
			}
			const auto Idx = y * inWidth * 16 + x;
			inOutTraversabilityData[Idx] = newCellData;
		}
	}
	if (inShouldUpdateSelf) {
		this->flags = newActorState.flags;
		this->pos = newActorState.pos;
		this->region = newActorState.region;
	}
}

void TraversabilityCache::CachedActorState::UpdateNewState()
{
	const CachedActorState newActorState { actor };
	this->flags = newActorState.flags;
	this->pos = newActorState.pos;
	this->region = newActorState.region;
}

void TraversabilityCache::Update()
{
	// this cache is updated once per frame and only if any path was requested
	if (hasBeenUpdatedThisFrame) {
		return;
	}
	hasBeenUpdatedThisFrame = true;

	// determine all changes in actors' state regarding their alive status, bumpable status and position, also check for new and removed actors
	std::vector<size_t> actorsRemoved;
	std::vector<std::vector<CachedActorState>::iterator> actorsUpdated;
	std::vector<CachedActorState> actorsNew;

	// for all actors in the map...
	for (auto currentActor : map->actors) {
		// find this actor in cache
		const auto foundCachedActor = std::find_if(cachedActorsState.begin(), cachedActorsState.end(),
							   [currentActor](const CachedActorState& cachedActor) {
								   return cachedActor.actor == currentActor;
							   });

		// if not found, it's a new actor
		if (foundCachedActor == cachedActorsState.cend()) {
			actorsNew.emplace_back(currentActor);
			continue;
		}

		// if found, check whether the position, bumpable status and alive status has been updated since last cache update
		const CachedActorState& cachedActor = *foundCachedActor;
		if (cachedActor.pos != currentActor->Pos ||
		    cachedActor.GetIsAlive() != currentActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) ||
		    cachedActor.GetIsBumpable() != currentActor->ValidTarget(GA_ONLY_BUMPABLE)) {
			actorsUpdated.push_back(foundCachedActor);
		}
	}

	// find all actors from cache which are not among the map actors and store their index to remove them later
	for (size_t i = 0; i < cachedActorsState.size(); ++i) {
		const auto& cachedActorState = cachedActorsState[i];
		const auto foundActor = std::find_if(map->actors.cbegin(), map->actors.cend(), [&cachedActorState](const Actor* actor) {
			return cachedActorState.actor == actor;
		});
		if (foundActor == map->actors.cend()) {
			actorsRemoved.push_back(i);
		}
	}

	// if there is no change, don't update
	if (actorsNew.empty() && actorsRemoved.empty() && actorsUpdated.empty()) {
		return;
	}

	// todo: this probably could be done once, when the map is loaded
	// make sure the cache is of proper size
	ValidateTraversabilityCacheSize();

	// for all removed actors: clear in cache all the cells they were part of
	for (const auto removedCachedActorIndex : actorsRemoved) {
		cachedActorsState[removedCachedActorIndex].ClearOldPosition(traversabilityData, map->PropsSize().w);
	}

	// for all updated actors: make necessary changes based on the status change
	for (auto iteratorUpdated : actorsUpdated) {
		CachedActorState& updatedCachedActor = *iteratorUpdated;

		// if the position of the actor changed...
		if (updatedCachedActor.pos != updatedCachedActor.actor->Pos) {
			// clear old position of this actor
			updatedCachedActor.ClearOldPosition(traversabilityData, map->PropsSize().w);

			// check whether any other actor didn't share the same position, if so - change the bumpable state of this cell (I can't remember why did I change the bumpable flag here??)
			for (auto& cachedActorState : cachedActorsState) {
				if (cachedActorState.actor == updatedCachedActor.actor) {
					continue;
				}
				if (cachedActorState.region.IntersectsRegion(updatedCachedActor.region)) {
					cachedActorState.FlipIsBumpable();
				}
			}
			// mark new position of this actor
			updatedCachedActor.MarkNewPosition(traversabilityData, map->PropsSize().w, true);
			continue;
		}

		// if our actor did go from dead to alive...
		if (!updatedCachedActor.GetIsAlive() && (updatedCachedActor.GetIsAlive() != updatedCachedActor.actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
			// no need to clear old position, just mark new position
			updatedCachedActor.MarkNewPosition(traversabilityData, map->PropsSize().w, true);
		}
		// if our actor did go from alive to dead...
		else if (updatedCachedActor.GetIsAlive() && (updatedCachedActor.GetIsAlive() != updatedCachedActor.actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
			// just clear old position
			updatedCachedActor.ClearOldPosition(traversabilityData, map->PropsSize().w);
			updatedCachedActor.UpdateNewState();
		}

		// if our actor did change its bumpable state
		if (updatedCachedActor.GetIsBumpable() != updatedCachedActor.actor->ValidTarget(GA_ONLY_BUMPABLE)) {
			// clear old cells and mark new cells
			updatedCachedActor.ClearOldPosition(traversabilityData, map->PropsSize().w);
			updatedCachedActor.MarkNewPosition(traversabilityData, map->PropsSize().w, true);
		}
	}

	// for any new actors, just mark their new position
	for (auto& newActor : actorsNew) {
		newActor.MarkNewPosition(traversabilityData, map->PropsSize().w);
	}

	// remove from cache all the actors deteced as removed from the map since last cache update
	for (auto iteratorRemoved = actorsRemoved.crbegin(); iteratorRemoved != actorsRemoved.crend(); ++iteratorRemoved) {
		const size_t removedIdx = *iteratorRemoved;
		cachedActorsState.erase(cachedActorsState.begin() + removedIdx);
	}

	// add to cache all the actors detected as new on the map since last cache update
	for (auto& newActor : actorsNew) {
		cachedActorsState.emplace_back(std::move(newActor));
	}
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
