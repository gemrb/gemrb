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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

/** 
 * @file EffectMgr.h
 * Declares EffectMgr class, loader for Effect objects
 * @author The GemRB Project
 */


#ifndef EFFECTMGR_H
#define EFFECTMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Effect.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/**
 * @class EffectMgr
 * Abstract loader for Effect objects
 */

class GEM_EXPORT EffectMgr : public Plugin {
public:
	EffectMgr(void);
	virtual ~EffectMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;

	/** Fills fx with Effect data loaded from the stream */
	virtual Effect* GetEffect(Effect *fx) = 0;
	/** Fills fx with Effect v1 data loaded from the stream*/
	virtual Effect* GetEffectV1(Effect *fx) = 0;
	/** Fills fx with Effect v2.0 data loaded from the stream*/
	virtual Effect* GetEffectV20(Effect *fx) = 0;
};

#endif
