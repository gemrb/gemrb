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
#include "PathfindingSettings.h"

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
	sizeCategory.push_back(inActor->getSizeCategory());
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

FitRegion TraversabilityCache::CachedActorsState::CalculateRegion(const Actor* inActor)
{
	// code from Selectable::DrawCircle, will it be always correct for all NPCs?
	const auto baseSize = Selectable::CircleSize2Radius(inActor->circleSize) * inActor->sizeFactor;
	const GemRB::Size s(baseSize * 8, baseSize * 6);
	return { inActor->Pos - s.Center(), s };
}

void TraversabilityCache::CachedActorsState::ClearOldPosition(const size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, const int inWidth) const
{
	const std::vector<uint8_t>& CachedBlockingShape = Actor::GetBlockingShape(actor[i], sizeCategory[i]);
	if (CachedBlockingShape.empty()) {
		return;
	}

	const auto CachedCellState = GetCellStateFromFlags(i);
	const auto BlockingShapeRegionW = Actor::GetBlockingShapeRegionW(sizeCategory[i]);

	for (int y = 0; y < region[i].size.h; ++y) {
		for (int x = 0; x < region[i].size.w; ++x) {

			const int targetX = region[i].origin.x + x;
			const int targetY = region[i].origin.y + y;

			if (targetX < 0 || targetY < 0) {
				continue;
			}

			const auto Idx = targetY * inWidth * 16 + targetX;
			TraversabilityCellData& CurrentTraversabilityData = inOutTraversabilityData[Idx];

			const auto BlockingShapeIdx = y * BlockingShapeRegionW * 16 + x;
			CurrentTraversabilityData.state -= (CachedBlockingShape[BlockingShapeIdx] * CachedCellState);

			if (CurrentTraversabilityData.occupyingActor == actor[i]) {
				CurrentTraversabilityData.occupyingActor = nullptr;
			}
		}
	}
}

void TraversabilityCache::CachedActorsState::MarkNewPosition(const size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf)
{
	const auto CurrentSizeCategory = actor[i]->getSizeCategory();
	const std::vector<uint8_t>& CurrentBlockingShape = Actor::GetBlockingShape(actor[i], CurrentSizeCategory);
	if (CurrentBlockingShape.empty()) {
		return;
	}

	const size_t newActorStateIdx = AddCachedActorState(actor[i]);

	const auto CurrentCellState = GetCellStateFromFlags(newActorStateIdx);
	const auto BlockingShapeRegionW = Actor::GetBlockingShapeRegionW(CurrentSizeCategory);

	for (int y = 0; y < region[newActorStateIdx].size.h; ++y) {
		for (int x = 0; x < region[newActorStateIdx].size.w; ++x) {

			const int targetX = region[newActorStateIdx].origin.x + x;
			const int targetY = region[newActorStateIdx].origin.y + y;

			if (targetX < 0 || targetY < 0) {
				continue;
			}

			const auto Idx = targetY * inWidth * 16 + targetX;
			TraversabilityCellData& CurrentTraversabilityData = inOutTraversabilityData[Idx];

			const auto BlockingShapeIdx = y * BlockingShapeRegionW * 16 + x;
			const uint16_t CellStateOfThisActor = CurrentBlockingShape[BlockingShapeIdx] * CurrentCellState;
			CurrentTraversabilityData.state += CellStateOfThisActor;

			if (CellStateOfThisActor > TraversabilityCellValueEmpty) {
				CurrentTraversabilityData.occupyingActor = actor[i];
			}
		}
	}

	if (inShouldUpdateSelf) {
		flags[i] = flags[newActorStateIdx];
		pos[i] = pos[newActorStateIdx];
		region[i] = region[newActorStateIdx];
		sizeCategory[i] = sizeCategory[newActorStateIdx];
	}

	erase(newActorStateIdx);
}

void TraversabilityCache::CachedActorsState::UpdateNewState(const size_t i)
{
	const size_t newActorStateIdx = AddCachedActorState(actor[i]);
	flags[i] = flags[newActorStateIdx];
	pos[i] = pos[newActorStateIdx];
	region[i] = region[newActorStateIdx];
	sizeCategory[i] = sizeCategory[newActorStateIdx];
	erase(newActorStateIdx);
}

