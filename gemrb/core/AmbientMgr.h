/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
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
 *
 */

#ifndef AMBIENTMGR_H
#define AMBIENTMGR_H

#include "exports.h"
#include "globals.h"

#include "Region.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace GemRB {

class Ambient;

class GEM_EXPORT AmbientMgr {
public:
	AmbientMgr();
	~AmbientMgr();

	void reset();
	void UpdateVolume(unsigned short value);
	void setAmbients(const std::vector<Ambient *> &a);

	void activate(const std::string &name); // hard play ;-)
	void activate();
	void deactivate(const std::string &name); // hard stop
	void deactivate();
	bool isActive(const std::string &name) const;

private:
	void ambientsSet(const std::vector<Ambient *>&);

private:
	mutable std::mutex ambientsMutex;
	std::vector<Ambient *> ambients;
	std::atomic_bool active {false};

	mutable std::recursive_mutex mutex;
	std::thread player;
	std::condition_variable_any cond;
	std::atomic_bool playing {true};

	class AmbientSource {
	public:
		explicit AmbientSource(const Ambient *a) : ambient(a) {};
		AmbientSource(const AmbientSource&) = delete;
		~AmbientSource();
		AmbientSource& operator=(const AmbientSource&) = delete;
		tick_t tick(tick_t ticks, Point listener, ieDword timeslice);
		void hardStop();
		void SetVolume(unsigned short volume) const;
	private:
		int stream = -1;
		const Ambient* ambient;
		tick_t lastticks = 0;
		tick_t nextdelay = 0;
		size_t nextref = 0;
		unsigned int totalgain = 0;

		bool isHeard(const Point &listener) const;
		tick_t enqueue() const;
	};
	std::vector<AmbientSource*> ambientSources;

	int play();
	tick_t tick(tick_t ticks) const;
	void hardStop() const;
};

}

#endif
