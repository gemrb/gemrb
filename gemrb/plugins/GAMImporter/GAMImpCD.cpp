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
 *
 */

#include "GAMImpCD.h"
#include "GAMImp.h"

GAMImpCD::GAMImpCD(void)
{
}

GAMImpCD::~GAMImpCD(void)
{
}

void* GAMImpCD::Create(void)
{
	return new GAMImp();
}

const char* GAMImpCD::ClassName(void)
{
	return "GAMImporter";
}

SClass_ID GAMImpCD::SuperClassID(void)
{
	return IE_GAM_CLASS_ID;
}

Class_ID GAMImpCD::ClassID(void)
{
	return Class_ID( 0x9a5d7c10, 0xca5a7a43 );
}
