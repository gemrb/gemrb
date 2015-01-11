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

#include "win32def.h"

#include "Interface.h"
#include "TableMgr.h"
#include "Variables.h"

namespace GemRB {

Calendar::Calendar(void)
{
	int i;
	AutoTable tab("months");
	if (!tab) {
		monthnamecount=-1;
		monthnames = NULL;
		days = NULL;
		return;
	}
	monthnamecount = tab->GetRowCount();
	monthnames = (int *) malloc(sizeof(int) * monthnamecount);
	days = (int *) malloc(sizeof(int) * monthnamecount);
	daysinyear=0;
	for(i=0;i<monthnamecount;i++) {
		days[i]=atoi(tab->QueryField(i,0));
		daysinyear+=days[i];
		monthnames[i]=atoi(tab->QueryField(i,1));
	}
}

Calendar::~Calendar(void)
{
	if (monthnames) free(monthnames);
	if (days) free(days);
}

void Calendar::GetMonthName(int dayandmonth) const
{
	int month=1;

	for(int i=0;i<monthnamecount;i++) {
		if (dayandmonth<days[i]) {
			char *tmp;

			core->GetTokenDictionary()->SetAtCopy("DAY", dayandmonth+1);

			tmp = core->GetCString( monthnames[i] );
			core->GetTokenDictionary()->SetAt("MONTHNAME",tmp);
			//must not free tmp, SetAt doesn't copy the pointer!

			core->GetTokenDictionary()->SetAtCopy("MONTH", month);
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
	int month=1;

	if (!daysinyear) return 0;
	dayandmonth = date%daysinyear;
	for(int i=0;i<monthnamecount;i++) {
		if (dayandmonth<days[i]) {
			break;
		}
		dayandmonth-=days[i];
		//ignoring single days (they are not months)
		if (days[i]!=1) month++;
	}
	return dayandmonth+1;
}

}
