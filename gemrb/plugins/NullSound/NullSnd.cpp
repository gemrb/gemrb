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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/NullSound/NullSnd.cpp,v 1.1 2004/01/11 14:25:27 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "NullSnd.h"

NullSnd::NullSnd(void)
{
	
}

NullSnd::~NullSnd(void)
{
	
}

bool NullSnd::Init(void)
{
	return true;
}

unsigned long NullSnd::Play(const char * ResRef, int XPos, int YPos)
{
	return 1000; //Returning 1 Second Length
}

unsigned long NullSnd::StreamFile(const char * filename)
{
	return 0;
}

bool NullSnd::Stop()
{
	return true;
}

bool NullSnd::Play()
{
	return true;
}

void NullSnd::ResetMusics()
{
	
}

void NullSnd::UpdateViewportPos(int XPos, int YPos)
{
	
}
