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
 * $Id$
 *
 */

#ifndef FACTORY_H
#define FACTORY_H

#include "../../includes/globals.h"
#include "FactoryObject.h"
#include "AnimationFactory.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Factory {
private:
	std::vector< FactoryObject*> fobjects;
public:
	Factory(void);
	~Factory(void);
	void AddFactoryObject(FactoryObject* fobject);
	int IsLoaded(const char* ResRef, SClass_ID type);
	FactoryObject* GetFactoryObject(int pos);
	void FreeObjects(void);
};

#endif
