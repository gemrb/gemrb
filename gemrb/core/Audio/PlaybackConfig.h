/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

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
