// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// TODO: move this to platforms/windows

#ifndef WIN32DEF_H
#define WIN32DEF_H

#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
	#define NOMINMAX
#endif

#define UNICODE
#define _UNICODE
#define NOGDI
#define NOUSER

#ifdef _DEBUG
	#include <crtdbg.h>
#endif

#include <Windows.h>
#include <ntverp.h>
#if defined(VER_PRODUCTBUILD) && VER_PRODUCTBUILD >= 8100
	#include <VersionHelpers.h>
#endif

#endif //! WIN32DEF_H
