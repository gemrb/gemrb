/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TableMgr.h,v 1.17 2005/11/17 21:08:32 edheldil Exp $
 *
 */

/**
 * @file TableMgr.h
 * Declares TableMgr class, abstract loader for Table objects (.2DA files)
 * @author The GemRB Project
 */


#ifndef TABLEMGR_H
#define TABLEMGR_H

#include "Plugin.h"
#include "../../includes/globals.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/**
 * @class TableMgr
 * Abstract loader for Table objects (.2DA files)
 */

class GEM_EXPORT TableMgr : public Plugin {
public: 
	TableMgr();
	virtual ~TableMgr();
	/** Returns the actual number of Rows in the Table */
	virtual int GetRowCount() const = 0;
	/** Returns the number of Columns in the Table */
	virtual int GetColNamesCount() const = 0;
	/** Returns the actual number of Columns in a row */
	virtual int GetColumnCount(unsigned int row = 0) const = 0;
	/** Returns a pointer to a zero terminated 2da element,
	 * 0,0 returns the default value, it may return NULL */
	virtual char* QueryField(unsigned int row = 0, unsigned int column = 0) const = 0;
	/** Returns a pointer to a zero terminated 2da element,
	 * uses column name and row name to search the field,
	 * may return NULL */
	virtual char* QueryField(const char* row, const char* column) const = 0;
	/** Opens a Table File */
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	/** Returns a Row Name, returns NULL on error */
	virtual int GetRowIndex(const char* rowname) const = 0;
	virtual char* GetColumnName(unsigned int index) const = 0;
	virtual char* GetRowName(unsigned int index) const = 0;
};

#endif  // ! TABLEMGR_H
