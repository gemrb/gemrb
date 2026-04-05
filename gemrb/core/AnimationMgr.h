// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ANIMATIONMGR_H
#define ANIMATIONMGR_H

#include "globals.h"

#include "Animation.h"
#include "AnimationFactory.h"
#include "Plugin.h"

namespace GemRB {

class Font;

class GEM_EXPORT AnimationMgr : public ImporterBase {
public:
	using index_t = AnimationFactory::index_t;

	virtual index_t GetCycleSize(index_t Cycle) = 0;
	virtual std::shared_ptr<AnimationFactory> GetAnimationFactory(const ResRef& resref, bool allowCompression = true) = 0;
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	virtual Holder<Sprite2D> GetPalette() = 0;
	virtual index_t GetCycleCount() = 0;
};

}

#endif
