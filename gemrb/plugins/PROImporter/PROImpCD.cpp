/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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

#include "PROImpCD.h"
#include "PROImp.h"

PROImpCD::PROImpCD(void)
{
}

PROImpCD::~PROImpCD(void)
{
}

void* PROImpCD::Create(void)
{
	return new PROImp();
}

const char* PROImpCD::ClassName(void)
{
	return "PROImp";
}

SClass_ID PROImpCD::SuperClassID(void)
{
	return IE_PRO_CLASS_ID;
}

Class_ID PROImpCD::ClassID(void)
{
	// FIXME?????
	return Class_ID( 0x0a8a5627, 0x1fc817de );
}
