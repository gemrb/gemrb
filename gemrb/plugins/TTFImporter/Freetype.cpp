// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Freetype.h"

#include "Logging/Logging.h"

namespace GemRB {

void LogFTError(FT_Error errCode)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s) { e, s },
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST \
	{ \
		0, 0 \
	} \
	} \
	;

	static const struct
	{
		int err_code;
		const char* err_msg;
	} ft_errors[] =
#include FT_ERRORS_H
		const char * err_msg;

	err_msg = NULL;
	for (const auto ftError : ft_errors) {
		if (errCode == ftError.err_code) {
			err_msg = ftError.err_msg;
			break;
		}
	}
	if (!err_msg) {
		err_msg = "unknown FreeType error";
	}
	Log(ERROR, "FreeType", "{}", err_msg);
}

}
