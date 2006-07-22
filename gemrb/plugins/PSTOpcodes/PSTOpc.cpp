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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/PSTOpcodes/PSTOpc.cpp,v 1.1 2006/07/22 07:43:43 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "../Core/Actor.h"
#include "../Core/EffectQueue.h"
#include "../Core/Interface.h"
#include "../Core/damages.h"
#include "PSTOpc.h"


int fx_iron_fist (Actor* Owner, Actor* target, Effect* fx);//ed

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectRef effectnames[] = {
	{ "IronFist", fx_iron_fist, 0}, //ed
	{ NULL, NULL, 0 },
};


PSTOpc::PSTOpc(void)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
}

PSTOpc::~PSTOpc(void)
{
}

//0xd0 fx_iron_fist
//GemRB extension: lets you specify not hardcoded values
int fx_iron_fist (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword p1,p2;

	if (0) printf( "fx_iron_fist (%2d): Par1: %d  Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2)
	{
	case 0:  p1 = 3; p2 = 6; break;
	default:
		p1 = ieWord (fx->Parameter1&0xffff);
		p2 = ieWord (fx->Parameter1>>16);
	}
	STAT_ADD(IE_FISTHIT, p1);
	STAT_ADD(IE_FISTDAMAGE, p2);
	return FX_APPLIED;
}
