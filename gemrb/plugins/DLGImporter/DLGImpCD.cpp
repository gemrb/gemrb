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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DLGImporter/DLGImpCD.cpp,v 1.2 2004/02/24 22:20:42 balrog994 Exp $
 *
 */

#include "DLGImpCD.h"
#include "DLGImp.h"

DLGImpCD::DLGImpCD(void)
{
}

DLGImpCD::~DLGImpCD(void)
{
}

void* DLGImpCD::Create(void)
{
	return new DLGImp();
}

const char* DLGImpCD::ClassName(void)
{
	return "DLGImporter";
}

SClass_ID DLGImpCD::SuperClassID(void)
{
	return IE_DLG_CLASS_ID;
}

Class_ID DLGImpCD::ClassID(void)
{
	return Class_ID( 0x76adef24, 0x17adc342 );
}

const char* DLGImpCD::InternalName(void)
{
	return "DLGImp";
}
