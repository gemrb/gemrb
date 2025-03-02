//
// Created by draghan on 01.03.25.
//

#include <Scriptable/Actor.h>
#include "Map.h"
#include "PathfindingSettings.h"

namespace GemRB {


Map::FCachedActorPosState::FCachedActorPosState(Actor *InActor) {
	if (!InActor) {
		return;
	}

	actor = InActor;
	pos = InActor->Pos;
	bumpable = InActor->ValidTarget(GA_ONLY_BUMPABLE);
	alive = InActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED);
	region = CalculateRegion(InActor);
}

Region Map::FCachedActorPosState::CalculateRegion(Actor* InActor) {
	const auto baseSize = InActor->CircleSize2Radius() * InActor->sizeFactor;
	const Size s(baseSize * 8, baseSize * 6);
	return {InActor->Pos - s.Center(), s};
}

void Map::FCachedActorPosState::ClearOldPosition(std::vector<FTraversability> &InOutTraversability, int InWidth) const {
	const auto& RemovedRegion = region;
	for (int x = RemovedRegion.x; x < RemovedRegion.x + RemovedRegion.w; ++x) {
		for (int y = RemovedRegion.y; y < RemovedRegion.y + RemovedRegion.h; ++y) {
			const auto Idx = y * InWidth * 16 + x;
			InOutTraversability[Idx] = FTraversability{};
		}
	}
}

void Map::FCachedActorPosState::
ClearOldPosition2(std::vector<FTraversability> &InOutTraversability, int InWidth) const {
	const auto& RemovedRegion = region;
	for (int x = RemovedRegion.x; x < RemovedRegion.x + RemovedRegion.w; ++x) {
		for (int y = RemovedRegion.y; y < RemovedRegion.y + RemovedRegion.h; ++y) {
			if (!actor->IsOver(Point(x, y), pos)) {
				continue;
			}
			const auto Idx = y * InWidth * 16 + x;
			InOutTraversability[Idx] = FTraversability{};
		}
	}
}

void Map::FCachedActorPosState::ClearNewPosition(std::vector<FTraversability> &InOutTraversability, int InWidth) const {
	const auto RemovedRegion = CalculateRegion(actor);
	for (int x = RemovedRegion.x; x < RemovedRegion.x + RemovedRegion.w; ++x) {
		for (int y = RemovedRegion.y; y < RemovedRegion.y + RemovedRegion.h; ++y) {
			const auto Idx = y * InWidth * 16 + x;
			InOutTraversability[Idx] = FTraversability{};
		}
	}
}

void Map::FCachedActorPosState::MarkNewPosition(std::vector<FTraversability> &InOutTraversability, int InWidth, bool bInUpdateSelf) {
	const FCachedActorPosState newState{actor};
	ETraversability NewTraversabilityType;
	if (!newState.alive) {
		NewTraversabilityType = ETraversability::empty;
	}
	else if (newState.bumpable) {
		NewTraversabilityType = ETraversability::actor;
	}
	else {
		NewTraversabilityType = ETraversability::actorNonTraversable;
	}

	const FTraversability NewTraversability{NewTraversabilityType, actor};
	const auto& NewRegion = newState.region;
	for (int x = NewRegion.x; x < NewRegion.x + NewRegion.w; ++x) {
		for (int y = NewRegion.y; y < NewRegion.y + NewRegion.h; ++y) {
			if (!actor->IsOver(Point{x, y})) {
				continue;
			}
			const auto Idx = y * InWidth * 16 + x;
			InOutTraversability[Idx] = NewTraversability;
		}
	}
	if (bInUpdateSelf) {
		this->alive = newState.alive;
		this->bumpable = newState.bumpable;
		this->pos = newState.pos;
		this->region = newState.region;
	}
}

void Map::FCachedActorPosState::MarkOldPosition(std::vector<FTraversability> &InOutTraversability, int InWidth) {
	ETraversability NewTraversabilityType;
	if (!alive) {
		NewTraversabilityType = ETraversability::empty;
	}
	else if (bumpable) {
		NewTraversabilityType = ETraversability::actor;
	}
	else {
		NewTraversabilityType = ETraversability::actorNonTraversable;
	}

	const FTraversability NewTraversability{NewTraversabilityType, actor};
	const auto& NewRegion = region;
	for (int x = NewRegion.x; x < NewRegion.x + NewRegion.w; ++x) {
		for (int y = NewRegion.y; y < NewRegion.y + NewRegion.h; ++y) {
			if (!actor->IsOver(Point{x, y})) {
				continue;
			}
			const auto Idx = y * InWidth * 16 + x;
			InOutTraversability[Idx] = NewTraversability;
		}
	}
}

void Map::FCachedActorPosState::UpdateNewState() {
	const FCachedActorPosState newState{actor};
	this->alive = newState.alive;
	this->bumpable = newState.bumpable;
	this->pos = newState.pos;
	this->region = newState.region;
}


