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

namespace GemRB {

Holder<SoundSourceHandle> NullSound::CreatePlaybackSource(const AudioPlaybackConfig&, bool)
{
	return MakeHolder<NullSoundSourceHandle>();
}

Holder<SoundStreamSourceHandle> NullSound::CreateStreamable(const AudioPlaybackConfig&, size_t)
{
	return MakeHolder<NullSoundStreamSourceHandle>();
}

Holder<SoundBufferHandle> NullSound::LoadSound(ResourceHolder<SoundMgr>, const AudioPlaybackConfig&)
{
	return MakeHolder<NullSoundBufferHandle>();
}

}

#include "plugindef.h"

GEMRB_PLUGIN(0x96E414D, "Null Sound Driver")
PLUGIN_DRIVER(NullSound, "none")
END_PLUGIN()
