/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CHUImporter/CHUImp.h,v 1.4 2003/11/25 13:48:03 balrog994 Exp $
 *
 */

#ifndef CHUIMP_H
#define CHUIMP_H

#include "../Core/WindowMgr.h"
#include "../Core/DataStream.h"

/**CHU File Importer Class
  *@author GemRB Developement Team
  */

class CHUImp : public WindowMgr  {
private:
	DataStream * str;
	bool autoFree;
	unsigned long WindowCount, CTOffset, WEOffset;
public: 
	CHUImp();
	~CHUImp();
	/** Returns the number of available windows */
	unsigned long GetWindowsCount();
	/** Returns the i-th window in the Previously Loaded Stream */
	Window * GetWindow(unsigned long i);
	/** This function loads all available windows from the 'stream' parameter. */
	bool Open(DataStream * stream, bool autoFree = true);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
