/***************************************************************************
                          TableMgr.h  -  description
                             -------------------
    begin                : ven ott 24 2003
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

#ifndef TABLEMGR_H
#define TABLEMGR_H

#include <Plugin.h>

/**Abstract class for Table Manager Plugins implementation
  *@author GemRB Developement Team
  */

class TableMgr : public Plugin  {
public: 
	TableMgr();
	~TableMgr();
  /** Returns the actual number of Rows in the Table */
  virtual int GetRowCount() = 0;
  /** Opens a Table File */
  virtual bool Open(DataStream * stream, bool autoFree = false) = 0;
};

#endif
