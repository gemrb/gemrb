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
 *
 */

#include "IDSImporter.h"

#include "IDSImporterDefs.h"

#include "globals.h"
#include "win32def.h"

#include <cstring>

using namespace GemRB;

IDSImporter::IDSImporter(void)
{
}

IDSImporter::~IDSImporter(void)
{
	for (unsigned int i = 0; i < ptrs.size(); i++) {
		free( ptrs[i] );
	}
}

bool IDSImporter::Open(DataStream* str)
{
	if (str == NULL) {
		return false;
	}

	str->CheckEncrypted();
	char tmp[11];
	str->ReadLine( tmp, 10 );
	tmp[10] = 0;
	if (tmp[0] != 'I') {
		str->Rewind();
	}
	while (true) {
		char* line = ( char* ) malloc( 256 );
		int len = str->ReadLine( line, 256 );
		strlwr( line );
		if (len == -1) {
			free( line );
			break;
		}
		if (len == 0) {
			free( line );
			continue;
		}
		if (len < 256)
			line = ( char * ) realloc( line, len + 1 );
		char* str = strtok( line, " " );
		Pair p;
		p.val = strtoul( str, NULL, 0 );
		str = strtok( NULL, " " );
		p.str = str;
		if (str != NULL) {
			ptrs.push_back( line );
			pairs.push_back( p );
		} else {
			free( line );
		}
	}

	delete str;
	return true;
}

int IDSImporter::GetValue(const char* txt) const
{
	for (unsigned int i = 0; i < pairs.size(); i++) {
		if (stricmp( pairs[i].str, txt ) == 0) {
			return pairs[i].val;
		}
	}
	return -1;
}

char* IDSImporter::GetValue(int val) const
{
	for (unsigned int i = 0; i < pairs.size(); i++) {
		if (pairs[i].val == val) {
			return pairs[i].str;
		}
	}
	return NULL;
}

char* IDSImporter::GetStringIndex(unsigned int Index) const
{
	if (Index >= pairs.size()) {
		return NULL;
	}
	return pairs[Index].str;
}

int IDSImporter::GetValueIndex(unsigned int Index) const
{
	if (Index >= pairs.size()) {
		return 0;
	}
	return pairs[Index].val;
}

int IDSImporter::FindString(char *str, int len) const
{
	int i=pairs.size();
	while(i--) {
		if (strnicmp(pairs[i].str, str, len) == 0) {
			return i;
		}
	}
	return -1;
}

int IDSImporter::FindValue(int val) const
{
	int i=pairs.size();
	while(i--) {
		if(pairs[i].val==val) {
			return i;
		}
	}
	return -1;
}


#include "plugindef.h"

GEMRB_PLUGIN(0x1F41B94C, "IDS File Importer")
PLUGIN_CLASS(IE_IDS_CLASS_ID, IDSImporter)
END_PLUGIN()
