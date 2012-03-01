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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "2DAImporter.h"

#include "win32def.h"

#include "Interface.h"
#include "System/FileStream.h"

using namespace GemRB;

#define MAXLENGTH 4096      //if a 2da has longer lines, change this
#define SIGNLENGTH 256      //if a 2da has longer default value, change this

p2DAImporter::p2DAImporter(void)
{
}

p2DAImporter::~p2DAImporter(void)
{
	for (unsigned int i = 0; i < ptrs.size(); i++) {
		free( ptrs[i] );
	}
}

bool p2DAImporter::Open(DataStream* str)
{
	if (str == NULL) {
		return false;
	}
	char Signature[SIGNLENGTH];
	str->CheckEncrypted();

	str->ReadLine( Signature, sizeof(Signature) );
	char* strp = Signature;
	while (*strp == ' ')
		strp++;
	if (strncmp( strp, "2DA V1.0", 8 ) != 0) {
		Log(WARNING, "2DAImporter", "Bad signature (%s)! Ignoring...", str->filename );
		// we don't care about this, so exptable.2da of iwd2 won't cause a bigger problem
		// also, certain creatures are described by 2da's without signature.
		// return false;
	}
	Signature[0] = 0;
	str->ReadLine( Signature, sizeof(Signature) );
	char* token = strtok( Signature, " " );
	if (token) {
		strncpy(defVal, token, sizeof(defVal) );
	} else { // no whitespace
		strncpy(defVal, Signature, sizeof(defVal) );
	}
	bool colHead = true;
	int row = 0;
	while (true) {
		char* line = ( char* ) malloc( MAXLENGTH );
		int len = str->ReadLine( line, MAXLENGTH-1 );
		if (len <= 0) {
			free( line );
			break;
		}
		if (line[0] == '#') { // allow comments
			free( line );
			continue;
		}
		if (len < MAXLENGTH)
			line = ( char * ) realloc( line, len + 1 );
		ptrs.push_back( line );
		if (colHead) {
			colHead = false;
			char* str = strtok( line, " " );
			while (str != NULL) {
				colNames.push_back( str );
				str = strtok( NULL, " " );
			}
		} else {
			char* str = strtok( line, " " );
			if (str == NULL)
				continue;
			rowNames.push_back( str );
			RowEntry r;
			rows.push_back( r );
			while (( str = strtok( NULL, " " ) ) != NULL) {
				rows[row].push_back( str );
			}
			row++;
		}
	}
	delete str;
	return true;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xB22F938, "2DA File Importer")
PLUGIN_CLASS(IE_2DA_CLASS_ID, p2DAImporter)
END_PLUGIN()
