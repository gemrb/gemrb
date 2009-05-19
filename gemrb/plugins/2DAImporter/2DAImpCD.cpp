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

#include "2DAImpCD.h"
#include "2DAImp.h"

p2DAImpCD::p2DAImpCD(void)
{
}

p2DAImpCD::~p2DAImpCD(void)
{
}

void* p2DAImpCD::Create(void)
{
	return new p2DAImp();
}

const char* p2DAImpCD::ClassName(void)
{
	return "2DAImporter";
}

SClass_ID p2DAImpCD::SuperClassID(void)
{
	return IE_2DA_CLASS_ID;
}

Class_ID p2DAImpCD::ClassID(void)
{
	return Class_ID( 0xa36513c6, 0x23755d73 );
}

const char* p2DAImpCD::InternalName(void)
{
	return "2DAImp";
}
