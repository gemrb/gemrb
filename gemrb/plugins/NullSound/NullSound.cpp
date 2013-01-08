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
 *
 */

#include "NullSound.h"

#include "win32def.h"

#include "AmbientMgr.h"
#include "SoundMgr.h"

using namespace GemRB;

NullSound::NullSound(void)
{
	XPos = 0;
	YPos = 0;
	ambim = new AmbientMgr();
}

NullSound::~NullSound(void)
{
	delete ambim;
}

bool NullSound::Init(void)
{
	return true;
}

Holder<SoundHandle> NullSound::Play(const char*, int, int, unsigned int, unsigned int *len)
{
	if (len) *len = 1000; //Returning 1 Second Length
	return Holder<SoundHandle>();
}

int NullSound::CreateStream(Holder<SoundMgr>)
{
	return 0;
}

bool NullSound::Stop()
{
	return true;
}

bool NullSound::Play()
{
	return true;
}

bool NullSound::Pause()
{
	return true;
}

bool NullSound::Resume()
{
	return true;
}

void NullSound::ResetMusics()
{
}

bool NullSound::CanPlay()
{
	return false;
}

void NullSound::UpdateListenerPos(int x, int y)
{
	XPos = x;
	YPos = y;
}

void NullSound::GetListenerPos(int& x, int& y)
{
	x = XPos;
	y = YPos;
}

int NullSound::SetupNewStream(ieWord, ieWord, ieWord, ieWord, bool, bool)
{
	return -1;
}

int NullSound::QueueAmbient(int, const char*)
{
	return -1;
}

bool NullSound::ReleaseStream(int, bool)
{
	return true;
}

void NullSound::SetAmbientStreamVolume(int, int)
{

}

void NullSound::QueueBuffer(int, unsigned short, int, short*, int, int)
{

}



#include "plugindef.h"

GEMRB_PLUGIN(0x96E414D, "Null Sound Driver")
PLUGIN_DRIVER(NullSound, "none")
END_PLUGIN()
