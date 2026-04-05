// SPDX-FileCopyrightText: 2007 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

#include "Plugin.h"
#include "Sprite2D.h"

#include "Streams/DataStream.h"

namespace GemRB {

class GEM_EXPORT ImageWriter : public Plugin {
public:
	/** Writes an Sprite2D to a stream and frees the sprite. */
	virtual void PutImage(DataStream* output, Holder<Sprite2D> sprite) = 0;
};

}

#endif
