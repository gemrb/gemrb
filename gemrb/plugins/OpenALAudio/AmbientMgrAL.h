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

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace GemRB {

class Ambient;

class AmbientMgrAL : public AmbientMgr {
public:
	AmbientMgrAL() : AmbientMgr(), player(nullptr) { }
	~AmbientMgrAL();

	void setAmbients(const std::vector<Ambient *> &a);
	void activate(const std::string &name);
	void activate();
	void deactivate(const std::string &name);
	void deactivate();
	void UpdateVolume(unsigned short value);
private:
	class AmbientSource {
	public:
		AmbientSource(const Ambient *a);
		~AmbientSource();
		unsigned int tick(uint64_t ticks, Point listener, ieDword timeslice);
		void hardStop();
		void SetVolume(unsigned short volume);
	private:
		int stream;
		const Ambient* ambient;
		uint64_t lastticks;
		unsigned int nextdelay;
		unsigned int nextref;
		unsigned int totalgain;

		bool isHeard(const Point &listener) const;
		int enqueue();
	};
	std::vector<AmbientSource *> ambientSources;
	
	int play();
	unsigned int tick(uint64_t ticks) const;
	void hardStop() const;
	
	std::mutex mutex;
	std::thread *player;
	std::condition_variable cond;
};

}

#endif /* AMBIENTMGRAL_H */
