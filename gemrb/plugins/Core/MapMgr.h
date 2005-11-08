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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MapMgr.h,v 1.7 2005/11/08 22:59:05 edheldil Exp $
 *
 */

/**
 * @file MapMgr.h
 * Declares MapMgr class, loader for Map objects
 * @author The GemRB Project
 */

#ifndef MAPMGR_H
#define MAPMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Map.h"

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
 * @class MapMgr
 * Abstract loader for Map objects
 */

class GEM_EXPORT MapMgr : public Plugin {
public:
	MapMgr(void);
	virtual ~MapMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual Map* GetMap(const char* ResRef) = 0;

	virtual int GetStoredFileSize(Map *map) = 0;
	virtual int PutArea(DataStream* stream, Map *map) = 0;
};

#endif
