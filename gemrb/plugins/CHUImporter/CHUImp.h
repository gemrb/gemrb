/***************************************************************************
                          CHUImp.h  -  description
                             -------------------
    begin                : dom ott 12 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
  bool Open(DataStream * stream, bool autoFree);
};

#endif
