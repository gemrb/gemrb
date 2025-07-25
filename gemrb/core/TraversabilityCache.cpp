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

std::vector<std::vector<bool>> TraversabilityCache::BlockingShapeCache;

size_t TraversabilityCache::CachedActorsState::AddCachedActorState(Actor* inActor)
{
	if (!inActor) {
		return -1;
	}

	const size_t newIdx = actor.size();

	actor.push_back(inActor);
	pos.push_back(inActor->Pos);
	sizeCategory.push_back(inActor->getSizeCategory());
	region.push_back(CalculateRegion(inActor));
	flags.push_back(0);
	SetIsBumpable(newIdx, inActor->ValidTarget(GA_ONLY_BUMPABLE));
	SetIsAlive(newIdx, inActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED));

	return newIdx;
}

FitRegion TraversabilityCache::CachedActorsState::CalculateRegion(const Actor* inActor)
{
	const auto baseSize = inActor->CircleSize2Radius() * inActor->sizeFactor;
	const GemRB::Size s(baseSize * 8, baseSize * 6);
	return { inActor->Pos - s.Center(), s };
}

void TraversabilityCache::CachedActorsState::ClearOldPosition(const size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, const int inWidth) const
{
	const std::vector<bool>& cachedBlockingShape = GetBlockingShape(actor[i], sizeCategory[i]);
	if (cachedBlockingShape.empty()) {
		return;
	}

	const auto cachedCellState = GetCellStateFromFlags(i);
	const auto blockingShapeRegionW = GetBlockingShapeRegionW(sizeCategory[i], actor[i]->sizeFactor);
	const size_t trashIdx = inOutTraversabilityData.size() - 1;
	for (int y = 0; y < region[i].size.h; ++y) {
		for (int x = 0; x < region[i].size.w; ++x) {
			const int targetX = region[i].origin.x + x;
			const int targetY = region[i].origin.y + y;
			const size_t targetIdx = targetY * inWidth * 16 + targetX;

			// use spare cell index for invalid data: it's faster than paying fee for validating branch each iteration:
			const size_t idx = targetIdx < trashIdx ? targetIdx : trashIdx;
			TraversabilityCellData& currentTraversabilityData = inOutTraversabilityData[idx];

			const auto blockingShapeIdx = y * blockingShapeRegionW * 16 + x;
			currentTraversabilityData.state -= (cachedBlockingShape[blockingShapeIdx] * cachedCellState);

			// the following is a branchless version of zeroing `CurrentTraversabilityData.occupyingActor` if it's equal to actor[i]
			currentTraversabilityData.occupyingActor = reinterpret_cast<Actor*>(
				(currentTraversabilityData.occupyingActor != actor[i]) *
				reinterpret_cast<size_t>(currentTraversabilityData.occupyingActor));
		}
	}
}

