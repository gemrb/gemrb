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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GAMImporter/GAMImp.cpp,v 1.3 2003/12/17 20:26:54 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "GAMImp.h"

GAMImp::GAMImp(void)
{
	str = NULL;
	autoFree = false;
}

GAMImp::~GAMImp(void)
{

	if(str && autoFree)
		delete(str);
}

bool GAMImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str)
		return false;
	str = stream;
	this->autoFree = autoFree;
        char Signature[8];
        str->Read(Signature, 8);
        if(strncmp(Signature, "GAMEV2.0", 8) == 0) { //soa, tob
		version=20;
	}
	else if(strncmp(Signature, "GAMEV1.0", 8) == 0) { //bg1?
		version=10;
	}
	else if(strncmp(Signature, "GAMEV1.1", 8) == 0) {//iwd, torment, totsc
		version=11;
	}
	else if(strncmp(Signature, "GAMEV2.2", 8) == 0) {//iwd2
		version=22;
	}
	else {
                printf("[GAMImporter]: This file is not a valid GAM File\n");
                return false;
        }

	str->Read(&GameTime,4);
	str->Read(&WhichFormation,4);
	for(int i=0;i<5;i++) {
		str->Read(Formations+i,4);
	}
	str->Read(&PartyGold,4);
	str->Read(&Unknown1c,4); //this is unknown yet
	str->Read(&PCOffset,4);
	str->Read(&PCCount,4);
	str->Read(&UnknownOffset,4);
	str->Read(&UnknownCount,4);
	str->Read(&NPCOffset,4);
	str->Read(&NPCCount,4);
	return true;
}
