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
 * $Id$
 *
 */

#ifndef GUISCRIPT_H
#define GUISCRIPT_H

#include "../Core/ScriptEngine.h"
#ifdef WIN32
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#else
#include <Python.h>
#endif

#define SV_BPP 0
#define SV_WIDTH 1
#define SV_HEIGHT 2

class GUIScript : public ScriptEngine {
private:
	PyObject* pModule, * pDict;
	PyObject* pMainDic;
public:
	GUIScript(void);
	~GUIScript(void);
	/** Initialization Routine */
	bool Init(void);
	/** Load Script */
	bool LoadScript(const char* filename);
	/** Run Function */
	bool RunFunction(const char* fname, bool error=true);
	/** Exec a single String */
	void ExecString(const char* string);
	/** lets hope this one can be here without screwing up the general interface */
	PyObject *CallbackFunction(const char* fname, PyObject* pArgs);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
