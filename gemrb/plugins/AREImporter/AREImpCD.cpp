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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImpCD.cpp,v 1.3 2004/02/24 22:20:44 balrog994 Exp $
 *
 */

#include "AREImpCD.h"
#include "AREImp.h"

AREImpCD::AREImpCD(void)
{
}

AREImpCD::~AREImpCD(void)
{
}

void* AREImpCD::Create(void)
{
	return new AREImp();
}

const char* AREImpCD::ClassName(void)
{
	return "AREImporter";
}

SClass_ID AREImpCD::SuperClassID(void)
{
	return IE_ARE_CLASS_ID;
}

Class_ID AREImpCD::ClassID(void)
{
	return Class_ID( 0x7a520471, 0xacb2417c );
}

const char* AREImpCD::InternalName(void)
{
	return "AREImp";
}
