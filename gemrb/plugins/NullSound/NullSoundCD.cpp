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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/NullSound/NullSoundCD.cpp,v 1.2 2004/02/24 22:20:39 balrog994 Exp $
 *
 */

#include "NullSoundCD.h"
#include "NullSnd.h"

NullSoundCD::NullSoundCD(void)
{
}

NullSoundCD::~NullSoundCD(void)
{
}

void* NullSoundCD::Create(void)
{
	return new NullSnd();
}

const char* NullSoundCD::ClassName(void)
{
	return "NullSound";
}

SClass_ID NullSoundCD::SuperClassID(void)
{
	return IE_WAV_CLASS_ID;
}

Class_ID NullSoundCD::ClassID(void)
{
	return Class_ID( 0x00000000, 0x00000001 );
}

const char* NullSoundCD::InternalName(void)
{
	return "NullSnd";
}
