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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/INIImporter/INIImpCD.cpp,v 1.2 2003/11/25 13:48:01 balrog994 Exp $
 *
 */

#include "INIImpCD.h"
#include "INIImp.h"

INIImpCD::INIImpCD(void)
{
}

INIImpCD::~INIImpCD(void)
{
}

void * INIImpCD::Create(void)
{
	return new INIImp();
}

const char* INIImpCD::ClassName(void)
{
	return "INIImporter";
}

SClass_ID INIImpCD::SuperClassID(void)
{
	return IE_INI_CLASS_ID;
}

Class_ID INIImpCD::ClassID(void)
{
	return Class_ID(0x581ad362, 0x770acd3e);
}

const char* INIImpCD::InternalName(void)
{
	return "INIImp";
}
