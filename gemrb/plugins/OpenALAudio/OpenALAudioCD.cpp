/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
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
 * $Id: ACMImp.cpp 4947 2007-12-29 14:01:22Z wjpalenstijn $
 *
 */

#include "OpenALAudio.h"
#include "OpenALAudioCD.h"

OpenALAudioCD::OpenALAudioCD(void)
{
}

OpenALAudioCD::~OpenALAudioCD(void)
{
}

void* OpenALAudioCD::Create(void)
{
	return new OpenALAudioDriver();
}

const char* OpenALAudioCD::ClassName(void)
{
	return "OpenALAudioDriver";
}

SClass_ID OpenALAudioCD::SuperClassID(void)
{
	return IE_AUDIO_CLASS_ID;
}


Class_ID OpenALAudioCD::ClassID(void)
{
	return Class_ID( 0x578d45a, 0xd86327f2 );
}

const char* OpenALAudioCD::InternalName(void)
{
	return "OpenALAudio";
}
