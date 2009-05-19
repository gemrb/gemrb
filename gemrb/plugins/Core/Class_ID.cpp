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
#include "Class_ID.h"

Class_ID::Class_ID(void)
{
	pa = 0xFFFFFFFF;
	pb = 0xFFFFFFFF;
}

Class_ID::Class_ID(unsigned long aa, unsigned long bb)
{
	pa = aa;
	pb = bb;
}

/*Class_ID::Class_ID(Class_ID& cid)
{
	pa = cid.PartA();
	pb = cid.PartB();
}*/

Class_ID::Class_ID(const Class_ID& cid)
{
	pa = cid.PartA();
	pb = cid.PartB();
}

unsigned long Class_ID::PartA(void) const
{
	return pa;
}

unsigned long Class_ID::PartB(void) const
{
	return pb;
}

int Class_ID::operator==(const Class_ID& cid) const
{
	return ( ( pa == cid.PartA() ) && ( pb == cid.PartB() ) );
}

int Class_ID::operator!=(const Class_ID& cid) const
{
	return ( ( pa != cid.PartA() ) || ( pb == cid.PartB() ) );
}

Class_ID& Class_ID::operator=(const Class_ID& cid)
{
	pa = cid.PartA();
	pb = cid.PartB();	
	return *this;
}

bool Class_ID::operator<(const Class_ID& rhs) const
{
	return ( ( pa < rhs.PartA() ) && ( pb < rhs.PartB() ) );
}
