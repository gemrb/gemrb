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

#include "Plugin.h"
#include "../../includes/globals.h"

/**Abstract class for Table Manager Plugins implementation
  *@author GemRB Developement Team
  */

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TableMgr : public Plugin  {
public: 
	TableMgr();
	virtual ~TableMgr();
  /** Returns the actual number of Rows in the Table */
  virtual int GetRowCount() = 0;
  /** Returns the actual number of Columns in the Table */
  virtual int GetColumnCount() = 0;
  /** Returns a pointer to a zero terminated 2da element,
      0,0 returns the default value, it may return NULL */
  virtual char *QueryField(int row = 0, int column = 0) const = 0;
  /** Returns a pointer to a zero terminated 2da element,
      uses column name and row name to search the field,
	  may return NULL */
  virtual char *QueryField(const char* row, const char* column) const = 0;
  /** Opens a Table File */
  virtual bool Open(DataStream * stream, bool autoFree = false) = 0;
  /** Returns a Row Name, returns NULL on error */
  virtual inline char *GetRowName(int index) const = 0;
};

#endif
