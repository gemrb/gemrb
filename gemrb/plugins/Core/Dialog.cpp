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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Dialog.cpp,v 1.7 2004/04/13 19:38:22 doc_wagon Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Dialog.h"

Dialog::Dialog(void)
{
}

Dialog::~Dialog(void)
{
	for (unsigned int i = 0; i < initialStates.size(); i++) {
		if (initialStates[i]) {
			FreeDialogState( initialStates[i] );
		}
	}
}

void Dialog::AddState(DialogState* ds)
{
	initialStates.push_back( ds );
}

DialogState* Dialog::GetState(unsigned int index)
{
	if (index >= initialStates.size()) {
		return NULL;
	}
	return initialStates.at( index );
}

void Dialog::FreeDialogState(DialogState* ds)
{
	for (unsigned int i = 0; i < ds->transitionsCount; i++) {
		if (ds->transitions[i]->action)
			FreeDialogString( ds->transitions[i]->action );
		if (ds->transitions[i]->trigger)
			FreeDialogString( ds->transitions[i]->trigger );
		delete( ds->transitions[i] );
	}
	delete( ds->transitions );
	if (ds->trigger) {
		FreeDialogString( ds->trigger );
	}
	delete( ds );
}

void Dialog::FreeDialogString(DialogString* ds)
{
	for (unsigned int i = 0; i < ds->count; i++) {
		if (ds->strings[i]) {
			printf( "Freeing String 0x%08X\n", ds->strings[i] );
			free( ds->strings[i] );
		}
	}
	free( ds->strings );
	delete( ds );
}
