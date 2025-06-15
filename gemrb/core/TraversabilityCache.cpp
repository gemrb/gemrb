//
// Created by draghan on 01.03.25.
//

#include <Scriptable/Actor.h>
#include "Map.h"
#include "PathfindingSettings.h"

namespace GemRB {
	FCachedActorPosState::FCachedActorPosState(Actor *InActor) {
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

	Region FCachedActorPosState::CalculateRegion(const Actor *InActor) {
		const auto baseSize = InActor->CircleSize2Radius() * InActor->sizeFactor;
		const Size s(baseSize * 8, baseSize * 6);
		return {InActor->Pos - s.Center(), s};
	}

	void FCachedActorPosState::
	ClearOldPosition(std::vector<FTraversability> &InOutTraversability, int InWidth) const {
		for (int x = region.x_get(); x < region.x_get() + region.w_get(); ++x) {
			for (int y = region.y_get(); y < region.y_get() + region.h_get(); ++y) {
				if (!actor->IsOver(Point(x, y), pos)) {
					continue;
				}
				const auto Idx = y * InWidth * 16 + x;
				InOutTraversability[Idx] = FTraversability{};
			}
		}
	}

	void FCachedActorPosState::ClearNewPosition(std::vector<FTraversability> &InOutTraversability, int InWidth) const {
		const auto RemovedRegion = CalculateRegion(actor);
		for (int x = RemovedRegion.x_get(); x < RemovedRegion.x_get() + RemovedRegion.w_get(); ++x) {
			for (int y = RemovedRegion.y_get(); y < RemovedRegion.y_get() + RemovedRegion.h_get(); ++y) {
				const auto Idx = y * InWidth * 16 + x;
				InOutTraversability[Idx] = FTraversability{};
			}
		}
	}

	void FCachedActorPosState::MarkNewPosition(std::vector<FTraversability> &InOutTraversability, int InWidth,
	                                           bool bInUpdateSelf) {
		const FCachedActorPosState newState{actor};

		ETraversability NewTraversabilityType{ETraversability::actorNonTraversable};
		if (!newState.GetIsAlive()) {
			NewTraversabilityType = ETraversability::empty;
		} else if (newState.GetIsBumpable()) {
			NewTraversabilityType = ETraversability::actor;
		}
		const FTraversability NewTraversability{NewTraversabilityType, actor};

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

	void FCachedActorPosState::MarkOldPosition(std::vector<FTraversability> &InOutTraversability, int InWidth) const {
		ETraversability NewTraversabilityType{ ETraversability::actorNonTraversable };
		if (!GetIsAlive()) {
			NewTraversabilityType = ETraversability::empty;
		} else if (GetIsBumpable()) {
			NewTraversabilityType = ETraversability::actor;
		}
		const FTraversability NewTraversability{NewTraversabilityType, actor};

		for (int x = region.x_get(); x < region.x_get() + region.w_get(); ++x) {
			for (int y = region.y_get(); y < region.y_get() + region.h_get(); ++y) {
				if (!actor->IsOver(Point{x, y})) {
					continue;
				}
				const auto Idx = y * InWidth * 16 + x;
				InOutTraversability[Idx] = NewTraversability;
			}
		}
	}

	void FCachedActorPosState::UpdateNewState() {
		const FCachedActorPosState newState{actor};
		this->flags = newState.flags;
		this->pos = newState.pos;
		this->region = newState.region;
	}

	void Map::UpdateTraversabilityCache() {

		// determine all changes in actors' state regarding their alive status, bumpable status and position, also check for new and removed actors
		std::vector<size_t> ActorsRemoved;
		std::vector<std::vector<FCachedActorPosState>::iterator> ActorsUpdated;
		std::vector<FCachedActorPosState> ActorsNew;

		// for all actors in the map...
		for (auto currentActor: actors) {

			// find this actor in cache
			const auto Found = std::find_if(CachedActorPosState.begin(), CachedActorPosState.end(),
			                                [currentActor](const FCachedActorPosState &CachedActor) {
				                                return CachedActor.actor == currentActor;
			                                });

			// if not found, it's a new actor
			if (Found == CachedActorPosState.cend()) {
				ActorsNew.emplace_back(currentActor);
				continue;
			}

			// if found, check if the position, bumpable status and alive status have been updated since last cache update
			const FCachedActorPosState &CachedActor = *Found;
			if (CachedActor.pos != currentActor->Pos ||
			    CachedActor.GetIsAlive() != currentActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) ||
			    CachedActor.GetIsBumpable() != currentActor->ValidTarget(GA_ONLY_BUMPABLE)) {
				ActorsUpdated.push_back(Found);
			}
		}

		// find all actors from cache which are not among the map actors and store their index to remove them later
		for (size_t i = 0; i < CachedActorPosState.size(); ++i) {
			const auto &CachedActorPos = CachedActorPosState[i];
			const auto Found = std::find_if(actors.cbegin(), actors.cend(), [&CachedActorPos](const Actor *actor) {
				return CachedActorPos.actor == actor;
			});
			if (Found == actors.cend()) {
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
			CachedActorPosState[RemovedIndex].ClearOldPosition(Traversability, PropsSize().w);
		}

		// for all updated actors: make necessary changes based on the status change
		for (auto IteratorUpdated: ActorsUpdated) {
			FCachedActorPosState &Updated = *IteratorUpdated;

			// if the position of the actor changed...
			if (Updated.pos != Updated.actor->Pos) {
				// clear old position of this actor
				Updated.ClearOldPosition(Traversability, PropsSize().w);

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
				Updated.MarkNewPosition(Traversability, PropsSize().w, true);
				continue;
			}

			// if our actor did go from dead to alive...
			if (!Updated.GetIsAlive() && (Updated.GetIsAlive() != Updated.actor->ValidTarget(
				                              GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// no need to clear old position, just mark new position
				Updated.MarkNewPosition(Traversability, PropsSize().w, true);
			}
			// if our actor did go from alive to dead...
			else if (Updated.GetIsAlive() && (Updated.GetIsAlive() != Updated.actor->ValidTarget(
				                                  GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// just clear old position
				Updated.ClearOldPosition(Traversability, PropsSize().w);
				Updated.UpdateNewState();
			}

			// if our actor did change its bumpable state
			if (Updated.GetIsBumpable() != Updated.actor->ValidTarget(GA_ONLY_BUMPABLE)) {
				// clear old cells and mark new cells
				Updated.ClearOldPosition(Traversability, PropsSize().w);
				Updated.MarkNewPosition(Traversability, PropsSize().w, true);
			}
		}

		// for any new actors, just mark their new position
		for (auto &New: ActorsNew) {
			New.MarkNewPosition(Traversability, PropsSize().w);
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

	void Map::ValidateTraversabilityCacheSize() {
		const auto ExpectedSize = PropsSize().h * 12 * PropsSize().w * 16;
		if (Traversability.size() != ExpectedSize) {
			Log(WARNING, "Map", "Resizing Traversability table.");
			Traversability.resize(ExpectedSize, FTraversability{ETraversability::empty, nullptr});
			memset(Traversability.data(), 0, sizeof(FTraversability) * Traversability.size());
		}
	}
}
