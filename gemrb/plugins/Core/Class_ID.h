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

#ifndef CLASSID_H
#define CLASSID_H

#include "../../includes/exports.h"

class GEM_EXPORT Class_ID {
public:
	Class_ID(void);
	Class_ID(unsigned long aa, unsigned long bb);
	//Class_ID(Class_ID& cid);
	Class_ID(const Class_ID& cid);
	unsigned long PartA(void) const;
	unsigned long PartB(void) const;
	int operator==(const Class_ID& cid) const;
	int operator!=(const Class_ID& cid) const;
	Class_ID& operator=(const Class_ID& cid);
	bool operator<(const Class_ID& rhs) const;
private:
	unsigned long pa, pb;
};

#endif
