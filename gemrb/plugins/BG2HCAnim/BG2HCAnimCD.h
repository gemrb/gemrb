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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BG2HCAnim/Attic/BG2HCAnimCD.h,v 1.2 2003/11/25 13:48:04 balrog994 Exp $
 *
 */

#ifndef BG2HCANIMCD_H
#define BG2HCANIMCD_H

#include "../Core/ClassDesc.h"

class BG2HCAnimCD : public ClassDesc
{
public:
	BG2HCAnimCD(void);
	~BG2HCAnimCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static BG2HCAnimCD Bg2HCAnimCD;

#endif
