// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/** 
 * @file EffectMgr.h
 * Declares EffectMgr class, loader for Effect objects
 * @author The GemRB Project
 */


#ifndef EFFECTMGR_H
#define EFFECTMGR_H

#include "Effect.h"
#include "Plugin.h"

#include "Streams/DataStream.h"

namespace GemRB {

/**
 * @class EffectMgr
 * Abstract loader for Effect objects
 */

class GEM_EXPORT EffectMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;

	virtual Effect* GetEffect() = 0;
	virtual Effect* GetEffectV1() = 0;
	virtual Effect* GetEffectV20() = 0;
	/** Fills the stream with Effect v2 data loaded from the effect*/
	virtual void PutEffectV2(DataStream* stream, const Effect* fx) = 0;
};

}

#endif
