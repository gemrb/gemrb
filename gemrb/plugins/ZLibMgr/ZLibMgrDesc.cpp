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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ZLibMgr/ZLibMgrDesc.cpp,v 1.2 2003/11/25 13:47:59 balrog994 Exp $
 *
 */

#include "ZLibMgrDesc.h"
#include "ZLibManager.h"

ZLibMgrDesc::ZLibMgrDesc(void)
{
}

ZLibMgrDesc::~ZLibMgrDesc(void)
{
}

void * ZLibMgrDesc::Create(void)
{
	return new ZLibManager();
}

const char* ZLibMgrDesc::ClassName(void)
{
	return "ZLibCM";
}

SClass_ID ZLibMgrDesc::SuperClassID(void)
{
	return IE_COMPRESSION_CLASS_ID;
}

Class_ID ZLibMgrDesc::ClassID(void)
{
	return Class_ID(0xA1C8E1D1, 0xE7AA2BE5);
}
