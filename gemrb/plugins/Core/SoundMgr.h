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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SoundMgr.h,v 1.9 2004/02/24 22:20:36 balrog994 Exp $
 *
 */

#ifndef SOUNDMGR_H
#define SOUNDMGR_H

#include "Plugin.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT SoundMgr : public Plugin {
public:
	SoundMgr(void);
	virtual ~SoundMgr(void);
	virtual bool Init(void) = 0;
	virtual unsigned long Play(const char* ResRef, int XPos = 0, int YPos = 0) = 0;
	virtual unsigned long StreamFile(const char* filename) = 0;
	virtual bool Stop() = 0;
	virtual bool Play() = 0;
	virtual void ResetMusics() = 0;
	virtual void UpdateViewportPos(int XPos, int YPos) = 0;
};

#endif
