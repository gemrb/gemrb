// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VitaLogger.h"

#include <psp2/kernel/clib.h>

#define printf sceClibPrintf

namespace GemRB {

void VitaLogger::LogInternal(log_level level, const char* owner, const char* message, log_color /*color*/)
{
	printf("[%s/%s]: %s\n", owner, log_level_text[level], message);
}

Logger::WriterPtr createVitaLogger()
{
	return Logger::WriterPtr(new VitaLogger());
}

}
