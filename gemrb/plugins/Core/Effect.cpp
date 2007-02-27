/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "Interface.h"
#include "Actor.h"
#include "Effect.h"
#include "Game.h"

// FIXME: what about area spells? They can have map & coordinates as target
void AddEffect(Effect* fx, Actor* self, Actor* pretarget)
{
	int i;
	Game *game;
	Map *map;

	switch (fx->Target) {
	case FX_TARGET_SELF:
		self->fxqueue.AddEffect( fx );
		break;

	case FX_TARGET_PRESET:
		pretarget->fxqueue.AddEffect( fx );
		break;

	case FX_TARGET_PARTY:
		game=core->GetGame();
		for (i = game->GetPartySize(true); i >= 0; i--) {
			Actor* actor = game->GetPC( i, true );
			actor->fxqueue.AddEffect( fx );
		}
		break;

	case FX_TARGET_GLOBAL_INCL_PARTY:
		map=self->GetCurrentArea();
		for (i = map->GetActorCount(true); i >= 0; i--) {
			Actor* actor = map->GetActor( i, true );
			actor->fxqueue.AddEffect( fx );
		}
		break;

	case FX_TARGET_GLOBAL_EXCL_PARTY:
		map=self->GetCurrentArea();
		for (i = map->GetActorCount(false); i >= 0; i--) {
			Actor* actor = map->GetActor( i, false );
			//GetActorCount can now return all nonparty critters
			//if (actor->InParty) continue;
			actor->fxqueue.AddEffect( fx );
		}
		break;

	case FX_TARGET_UNKNOWN:
	default:
		printf( "Unknown FX target type: %d\n", fx->Target);
		break;
	}
}
