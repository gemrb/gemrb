//
// Created by draghan on 01.03.25.
//

#include <Scriptable/Actor.h>
#include "Map.h"
#include "PathfindingSettings.h"

namespace GemRB {

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
	if (actors.size() != CachedTraversability.size()) {
		return true;
	}
	for (size_t i = 0; i < actors.size(); ++i) {
		if (actors[i] != CachedTraversability[i].actor ||
			actors[i]->Pos != CachedTraversability[i].pos ||
			actors[i]->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED) != CachedTraversability[i].alive ||
			actors[i]->ValidTarget(GA_ONLY_BUMPABLE) != CachedTraversability[i].bumpable
			) {
			return true;
			}
	}
	return false;
#endif
}

void Map::UpdateTraversability() const {
#if PATH_RUN_IMPROVED
	ScopedTimer t("}} cache ");
	if (!ShouldUpdateTraversability()) {
		return;
	}
	// Log(WARNING, "Map", "...Updating traversability...");
	ValidateTraversabilitySize();
	memset(Traversability.data(), 0, sizeof(FTraversability) * Traversability.size());

	// CachedTraversability.clear();
	CachedTraversability.reserve(actors.size());
	memset(CachedTraversability.data(), 0, sizeof(FCachedActorPosState) * actors.size());
	for (const auto actor : actors) {

		Point ActorPos;
		int alive = 0;
		int bumpable = 0;
		if (actor) {
			ActorPos = actor->Pos;
			alive = actor->ValidTarget(GA_NO_UNSCHEDULED | GA_NO_DEAD);
			bumpable = actor->ValidTarget(GA_ONLY_BUMPABLE);
		}
		CachedTraversability.push_back({actor, ActorPos, bumpable, alive});

		if (!actor) {
			continue;
		}
		TraversabilityBlock(actor);
	}
#endif
}

void Map::ValidateTraversabilitySize() const  {
#if PATH_RUN_IMPROVED
	const auto ExpectedSize = PropsSize().h * 12 * PropsSize().w * 16;
	if (Traversability.size() != ExpectedSize) {
		Log(WARNING, "Map", "Resizing Traversability table.");
		Traversability.resize(ExpectedSize, FTraversability{ ETraversability::empty, nullptr} );
	}
#endif
}
}