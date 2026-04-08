// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SpatialMonitor.h"

#include "ie_stats.h"

#include "Interface.h"
#include "Map.h"

#include "GUI/GameControl.h"

namespace GemRB {

SpatialMonitor::SpatialMonitor() : active(core->GetAudioDrv()->HasOcclusionFeature()) {}

void SpatialMonitor::AddHandleToMonitor(const Holder<PlaybackHandle>& playbackHandle, const ResRef& mapName)
{
	if (playbackHandle) {
		AddHandleToMonitor(playbackHandle->GetSourceHandle(), mapName);
	}
}

void SpatialMonitor::AddHandleToMonitor(const Holder<SoundSourceHandle>& source, const ResRef& mapName)
{
	if (!active || !source) {
		return;
	}

	if (!source->IsSpatial()) {
		return;
	}

	monitoredHandles.emplace_back(); // C++17
	auto& handle = monitoredHandles.back();
	handle.map = mapName;
	handle.source = source;

	EvaluateOcclusion(handle);
}

void SpatialMonitor::UpdateSoundEffects()
{
	if (!active) {
		return;
	}

	Housekeeping();

	for (auto& handle : monitoredHandles) {
		EvaluateOcclusion(handle);
	}
}

void SpatialMonitor::UpdateSoundForHandle(const Holder<PlaybackHandle>& playbackHandle)
{
	for (auto& handle : monitoredHandles) {
		auto sharedHandle = handle.source.lock();
		if (*sharedHandle == *playbackHandle->GetSourceHandle()) {
			EvaluateOcclusion(handle);
			break;
		}
	}
}

void SpatialMonitor::EvaluateOcclusion(SpatialHandle& handle)
{
	if (handle.source.expired()) {
		return;
	}

	auto game = core->GetGame();
	const auto& viewport = core->GetGameControl()->Viewport();
	auto source = handle.source.lock();
	auto mapName = game->CurrentArea;

	if (!handle.map.empty() && mapName != handle.map) {
		source->SetOccluded(true);
		return;
	}

	// Off screen: always occluded
	Point position = static_cast<Point>(source->GetPosition());
	if (!viewport.PointInside(position)) {
		source->SetOccluded(true);
		return;
	}

	auto map = game->GetMap(mapName, false);
	source->SetOccluded(!map->IsVisible(position));
}

void SpatialMonitor::Housekeeping()
{
	for (auto it = monitoredHandles.begin(); it != monitoredHandles.end();) {
		if (it->source.expired()) {
			it = monitoredHandles.erase(it);
		} else {
			++it;
		}
	}
}

}
