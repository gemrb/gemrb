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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.h,v 1.2 2004/11/13 13:03:32 edheldil Exp $
 *
 */

#ifndef EFFECTQUEUE_H
#define EFFECTQUEUE_H


#include <vector>
#include "Effect.h"

class Actor;

//#define MAX_FX_OPCODES  255


class EffectQueue {
	std::vector< Effect* >  effects;

public:
	EffectQueue();
	virtual ~EffectQueue();

	// Returns true is successful. fx is just a reference, AddEffect()
	// will malloc its own copy
	bool AddEffect(Effect* fx);

	void ApplyAllEffects(Actor* target);
	void ApplyEffect(Actor* target, Effect* fx);
};


#endif  // ! EFFECTQUEUE_H
