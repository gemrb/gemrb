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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "PNGImpCD.h"
#include "PNGImp.h"

PNGImpCD::PNGImpCD(void)
{
}

PNGImpCD::~PNGImpCD(void)
{
}

void* PNGImpCD::Create(void)
{
	return new PNGImp();
}

const char* PNGImpCD::ClassName(void)
{
	return "PNGImporter";
}

SClass_ID PNGImpCD::SuperClassID(void)
{
	return IE_PNG_CLASS_ID;
}


Class_ID PNGImpCD::ClassID(void)
{
	return Class_ID( 0x5181126a, 0xf19871a3 );
}

const char* PNGImpCD::InternalName(void)
{
	return "PNGImp";
}
