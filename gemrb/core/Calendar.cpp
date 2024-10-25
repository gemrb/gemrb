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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

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