Region Map::GetTraversabilityRegion(Actor *actor) const {
	auto baseSize = actor->CircleSize2Radius() * actor->sizeFactor;
	const Size s(baseSize * 8, baseSize * 6);
	const Region r(actor->Pos - s.Center(), s);

	return r;
	//
	// std::vector<Region> RetVal;
	//
	// // RetVal.push_back(r);
	// // return RetVal;
	//
	// for (int x = r.x; x < r.x + r.w; ++x) {
	// 	for (int y = r.y; y < r.y + r.h; ++y) {
	// 		const Point CurrentPoint{x, y};
	// 		if (!actor->IsOver(CurrentPoint)) {
	// 			continue;
	// 		}
	// 		const SearchmapPoint BeginPoint{CurrentPoint};
	// 		const NavmapPoint BeginPointNavmap = BeginPoint.ToNavmapPoint();
	// 		RetVal.push_back(Region{BeginPointNavmap, Size{16, 12}});
	// 		// x = BeginPointNavmap.x + 16;
	// 		y = BeginPointNavmap.y + 12;
	// 	}
	// }
	// return RetVal;
}


void Map::TraversabilityUnblock(Actor *actor) {
#if PATH_RUN_IMPROVED
	ValidateTraversabilitySize();
	// const auto Regions = GetTraversabilityRegion(actor);
	//
	// for (const auto& r : Regions) {
	// 	for (int x = r.x; x < r.x + r.w; ++x)
	// 	{
	// 		for (int y = r.y; y < r.y + r.h; ++y)
	// 		{
	// 			if (x < 0 || y < 0)
	// 			{
	// 				continue;
	// 			}
	// 			const auto Idx = y * PropsSize().w * 16 + x;
	// 			if (Idx < Traversability.size()) {
	// 				Traversability[Idx].type = ETraversability::empty;
	// 			}
	// 		}
	// 	}
	// }
#endif
}

void Map::TraversabilityBlock(Actor *actor) const {
#if PATH_RUN_IMPROVED
	const bool bIsTraversable = actor->IsTraversable();
	const auto r = GetTraversabilityRegion(actor);

	// for (const auto& r : Regions)
	{
		int allPoints = 0;
		int selectedPoints = 0;
		int d0 = 0;
		int d1 = 0;
		int d2 = 0;
		if (!actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED)) {
			return;
		}
		for (int x = r.x; x < r.x + r.w; ++x)
		{
			for (int y = r.y; y < r.y + r.h; ++y)
			{
				// ++allPoints;
				if (x < 0 || y < 0)
				{
					// ++d0;
					continue;
				}
				if (!actor->IsOver(Point(x, y))) {
					// ++d1;
					continue;
				}
				// ++selectedPoints;
				const auto Idx = y * PropsSize().w * 16 + x;
				if (Idx < Traversability.size()) {
					Traversability[Idx].type = bIsTraversable ? ETraversability::actor : ETraversability::actorNonTraversable;
					Traversability[Idx].actor = actor;
				}
			}
		}
		// std::cout << "All points: " << allPoints << ", selected points: " << selectedPoints << ", discarded points: " << allPoints - selectedPoints << ", discarded0: " << d0 << ", discarded1: " << d1 << std::endl;
	}
#endif
}

bool Map::ShouldUpdateTraversability() const {
#if PATH_RUN_IMPROVED
	if (actors.size() != CachedActorPosState.size()) {
		return true;
	}
	for (size_t i = 0; i < actors.size(); ++i) {
		if (actors[i] != CachedActorPosState[i].actor ||
			actors[i]->Pos != CachedActorPosState[i].pos ||
			actors[i]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) != CachedActorPosState[i].alive ||
			actors[i]->ValidTarget(GA_ONLY_BUMPABLE) != CachedActorPosState[i].bumpable
			)
			{
				return true;
			}
	}
	return false;
#endif
}

bool Map::ShouldUpdateTraversability2() const {
#if PATH_RUN_IMPROVED
	if (actors.size() != CachedActorPosState2.size()) {
		return true;
	}
	for (size_t i = 0; i < actors.size(); ++i) {
		if (actors[i] != CachedActorPosState2[i].actor ||
			actors[i]->Pos != CachedActorPosState2[i].pos ||
			actors[i]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) != CachedActorPosState2[i].alive ||
			actors[i]->ValidTarget(GA_ONLY_BUMPABLE) != CachedActorPosState2[i].bumpable
			)
		{
			return true;
		}
	}
	return false;
#endif
}

void Map::UpdateTraversability() const {
#if PATH_RUN_IMPROVED
	UpdateTraversabilitySmort();
	UpdateTraversabilityDumb();
#endif
}

