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

#include "MUSImpCD.h"
#include "MUSImp.h"

MUSImpCD::MUSImpCD(void)
{
}

MUSImpCD::~MUSImpCD(void)
{
}

void* MUSImpCD::Create(void)
{
	return new MUSImp();
}

const char* MUSImpCD::ClassName(void)
{
	return "MUSImporter";
}

SClass_ID MUSImpCD::SuperClassID(void)
{
	return IE_MUS_CLASS_ID;
}


Class_ID MUSImpCD::ClassID(void)
{
	return Class_ID( 0x6adc1420, 0xCC241263 );
}

const char* MUSImpCD::InternalName(void)
{
	return "MUSImp";
}
