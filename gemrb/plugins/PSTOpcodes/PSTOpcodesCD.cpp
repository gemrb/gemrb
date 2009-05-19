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

#include "PSTOpcodesCD.h"
#include "PSTOpc.h"

PSTOpcodesCD::PSTOpcodesCD(void)
{
}

PSTOpcodesCD::~PSTOpcodesCD(void)
{
}

void* PSTOpcodesCD::Create(void)
{
	return new PSTOpc();
}

const char* PSTOpcodesCD::ClassName(void)
{
	return "PSTOpcodes";
}

SClass_ID PSTOpcodesCD::SuperClassID(void)
{
	return IE_FX_CLASS_ID|ALLOW_CONCURRENT;
}

Class_ID PSTOpcodesCD::ClassID(void)
{
	return Class_ID( 0x00300000, 0x00000003 );
}

const char* PSTOpcodesCD::InternalName(void)
{
	return "PSTOpc";
}