void TraversabilityCache::CachedActorsState::MarkNewPosition(const size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf)
{
	const auto currentSizeCategory = actor[i]->getSizeCategory();
	const std::vector<bool>& currentBlockingShape = GetBlockingShape(actor[i], currentSizeCategory);
	if (currentBlockingShape.empty()) {
		return;
	}

	const size_t newActorStateIdx = AddCachedActorState(actor[i]);

	const auto currentCellState = GetCellStateFromFlags(newActorStateIdx);
	const auto blockingShapeRegionW = GetBlockingShapeRegionW(currentSizeCategory, actor[i]->sizeFactor);
	const size_t trashIdx = inOutTraversabilityData.size() - 1;
	for (int y = 0; y < region[newActorStateIdx].size.h; ++y) {
		for (int x = 0; x < region[newActorStateIdx].size.w; ++x) {
			const int targetX = region[newActorStateIdx].origin.x + x;
			const int targetY = region[newActorStateIdx].origin.y + y;
			const size_t targetIdx = targetY * inWidth * 16 + targetX;

			// use spare cell index for invalid data: it's faster than paying fee for validating branch each iteration:
			const size_t idx = targetIdx < trashIdx ? targetIdx : trashIdx;
			TraversabilityCellData& currentTraversabilityData = inOutTraversabilityData[idx];

			const auto blockingShapeIdx = y * blockingShapeRegionW * 16 + x;
			const uint16_t cellStateOfThisActor = currentBlockingShape[blockingShapeIdx] * currentCellState;
			currentTraversabilityData.state += cellStateOfThisActor;

			currentTraversabilityData.occupyingActor = cellStateOfThisActor > TraversabilityCellValueEmpty ? actor[i] : currentTraversabilityData.occupyingActor;
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
	ScopedTimer::extraTagsTracked.push_back(std::string {});
	const size_t extraTrackedIdx = ScopedTimer::extraTimeTracked.size() - 1;
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
		// if the position or the size category of the actor changed...
		if (cachedActorsState.pos[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->Pos ||
		    cachedActorsState.sizeCategory[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->getSizeCategory()) {
			// clear old and then mark new position of this actor
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->PropsSize().w);
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

	// remove from cache all the actors detected as removed from the map since last cache update
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

TraversabilityCache::TraversabilityCellState TraversabilityCache::CachedActorsState::GetCellStateFromFlags(const size_t i) const
{
	constexpr static TraversabilityCellState flagsToCellStateMapping[] {
		TraversabilityCellValueEmpty,
		TraversabilityCellValueActorNonTraversable,
		TraversabilityCellValueActor
	};

	// if it's not alive, we should mark cell as empty
	// if it's alive, we check the bumpable to determine the cell state

	// This equation produces the following values, which directly maps onto
	// index for the array above:
	// 0 - for not alive,
	// 1 - for alive and non bumpable,
	// 2 - for alive and bumpable
	const uint8_t idx = GetIsAlive(i) * (GetIsBumpable(i) + 1);
	return flagsToCellStateMapping[idx];
}

void TraversabilityCache::ValidateTraversabilityCacheSize()
{
	// append a spare cell, it will be used as a dumpster, to throw at it calculations for invalid indices;
	// it's faster than paying the fee for validation branches each iteration.
	// It's okay to have garbage cell in the data, if no one will never read from it.
	constexpr size_t spareCells = 1;
	const size_t expectedSize = map->PropsSize().h * 12 * map->PropsSize().w * 16 + spareCells;
	if (traversabilityData.size() != expectedSize) {
		traversabilityData.resize(expectedSize, TraversabilityCellData { nullptr, TraversabilityCellValueEmpty });
		memset(static_cast<void*>(traversabilityData.data()), 0, sizeof(TraversabilityCellData) * traversabilityData.size());
	}
}

const std::vector<bool>& TraversabilityCache::GetBlockingShape(const Actor* actor, const Actor::BlockingSizeCategory blockingSizeCategory)
{
	// if we don't have data with given index, fill the cache with default data up to that index;
	// it's okay to do so, blocking size category is a small integer, should be below value of 10
	if (BlockingShapeCache.size() <= static_cast<size_t>(blockingSizeCategory)) {
		BlockingShapeCache.resize(blockingSizeCategory + 1);
	}

	// if we don't have the proper data yet, calculate it
	if (BlockingShapeCache[blockingSizeCategory].empty()) {
		std::vector<bool> blockingShape;
		if (actor->sizeFactor != 0) {
			const ::GemRB::Size blockingShapeRegionSize(GetBlockingShapeRegionW(blockingSizeCategory, actor->sizeFactor), GetBlockingShapeRegionH(blockingSizeCategory, actor->sizeFactor));
			constexpr bool NotBlockingValue = false;
			blockingShape.resize(blockingShapeRegionSize.w * blockingShapeRegionSize.h * 16, NotBlockingValue);

			const FitRegion CurrentBlockingRegion = { actor->Pos - blockingShapeRegionSize.Center(), blockingShapeRegionSize };
			for (int y = 0; y < blockingShapeRegionSize.h; ++y) {
				for (int x = 0; x < blockingShapeRegionSize.w; ++x) {
					const bool ShapeMask = actor->IsOver({ x + CurrentBlockingRegion.origin.x, y + CurrentBlockingRegion.origin.y });
					const auto Idx = y * blockingShapeRegionSize.w * 16 + x;
					blockingShape[Idx] = ShapeMask;
				}
			}
		}
		BlockingShapeCache[blockingSizeCategory] = std::move(blockingShape);
	}
	return BlockingShapeCache[blockingSizeCategory];
}

uint16_t TraversabilityCache::GetBlockingShapeRegionW(const Actor::BlockingSizeCategory blockingSizeCategory, const float sizeFactor)
{
	const auto baseSize = sizeFactor * Actor::CircleSize2Radius(blockingSizeCategory);
	return baseSize * 8;
}

uint16_t TraversabilityCache::GetBlockingShapeRegionH(const Actor::BlockingSizeCategory blockingSizeCategory, const float sizeFactor)
{
	const auto baseSize = sizeFactor * Actor::CircleSize2Radius(blockingSizeCategory);
	return baseSize * 6;
}
}
