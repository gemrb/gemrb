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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "Spell.h"
#include "Interface.h"

SPLExtHeader::SPLExtHeader(void)
{
}

SPLExtHeader::~SPLExtHeader(void)
{
	delete [] features;
}

Spell::Spell(void)
{
//	SpellIconBAM = NULL;
}

Spell::~Spell(void)
{
	core->FreeSPLExt(ext_headers, casting_features);
/*
	if (SpellIconBAM) {
		core->FreeInterface( SpellIconBAM );
		SpellIconBAM = NULL;
	}
*/
}

//-1 will return cfb
//0 will always return first spell block
//otherwise set to caster level
EffectQueue *Spell::GetEffectBlock(int level)
{
	Effect *features;
	int count;

	//iwd2 has this hack
	if (level>=0) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			features = ext_headers[0].features;
			count = ext_headers[0].FeatureCount;
		} else {
			int block_index;
			for(block_index=0;block_index<ExtHeaderCount-1;block_index++) {
				if (ext_headers[block_index+1].RequiredLevel>level) {
					break;
				}
			}
			features = ext_headers[block_index].features;
			count = ext_headers[block_index].FeatureCount;
		}
	} else {
		features = casting_features;
		count = CastingFeatureCount;
	}
	EffectQueue *fxqueue = new EffectQueue();
	
	for (int i=0;i<count;i++) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			//hack the effect according to Level
			//fxqueue->AddEffect will copy the effect,
			//so we don't risk any overwriting
			if (EffectQueue::HasDuration(features+i)) {
				features[i].Duration=(TimePerLevel*level+TimeConstant)*7;
			}
		}
		fxqueue->AddEffect( features+i );
	}
	return fxqueue;
}
