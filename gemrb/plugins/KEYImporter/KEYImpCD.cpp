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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "KEYImpCD.h"
#include "KeyImp.h"

KEYImpCD::KEYImpCD(void)
{
}

KEYImpCD::~KEYImpCD(void)
{
}

void* KEYImpCD::Create()
{
	return new KeyImp();
}

const char* KEYImpCD::ClassName(void)
{
	return "KEYImporter";
}

SClass_ID KEYImpCD::SuperClassID(void)
{
	return IE_KEY_CLASS_ID;
}

Class_ID KEYImpCD::ClassID(void)
{
	return Class_ID( 0x982da6f8, 0x47a4128f );
}

const char* KEYImpCD::InternalName(void)
{
	return "KEYImp";
}
