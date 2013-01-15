/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Freetype.h"
#include "System/Logging.h"

namespace GemRB {

void LogFTError(FT_Error errCode)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
	
	static const struct
	{
		int          err_code;
		const char*  err_msg;
	} ft_errors[] =
#include FT_ERRORS_H
	int i;
	const char *err_msg;
	
	err_msg = NULL;
	for ( i=0; i < (int)((sizeof ft_errors)/(sizeof ft_errors[0])); ++i ) {
		if ( errCode == ft_errors[i].err_code ) {
			err_msg = ft_errors[i].err_msg;
			break;
		}
	}
	if ( !err_msg ) {
		err_msg = "unknown FreeType error";
	}
	Log(ERROR, "FreeType", "%s", err_msg);
}

}
