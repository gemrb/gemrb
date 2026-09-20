// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later


#include "TraversabilityCache.h"

#include "Map.h"

#include "Scriptable/Actor.h"

namespace GemRB {

// The cache stores one entry per searchmap tile. An actor's footprint is still a pixel-space
// circle, so stamping marks every tile whose centre falls inside it.
namespace {

	constexpr int TileW = SEARCHMAP_SQUARE_WIDTH;
	constexpr int TileH = SEARCHMAP_SQUARE_HEIGHT;

	// Integer division rounding down, including for negatives: a tile bound derived from a
	// negative pixel coordinate must stay negative rather than fold onto 0.
	int FloorDiv(int a, int b)
	{
		return a >= 0 ? a / b : -((-a + b - 1) / b);
	}

	// Integer division rounding up, including for negatives (truncation already rounds up there).
	int CeilDiv(int a, int b)
	{
		return a >= 0 ? (a + b - 1) / b : a / b;
	}
}

// Returns the tiles whose centre can fall inside the actor's pixel footprint, clamped to the map.
TraversabilityCache::ActorFootprint ComputeActorFootprint(const Point& pos, int circleSize, int mapWidth, int mapHeight)
{
	const int baseSize = Selectable::CircleSize2Radius(circleSize);
	const int halfW = baseSize * 4;
	const int halfH = baseSize * 3;
	// The ground ellipse reaches halfW/halfH on both sides. The shape is even-sized and half-open,
	// so for sizes 2 and up the far row/column (at +halfW/+halfH) has to be included explicitly -
	// movement's IsWithinEllipse does include it, and a pathfinder that leaves it out routes the
	// walker onto a tile the collision test then refuses. Size 1 keeps its original box.
	const int includeFarEdge = circleSize < 2 ? 0 : 1;
	const int originX = pos.x - halfW;
	const int originY = pos.y - halfH;
	const int sizeW = 2 * halfW + includeFarEdge;
	const int sizeH = 2 * halfH + includeFarEdge;

	return {
		std::max(0, CeilDiv(originX - TileW / 2, TileW)),
		std::min(mapWidth - 1, FloorDiv(originX + sizeW - 1 - TileW / 2, TileW)),
		std::max(0, CeilDiv(originY - TileH / 2, TileH)),
		std::min(mapHeight - 1, FloorDiv(originY + sizeH - 1 - TileH / 2, TileH)),
		pos,
		circleSize - 1
	};
}

// C++14 needs a definition for a static constexpr member that gets odr-used, which is
// what happens as soon as one is bound to a reference outside this file;
// could be removed when moving to C++17 with inlined static constexpr members
constexpr TraversabilityCache::TraversabilityCellState TraversabilityCache::TraversabilityCellValueEmpty;
constexpr TraversabilityCache::TraversabilityCellState TraversabilityCache::TraversabilityCellValueActor;
constexpr TraversabilityCache::TraversabilityCellState TraversabilityCache::TraversabilityCellValueActorNonTraversable;

size_t TraversabilityCache::CachedActorsState::AddCachedActorState(Actor* inActor)
{
	if (!inActor) {
		return -1;
	}

	const size_t newIdx = actor.size();

	actor.push_back(inActor);
	pos.push_back(inActor->Pos);
	sizeCategory.push_back(inActor->getSizeCategory());
	flags.push_back(0);
	SetIsBumpable(newIdx, inActor->ValidTarget(GA_ONLY_BUMPABLE));
	SetIsAlive(newIdx, inActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED));

	return newIdx;
}

void TraversabilityCache::CachedActorsState::ClearOldPosition(const size_t i, Data_t& inOutTraversabilityData, const int inWidth) const
{
	const auto cachedCellState = GetCellStateFromFlags(i);
	const size_t mapCells = inOutTraversabilityData.size();
	const int mapHeight = static_cast<int>((mapCells - 1) / static_cast<size_t>(inWidth));
	const ActorFootprint footprint = ComputeActorFootprint(pos[i], sizeCategory[i], inWidth, mapHeight);

	for (int ty = footprint.firstY; ty <= footprint.lastY; ++ty) {
		const size_t rowBase = static_cast<size_t>(ty) * inWidth;
		for (int tx = footprint.firstX; tx <= footprint.lastX; ++tx) {
			if (!footprint.covers(SearchmapPoint(tx, ty))) continue;
			const size_t idx = rowBase + tx;

			inOutTraversabilityData[idx] = static_cast<TraversabilityCellState>(inOutTraversabilityData[idx] - cachedCellState);
		}
	}
}

void TraversabilityCache::CachedActorsState::MarkNewPosition(const size_t i, Data_t& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf)
{
	const size_t newActorStateIdx = AddCachedActorState(actor[i]);
	const auto currentCellState = GetCellStateFromFlags(newActorStateIdx);
	const int circleSize = sizeCategory[newActorStateIdx];
	const size_t mapCells = inOutTraversabilityData.size();
	const int mapHeight = static_cast<int>((mapCells - 1) / static_cast<size_t>(inWidth));
	const ActorFootprint footprint = ComputeActorFootprint(actor[i]->Pos, circleSize, inWidth, mapHeight);

	for (int ty = footprint.firstY; ty <= footprint.lastY; ++ty) {
		const size_t rowBase = static_cast<size_t>(ty) * inWidth;
		for (int tx = footprint.firstX; tx <= footprint.lastX; ++tx) {
			if (!footprint.covers(SearchmapPoint(tx, ty))) continue;
			const size_t idx = rowBase + tx;

			inOutTraversabilityData[idx] = static_cast<TraversabilityCellState>(inOutTraversabilityData[idx] + currentCellState);
		}
	}

	if (inShouldUpdateSelf) {
		flags[i] = flags[newActorStateIdx];
		pos[i] = pos[newActorStateIdx];
		sizeCategory[i] = sizeCategory[newActorStateIdx];
	}

	erase(newActorStateIdx);
}