void TraversabilityCache::Update()
{
	// this cache is updated once per frame and only if any path was requested
	if (hasBeenUpdatedThisFrame) {
		return;
	}
	hasBeenUpdatedThisFrame = true;

	ScopedTimer::extraTimeTracked.push_back(0);
	ScopedTimer::extraTagsTracked.push_back(std::string{});
	const size_t extraTrackedIdx =  ScopedTimer::extraTimeTracked.size() - 1;
	ScopedTimer s("$", &ScopedTimer::extraTimeTracked[extraTrackedIdx], &ScopedTimer::extraTagsTracked[extraTrackedIdx]);

	// determine all changes in actors' state regarding their alive status, bumpable status, their size category and position, also check for new and removed actors
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
		    cachedActorsState.GetIsBumpable(cachedActorIdx) != currentActor->ValidTarget(GA_ONLY_BUMPABLE) ||
		    cachedActorsState.sizeCategory[cachedActorIdx] != currentActor->getSizeCategory()) {
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

		// if the position or the size category of the actor changed...
		if (cachedActorsState.pos[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->Pos ||
			cachedActorsState.sizeCategory[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->getSizeCategory()) {

			// clear old position of this actor
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w);

			// // check whether any other actor didn't share the same position - if so, re-draw their position on the cache
			// for (size_t cachedActorIdx = 0; cachedActorIdx < cachedActorsState.actor.size(); ++cachedActorIdx) {
			// 	if (cachedActorsState.actor[cachedActorIdx] == cachedActorsState.actor[updatedCachedIdx]) {
			// 		continue;
			// 	}
			// 	if (cachedActorsState.region[cachedActorIdx].IntersectsRegion(cachedActorsState.region[updatedCachedIdx])) {
			// 		cachedActorsState.MarkNewPosition(cachedActorIdx, traversabilityData, map->PropsSize().w, false);
			// 	}
			// }
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
	sizeCategory.reserve(reserve);
}

void TraversabilityCache::CachedActorsState::clear()
{
	region.clear();
	actor.clear();
	pos.clear();
	flags.clear();
	sizeCategory.clear();
}

void TraversabilityCache::CachedActorsState::erase(const size_t idx)
{
	region.erase(region.begin() + idx);
	actor.erase(actor.begin() + idx);
	pos.erase(pos.begin() + idx);
	flags.erase(flags.begin() + idx);
	sizeCategory.erase(sizeCategory.begin() + idx);
}

void TraversabilityCache::CachedActorsState::emplace_back(CachedActorsState&& another)
{
	reserve(region.size() + another.region.size());
	region.insert(region.end(), another.region.begin(), another.region.end());
	actor.insert(actor.end(), another.actor.begin(), another.actor.end());
	pos.insert(pos.end(), another.pos.begin(), another.pos.end());
	flags.insert(flags.end(), another.flags.begin(), another.flags.end());
	sizeCategory.insert(sizeCategory.end(), another.sizeCategory.begin(), another.sizeCategory.end());
}

TraversabilityCache::TraversabilityCellState TraversabilityCache::CachedActorsState::GetCellStateFromFlags(const size_t i) const {
	TraversabilityCellState newCellState { TraversabilityCellValueActorNonTraversable };
	if (!GetIsAlive(i)) {
		newCellState = TraversabilityCellValueEmpty;
	} else if (GetIsBumpable(i)) {
		newCellState = TraversabilityCellValueActor;
	}
	return newCellState;
}

void TraversabilityCache::ValidateTraversabilityCacheSize()
{
	const size_t expectedSize = map->PropsSize().h * 12 * map->PropsSize().w * 16;
	if (traversabilityData.size() != expectedSize) {
		Log(DEBUG, "TraversabilityCache", "Resizing traversabilityData cache.");
		traversabilityData.resize(expectedSize, TraversabilityCellData { nullptr, TraversabilityCellValueEmpty });
		memset(static_cast<void*>(traversabilityData.data()), 0, sizeof(TraversabilityCellData) * traversabilityData.size());
	}
}
}
