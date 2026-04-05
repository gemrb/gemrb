// SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#ifdef WIN32
	#include "win32def.h"
#elif defined(HAVE_UNISTD_H)
	#include <unistd.h>
#endif

#ifdef USE_TRACY
	#define TRACY_ENABLE 1
	#include <tracy/Tracy.hpp>
	#define TRACY(x) x
#else
	#define TRACY(x)
#endif

#include <cstddef>

#endif //! PLATFORM_H