void TraversabilityCache::CachedActorsState::UpdateNewState(const size_t i)
{
	const size_t newActorStateIdx = AddCachedActorState(actor[i]);
	flags[i] = flags[newActorStateIdx];
	pos[i] = pos[newActorStateIdx];
	sizeCategory[i] = sizeCategory[newActorStateIdx];
	erase(newActorStateIdx);
}

bool TraversabilityCache::Update()
{
	// this cache is updated once per frame and only if any path was requested
	if (hasBeenUpdatedThisFrame) {
		return false;
	}
	hasBeenUpdatedThisFrame = true;

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
		return false;
	}

	// todo: this probably could be done once, when the map is loaded
	// make sure the cache is of proper size
	ValidateTraversabilityCacheSize();

	// for all removed actors: clear in cache all the cells they were part of
	for (const auto removedCachedActorIndex : actorsRemoved) {
		cachedActorsState.ClearOldPosition(removedCachedActorIndex, traversabilityData, map->tileProps.GetSize().w);
	}

	// for all updated actors: make necessary changes based on the status change
	for (auto updatedCachedIdx : actorsUpdated) {
		// if the position or the size category of the actor changed...
		if (cachedActorsState.pos[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->Pos ||
		    cachedActorsState.sizeCategory[updatedCachedIdx] != cachedActorsState.actor[updatedCachedIdx]->getSizeCategory()) {
			// clear old and then mark new position of this actor
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->tileProps.GetSize().w);
			cachedActorsState.MarkNewPosition(updatedCachedIdx, traversabilityData, map->tileProps.GetSize().w, true);
			continue;
		}

		// if our actor did go from dead to alive...
		if (!cachedActorsState.GetIsAlive(updatedCachedIdx) && (cachedActorsState.GetIsAlive(updatedCachedIdx) != cachedActorsState.actor[updatedCachedIdx]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
			// no need to clear old position, just mark new position
			cachedActorsState.MarkNewPosition(updatedCachedIdx, traversabilityData, map->tileProps.GetSize().w, true);
		}
		// if our actor did go from alive to dead...
		else if (cachedActorsState.GetIsAlive(updatedCachedIdx) && (cachedActorsState.GetIsAlive(updatedCachedIdx) != cachedActorsState.actor[updatedCachedIdx]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
			// just clear old position
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->tileProps.GetSize().w);
			cachedActorsState.UpdateNewState(updatedCachedIdx);
		}

		// if our actor did change its bumpable state
		if (cachedActorsState.GetIsBumpable(updatedCachedIdx) != cachedActorsState.actor[updatedCachedIdx]->ValidTarget(GA_ONLY_BUMPABLE)) {
			// clear old cells and mark new cells
			cachedActorsState.ClearOldPosition(updatedCachedIdx, traversabilityData, map->tileProps.GetSize().w);
			cachedActorsState.MarkNewPosition(updatedCachedIdx, traversabilityData, map->tileProps.GetSize().w, true);
		}
	}

	// for any new actors, just mark their new position
	for (size_t i = 0; i < actorsNew.actor.size(); ++i) {
		actorsNew.MarkNewPosition(i, traversabilityData, map->tileProps.GetSize().w);
	}

	// remove from cache all the actors detected as removed from the map since last cache update
	for (auto iteratorRemoved = actorsRemoved.crbegin(); iteratorRemoved != actorsRemoved.crend(); ++iteratorRemoved) {
		const size_t removedIdx = *iteratorRemoved;
		cachedActorsState.erase(removedIdx);
	}

	// add to cache all the actors detected as new on the map since last cache update
	cachedActorsState.emplace_back(std::move(actorsNew));

	return true;
}

TraversabilityCache::CachedActorsState::CachedActorsState(const size_t reserve)
{
	if (reserve != 0) {
		this->reserve(reserve);
	}
}

void TraversabilityCache::CachedActorsState::reserve(const size_t reserve)
{
	actor.reserve(reserve);
	pos.reserve(reserve);
	flags.reserve(reserve);
	sizeCategory.reserve(reserve);
}

void TraversabilityCache::CachedActorsState::clear()
{
	actor.clear();
	pos.clear();
	flags.clear();
	sizeCategory.clear();
}

void TraversabilityCache::CachedActorsState::erase(const size_t idx)
{
	actor.erase(actor.begin() + idx);
	pos.erase(pos.begin() + idx);
	flags.erase(flags.begin() + idx);
	sizeCategory.erase(sizeCategory.begin() + idx);
}

void TraversabilityCache::CachedActorsState::emplace_back(CachedActorsState&& another)
{
	reserve(actor.size() + another.actor.size());
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
	const uint8_t idx = static_cast<uint8_t>(GetIsAlive(i)) * (static_cast<uint8_t>(GetIsBumpable(i)) + 1);
	return flagsToCellStateMapping[idx];
}

void TraversabilityCache::ValidateTraversabilityCacheSize()
{
	// append a spare cell, it will be used as a dumpster, to throw at it calculations for invalid indices;
	// it's faster than paying the fee for validation branches each iteration.
	// It's okay to have garbage cell in the data, if no one will never read from it.
	constexpr size_t spareCells = 1;
	const size_t expectedSize = map->tileProps.GetSize().h * map->tileProps.GetSize().w + spareCells;
	if (traversabilityData.size() != expectedSize) {
		traversabilityData.assign(expectedSize, TraversabilityCellValueEmpty);
	}
}
}
