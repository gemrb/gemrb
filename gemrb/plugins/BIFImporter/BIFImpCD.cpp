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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BIFImporter/BIFImpCD.cpp,v 1.2 2003/11/25 13:48:03 balrog994 Exp $
 *
 */

#include "BIFImp.h"
#include "BIFImpCD.h"

BIFImpCD::BIFImpCD(void)
{
}

BIFImpCD::~BIFImpCD(void)
{
}

void * BIFImpCD::Create(void)
{
	return new BIFImp();
}

const char* BIFImpCD::ClassName(void)
{
	return "BIFImporter";
}

SClass_ID BIFImpCD::SuperClassID(void)
{
	return IE_BIF_CLASS_ID;
}


Class_ID BIFImpCD::ClassID(void)
{
	return Class_ID(0x578a3f51, 0xd863ed17);
}

const char* BIFImpCD::InternalName(void)
{
	return "BIFImp";
}
