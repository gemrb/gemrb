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

#include "../../includes/win32def.h"
#include "ClassDesc.h"

ClassDesc::ClassDesc(void)
{
}

ClassDesc::~ClassDesc(void)
{
}

int ClassDesc::BeginCreate()
{
	return 0;
}

int ClassDesc::EndCreate()
{
	return 0;
}

SClass_ID ClassDesc::SubClassID(void)
{
	return SuperClassID()&CLASS_ID_MASK;
}

const char* ClassDesc::InternalName(void)
{
	return 0;
}
