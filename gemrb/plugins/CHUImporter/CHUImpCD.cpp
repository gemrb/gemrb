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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CHUImporter/CHUImpCD.cpp,v 1.3 2004/02/24 22:20:43 balrog994 Exp $
 *
 */

#include "CHUImp.h"
#include "CHUImpCD.h"

CHUImpCD::CHUImpCD(void)
{
}

CHUImpCD::~CHUImpCD(void)
{
}

void* CHUImpCD::Create(void)
{
	return new CHUImp();
}

const char* CHUImpCD::ClassName(void)
{
	return "CHUImporter";
}

SClass_ID CHUImpCD::SuperClassID(void)
{
	return IE_CHU_CLASS_ID;
}

Class_ID CHUImpCD::ClassID(void)
{
	return Class_ID( 0xc018af34, 0x10a63725 );
}

const char* CHUImpCD::InternalName(void)
{
	return "CHUImp";
}
