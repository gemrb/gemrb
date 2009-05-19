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

#include "../../includes/win32def.h"
#include "VideoMode.h"

VideoMode::VideoMode(void)
	: Width( 0 ), Height( 0 ), bpp( 0 ), fullscreen( false )
{
}

VideoMode::~VideoMode(void)
{
}

VideoMode::VideoMode(const VideoMode& vm)
{
	Width = vm.GetWidth();
	Height = vm.GetHeight();
	bpp = vm.GetBPP();
	fullscreen = vm.GetFullScreen();
}

VideoMode::VideoMode(int w, int h, int bpp, bool fs)
{
	Width = w;
	Height = h;
	this->bpp = bpp;
	fullscreen = fs;
}

void VideoMode::SetWidth(int w)
{
	Width = w;
}

int VideoMode::GetWidth(void) const
{
	return Width;
}

void VideoMode::SetHeight(int h)
{
	Height = h;
}

int VideoMode::GetHeight(void) const
{
	return Height;
}

void VideoMode::SetBPP(int b)
{
	bpp = b;
}

int VideoMode::GetBPP(void) const
{
	return bpp;
}

void VideoMode::SetFullScreen(bool fs)
{
	fullscreen = fs;
}

bool VideoMode::GetFullScreen(void) const
{
	return fullscreen;
}

bool VideoMode::operator==(const VideoMode& cpt) const
{
	if (( Width == cpt.GetWidth() ) &&
		( Height == cpt.GetHeight() ) &&
		( bpp == cpt.GetBPP() ) &&
		( fullscreen == cpt.GetFullScreen() )) {
		return true;
	}
	return false;
}

VideoMode& VideoMode::operator=(const VideoMode& vm)
{
	Width = vm.GetWidth();
	Height = vm.GetHeight();
	bpp = vm.GetBPP();
	fullscreen = vm.GetFullScreen();
	return *this;
}
