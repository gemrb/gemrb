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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Sprite2D.cpp,v 1.5 2004/02/24 22:20:36 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Sprite2D.h"

Sprite2D::Sprite2D()
{
	this->XPos = this->YPos = 0;
	this->palette = NULL;
}

Sprite2D::~Sprite2D(void)
{
}

Sprite2D& Sprite2D::operator=(Sprite2D& p)
{
	vptr = p.vptr;
	pixels = p.pixels;
	XPos = p.XPos;
	YPos = p.YPos;
	return *this;
}

Sprite2D::Sprite2D(Sprite2D& p)
{
	vptr = p.vptr;
	pixels = p.pixels;
	XPos = p.XPos;
	YPos = p.YPos;
}
