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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BG2HCAnim/Attic/BG2HCAnim.h,v 1.3 2003/11/25 13:48:04 balrog994 Exp $
 *
 */

#ifndef BG2HCANIM_H
#define BG2HCANIM_H
#include "../Core/HCAnimationSeq.h"

class BG2HCAnim : public HCAnimationSeq
{
public:
	BG2HCAnim(void);
	~BG2HCAnim(void);
	void GetCharAnimations(Actor * actor);
private:
	void LoadStaticAnims(const char * ResRef, Actor * actor);
	void LoadFullAnimLow(const char * ResRef, Actor * actor);
	void LoadMonsterHiRes(const char * ResRef, Actor * actor);
	void ParseStaticAnims(Actor* actor);
	void ParseSleeping(Actor* actor);
	void ParseFullAnimLow(Actor* actor);

	void LoadMonsterMidRes(const char * ResRef, Actor * actor);
	void LoadCritter(const char * ResRef, Actor * actor, bool bow);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
