// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_AUDIO_SPATIAL_MONITOR
#define H_AUDIO_SPATIAL_MONITOR

#include "globals.h"

#include "AudioBackend.h"
#include "Playback.h"

#include <list>

namespace GemRB {

class Map;

class GEM_EXPORT SpatialMonitor {
public:
	SpatialMonitor();

	void AddHandleToMonitor(const Holder<PlaybackHandle>&, const ResRef&);
	void AddHandleToMonitor(const Holder<SoundSourceHandle>&, const ResRef&);
	void UpdateSoundEffects();
	void UpdateSoundForHandle(const Holder<PlaybackHandle>&);

private:
	struct SpatialHandle {
		std::weak_ptr<SoundSourceHandle> source;
		ResRef map;
	};

	bool active = false;
	std::list<SpatialHandle> monitoredHandles;

	void EvaluateOcclusion(SpatialHandle&);
	void Housekeeping();
};

}

#endif
