// SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CALENDAR_H
#define CALENDAR_H

#include "exports.h"
#include "ie_types.h"

#include <vector>

namespace GemRB {

class GEM_EXPORT Calendar {
private:
	int daysInYear = 0;
	std::vector<int> days;
	std::vector<ieStrRef> monthNames;

public:
	Calendar(void);
	Calendar(const Calendar&) = delete;
	Calendar& operator=(const Calendar&) = delete;
	void GetMonthName(int dayAndMonth) const;
	int GetCalendarDay(int date) const;
};

}

#endif
