/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef AMBIENTMGRAL_H
#define AMBIENTMGRAL_H

#include "AmbientMgr.h"
#include "Region.h"
#include "globals.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace GemRB {

class Ambient;

class AmbientMgrAL final : public AmbientMgr {
public:
	AmbientMgrAL();
	~AmbientMgrAL() final;

	void activate(const std::string &name) final;
	void activate() final;
	void deactivate(const std::string &name) final;
	void deactivate() final;

	void UpdateVolume(unsigned short value);
private:
	void ambientsSet(const std::vector<Ambient *>&) final;
	
	class AmbientSource {
	public:
		explicit AmbientSource(const Ambient *a);
		~AmbientSource();
		tick_t tick(tick_t ticks, Point listener, ieDword timeslice);
		void hardStop();
		void SetVolume(unsigned short volume) const;
	private:
		int stream;
		const Ambient* ambient;
		tick_t lastticks;
		tick_t nextdelay;
		size_t nextref;
		unsigned int totalgain;

		bool isHeard(const Point &listener) const;
		tick_t enqueue() const;
	};
	std::vector<AmbientSource *> ambientSources;
	
	int play();
	tick_t tick(tick_t ticks) const;
	void hardStop() const;
	
	mutable std::recursive_mutex mutex;
	std::thread player;
	std::condition_variable_any cond;
	std::atomic_bool playing {true};
};

}

#endif /* AMBIENTMGRAL_H */
