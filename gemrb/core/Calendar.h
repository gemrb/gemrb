/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2009 The GemRB Project
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

#ifndef CALENDAR_H
#define CALENDAR_H

#include "exports.h"
#include "ie_types.h"

namespace GemRB {

class GEM_EXPORT Calendar {
private:
	int daysinyear = 0;
	int monthnamecount = -1;
	int *days = nullptr;
	ieStrRef *monthnames = nullptr;

public:
	Calendar(void);
	~Calendar(void);
	void GetMonthName(int dayandmonth) const;
	int GetCalendarDay(int date) const;
};

}

#endif
