// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NullSound.h"

namespace GemRB {

AudioPoint NullSoundSourceHandle::nullPoint;

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
