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
#include "Variables.h"

namespace GemRB {

Calendar::Calendar(void)
{
	AutoTable tab = gamedata->LoadTable("months");
	if (!tab) {
		return;
	}
	monthnamecount = tab->GetRowCount();
	monthnames = new ieStrRef[monthnamecount];
	days = new int[monthnamecount];

	for (size_t i = 0; i < monthnamecount; i++) {
		days[i] = tab->QueryFieldSigned<int>(i,0);
		daysinyear+=days[i];
		monthnames[i] = tab->QueryFieldAsStrRef(i,1);
	}
}

Calendar::~Calendar(void)
{
	delete[] monthnames;
	delete[] days;
}

void Calendar::GetMonthName(int dayandmonth) const
{
	int month=1;

	for (size_t i = 0; i < monthnamecount; ++i) {
		if (dayandmonth<days[i]) {
			core->GetTokenDictionary()->SetAtAsString("DAY", dayandmonth + 1);

			String tmp = core->GetString(monthnames[i]);
			core->GetTokenDictionary()->SetAt("MONTHNAME",tmp);
			//must not free tmp, SetAt doesn't copy the pointer!

			core->GetTokenDictionary()->SetAtAsString("MONTH", month);
			return;
		}
		dayandmonth-=days[i];
		//ignoring single days (they are not months)
		if (days[i]!=1) month++;
	}
}

int Calendar::GetCalendarDay(int date) const
{
	int dayandmonth;

	if (!daysinyear) return 0;
	dayandmonth = date%daysinyear;
	for (size_t i=0; i < monthnamecount; ++i) {
		if (dayandmonth<days[i]) {
			break;
		}
		dayandmonth-=days[i];
	}
	return dayandmonth+1;
}

}
