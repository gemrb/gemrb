// SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Calendar.h"

#include "Interface.h"
#include "TableMgr.h"

namespace GemRB {

Calendar::Calendar(void)
{
	AutoTable tab = gamedata->LoadTable("months");
	if (!tab) {
		return;
	}
	size_t monthNameCount = tab->GetRowCount();
	monthNames.resize(monthNameCount);
	days.resize(monthNameCount);

	for (size_t i = 0; i < monthNameCount; i++) {
		days[i] = tab->QueryFieldSigned<int>(i, 0);
		daysInYear += days[i];
		monthNames[i] = tab->QueryFieldAsStrRef(i, 1);
	}
}

void Calendar::GetMonthName(int dayAndMonth) const
{
	int month = 1;

	for (size_t i = 0; i < monthNames.size(); ++i) {
		if (dayAndMonth < days[i]) {
			SetTokenAsString("DAY", dayAndMonth + 1);
			core->GetTokenDictionary()["MONTHNAME"] = core->GetString(monthNames[i]);
			SetTokenAsString("MONTH", month);
			return;
		}
		dayAndMonth -= days[i];
		//ignoring single days (they are not months)
		if (days[i] != 1) month++;
	}
}

int Calendar::GetCalendarDay(int date) const
{
	if (!daysInYear) return 0;
	int dayAndMonth = date % daysInYear;
	for (const auto& day : days) {
		if (dayAndMonth < day) {
			break;
		}
		dayAndMonth -= day;
	}
	return dayAndMonth + 1;
}

}
