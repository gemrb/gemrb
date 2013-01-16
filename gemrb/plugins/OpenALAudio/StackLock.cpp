/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007 The GemRB Project
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

#include "StackLock.h"

#include <cstdio>

using namespace GemRB;

// adapted from ScummVM's mutex.cpp

StackLock::StackLock(SDL_mutex* mutex, const char *mutexName)
	: _mutex(mutex), _mutexName(mutexName) {
	lock();
	(void)_mutexName; // NOOP to silence clang
}

StackLock::~StackLock() {
	unlock();
}

void StackLock::lock() {
#if 0
	if (_mutexName != NULL) {
		fprintf(stderr, "Locking mutex %s\n", _mutexName);
	}
#endif

	SDL_mutexP(_mutex);
}

void StackLock::unlock() {
#if 0
	if (_mutexName != NULL) {
		fprintf(stderr, "Unlocking mutex %s\n", _mutexName);
	}
#endif

	SDL_mutexV(_mutex);
}

