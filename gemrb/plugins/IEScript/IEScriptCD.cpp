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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IEScript/Attic/IEScriptCD.cpp,v 1.1 2003/12/04 22:15:59 balrog994 Exp $
 *
 */

#include "IEScriptCD.h"
#include "IEScript.h"

IEScriptCD::IEScriptCD(void)
{
}

IEScriptCD::~IEScriptCD(void)
{
}

void * IEScriptCD::Create(void)
{
	return new IEScript();
}

const char * IEScriptCD::ClassName(void)
{
	return "IEScript";
}

SClass_ID IEScriptCD::SuperClassID(void)
{
	return IE_SCRIPT_CLASS_ID;
}

Class_ID IEScriptCD::ClassID(void)
{
	return Class_ID(0x54aa0715, 0xdd0a8162);
}
