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
 *
 */

#ifndef EFFIMP_H
#define EFFIMP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../../includes/ie_types.h"
#include "../Core/Effect.h"
#include "../Core/EffectMgr.h"


class EFFImp : public EffectMgr {
private:
	DataStream* str;
	bool autoFree;
	int version;

public:
	EFFImp(void);
	~EFFImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Effect* GetEffect(Effect *fx);
	Effect* GetEffectV1(Effect *fx);
	Effect* GetEffectV20(Effect *fx);
	void release(void)
	{
		delete this;
	}
};


#endif
