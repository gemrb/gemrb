// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_AUDIO_PLAYBACK_CONFIG
#define H_AUDIO_PLAYBACK_CONFIG

#include "globals.h"

#include "Region.h"

#include <array>

namespace GemRB {

struct AudioPoint {
	AudioPoint() = default;
	AudioPoint(const Point& point, int32_t z = 0)
		: x(point.x), y(point.y), z(z) {}
	AudioPoint(int32_t x, int32_t y, int32_t z)
		: x(x), y(y), z(z) {}

	explicit operator Point() const { return { x, y }; }

	int32_t x = 0;
	int32_t y = 0;
	int32_t z = 0;
};

struct GEM_EXPORT AudioPlaybackConfig {
	bool loop = false;
	bool efx = false;
	// 0-100 by settings
	int masterVolume = 100;
	// zero or positive by channel, fine-tuning
	int channelVolume = 100;

	bool spatial = false;
	AudioPoint position;

	uint16_t muteDistance = 1;

	bool directional = false;
	std::array<float, 3> direction = { 0.0f, 0.0f, 0.0f };
	// 0 to 360
	int32_t cone = 0;
};

}

#endif
