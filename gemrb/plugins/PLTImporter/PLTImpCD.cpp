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

#include "PLTImpCD.h"
#include "PLTImp.h"

PLTImpCD::PLTImpCD(void)
{
}

PLTImpCD::~PLTImpCD(void)
{
}

void* PLTImpCD::Create(void)
{
	return new PLTImp();
}

const char* PLTImpCD::ClassName(void)
{
	return "PLTImporter";
}

SClass_ID PLTImpCD::SuperClassID(void)
{
	return IE_PLT_CLASS_ID;
}


Class_ID PLTImpCD::ClassID(void)
{
	return Class_ID( 0x61725ade, 0x33ac4005 );
}

const char* PLTImpCD::InternalName(void)
{
	return "PLTImp";
}