int UpdateCounter = 0;
void Map::UpdateTraversabilitySmort() const {
	ScopedTimer t("}} cacheS ");

	const bool bForceRefresh = UpdateCounter == 0;
	++UpdateCounter;
	if (UpdateCounter >= 60) {
		UpdateCounter = 0;
	}

	std::vector<size_t> ActorsRemoved;
	std::vector<std::vector<FCachedActorPosState>::iterator> ActorsUpdated;
	std::vector<FCachedActorPosState> ActorsNew;

	if (bForceRefresh) {
		// Log(WARNING, "Map", "Force cache refresh");
		ActorsRemoved.reserve(CachedActorPosState.size());
		ActorsNew.reserve(CachedActorPosState.size());
		for (size_t i = 0; i < CachedActorPosState.size(); ++i) {
			ActorsRemoved.push_back(i);
		}
		for (auto& actor : actors) {
			ActorsNew.emplace_back(actor);
		}
	}
	else {
		for (size_t i = 0; i < actors.size(); ++i) {
			auto currentActor = actors[i];
			const auto Found = std::find_if(CachedActorPosState.begin(), CachedActorPosState.end(), [currentActor](const FCachedActorPosState& CachedActor) {
				return CachedActor.actor == currentActor;
			});
			if (Found == CachedActorPosState.cend()) {
				ActorsNew.emplace_back(currentActor);
				continue;
			}

			const FCachedActorPosState& CachedActor = *Found;
			if (CachedActor.pos != currentActor->Pos ||
				CachedActor.alive != currentActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) ||
				CachedActor.bumpable != currentActor->ValidTarget(GA_ONLY_BUMPABLE)) {
				ActorsUpdated.push_back(Found);
			}
		}

		for (size_t i = 0; i < CachedActorPosState.size(); ++i) {
			const auto& CachedActorPos = CachedActorPosState[i];
			const auto Found = std::find_if(actors.cbegin(), actors.cend(), [&CachedActorPos](const Actor* actor) {
				return CachedActorPos.actor == actor;
			});
			if (Found == actors.cend()) {
				ActorsRemoved.push_back(i);
			}
		}
	}


	if (ActorsNew.empty() && ActorsRemoved.empty() && ActorsUpdated.empty()) {
		return;
	}

	// Log(WARNING, "Map", "...Updating traversability...");
	ValidateTraversabilitySize();

	for (const auto RemovedIndex : ActorsRemoved) {
		const auto RemovedRegion = CachedActorPosState[RemovedIndex].region;
		for (int x = RemovedRegion.x; x < RemovedRegion.x + RemovedRegion.w; ++x) {
			for (int y = RemovedRegion.y; y < RemovedRegion.y + RemovedRegion.h; ++y) {
				const auto Idx = y * PropsSize().w * 16 + x;
				Traversability[Idx] = FTraversability{};
			}
		}
	}

	for (auto IteratorUpdated : ActorsUpdated) {
		FCachedActorPosState& Updated = *IteratorUpdated;
		if (Updated.pos != Updated.actor->Pos) {
			// clear old position, mark new position
			Updated.ClearOldPosition(Traversability, PropsSize().w);
			Updated.MarkNewPosition(Traversability, PropsSize().w, true);
			continue;
		}
		else {
			// go from dead to alive
			if (!Updated.alive && (Updated.alive != Updated.actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// no need to clear old position, just mark new position
				Updated.MarkNewPosition(Traversability, PropsSize().w, true);
			}
			// go from alive to dead
			else if (Updated.alive && (Updated.alive != Updated.actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// just clear old position
				Updated.ClearOldPosition(Traversability, PropsSize().w);
				Updated.UpdateNewState();
			}

			// change in bumpable
			if (Updated.bumpable != Updated.actor->ValidTarget(GA_ONLY_BUMPABLE)) {
				// clear old, mark new
				Updated.ClearOldPosition(Traversability, PropsSize().w);
				Updated.MarkNewPosition(Traversability, PropsSize().w, true);
			}
		}
	}

	for (auto & New : ActorsNew) {
		New.MarkNewPosition(Traversability, PropsSize().w);
	}

	for (auto RemovedIt = ActorsRemoved.crbegin(); RemovedIt != ActorsRemoved.crend(); ++RemovedIt) {
		size_t RemovedIdx = *RemovedIt;
		CachedActorPosState.erase(CachedActorPosState.begin() + RemovedIdx);
	}

	for (auto& Added : ActorsNew) {
		CachedActorPosState.emplace_back(std::move(Added));
	}
}

void Map::UpdateTraversabilityDumb() const {
	ScopedTimer t("}} cacheD ");

	std::vector<size_t> ActorsRemoved;
	std::vector<std::vector<FCachedActorPosState>::iterator> ActorsUpdated;
	std::vector<FCachedActorPosState> ActorsNew;

	{
		for (size_t i = 0; i < actors.size(); ++i) {
			auto currentActor = actors[i];
			const auto Found = std::find_if(CachedActorPosState2.begin(), CachedActorPosState2.end(), [currentActor](const FCachedActorPosState& CachedActor) {
				return CachedActor.actor == currentActor;
			});
			if (Found == CachedActorPosState2.cend()) {
				ActorsNew.emplace_back(currentActor);
				continue;
			}

			const FCachedActorPosState& CachedActor = *Found;
			if (CachedActor.pos != currentActor->Pos ||
				CachedActor.alive != currentActor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) ||
				CachedActor.bumpable != currentActor->ValidTarget(GA_ONLY_BUMPABLE)) {
				ActorsUpdated.push_back(Found);
			}
		}

		for (size_t i = 0; i < CachedActorPosState2.size(); ++i) {
			const auto& CachedActorPos = CachedActorPosState2[i];
			const auto Found = std::find_if(actors.cbegin(), actors.cend(), [&CachedActorPos](const Actor* actor) {
				return CachedActorPos.actor == actor;
			});
			if (Found == actors.cend()) {
				ActorsRemoved.push_back(i);
			}
		}
	}


	if (ActorsNew.empty() && ActorsRemoved.empty() && ActorsUpdated.empty()) {
		return;
	}

	// Log(WARNING, "Map", "...Updating Traversability2...");
	ValidateTraversabilitySize2();

	for (const auto RemovedIndex : ActorsRemoved) {
		CachedActorPosState2[RemovedIndex].ClearOldPosition2(Traversability2, PropsSize().w);
	}

	for (auto IteratorUpdated : ActorsUpdated) {
		FCachedActorPosState& Updated = *IteratorUpdated;
		if (Updated.pos != Updated.actor->Pos) {
			// clear old position, mark new position
			Updated.ClearOldPosition2(Traversability2, PropsSize().w);
			for (int i = 0; i < CachedActorPosState2.size(); ++i) {
				if (CachedActorPosState2[i].actor == Updated.actor) {
					continue;
				}
				if (CachedActorPosState2[i].region.IntersectsRegion(Updated.region)) {
					CachedActorPosState2[i].bumpable = !static_cast<bool>(CachedActorPosState2[i].bumpable);
				}
			}
			Updated.MarkNewPosition(Traversability2, PropsSize().w, true);
			continue;
		}
		else {
			// go from dead to alive
			if (!Updated.alive && (Updated.alive != Updated.actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// no need to clear old position, just mark new position
				Updated.MarkNewPosition(Traversability2, PropsSize().w, true);
			}
			// go from alive to dead
			else if (Updated.alive && (Updated.alive != Updated.actor->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED))) {
				// just clear old position
				Updated.ClearOldPosition2(Traversability2, PropsSize().w);
				Updated.UpdateNewState();
			}

			// change in bumpable
			if (Updated.bumpable != Updated.actor->ValidTarget(GA_ONLY_BUMPABLE)) {
				// clear old, mark new
				Updated.ClearOldPosition2(Traversability2, PropsSize().w);
				Updated.MarkNewPosition(Traversability2, PropsSize().w, true);
			}
		}
	}

	for (auto & New : ActorsNew) {
		New.MarkNewPosition(Traversability2, PropsSize().w);
	}

	for (auto RemovedIt = ActorsRemoved.crbegin(); RemovedIt != ActorsRemoved.crend(); ++RemovedIt) {
		size_t RemovedIdx = *RemovedIt;
		CachedActorPosState2.erase(CachedActorPosState2.begin() + RemovedIdx);
	}

	for (auto& Added : ActorsNew) {
		CachedActorPosState2.emplace_back(std::move(Added));
	}
}

void Map::ValidateTraversabilitySize() const  {
#if PATH_RUN_IMPROVED
	const auto ExpectedSize = PropsSize().h * 12 * PropsSize().w * 16;
	if (Traversability.size() != ExpectedSize) {
		Log(WARNING, "Map", "Resizing Traversability table.");
		Traversability.resize(ExpectedSize, FTraversability{ ETraversability::empty, nullptr} );
		memset(Traversability.data(), 0, sizeof(FTraversability) * Traversability.size());
	}
#endif
}

void Map::ValidateTraversabilitySize2() const {
#if PATH_RUN_IMPROVED
	const auto ExpectedSize = PropsSize().h * 12 * PropsSize().w * 16;
	if (Traversability2.size() != ExpectedSize) {
		Log(WARNING, "Map", "Resizing Traversability table.");
		Traversability2.resize(ExpectedSize, FTraversability{ ETraversability::empty, nullptr} );
		memset(Traversability2.data(), 0, sizeof(FTraversability) * Traversability2.size());
	}
#endif
}
}
