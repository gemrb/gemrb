// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

// Semantic warning macros
#ifdef HAVE_ATTRIBUTE_WARN_UNUSED_RESULT
	#define WARN_UNUSED __attribute__((warn_unused_result))
#else
	#define WARN_UNUSED
#endif

// Silence some persistent unused warnings (supported since gcc 2.4)
#ifdef HAVE_ATTRIBUTE_UNUSED
	#define IGNORE_UNUSED __attribute__((unused))
#else
	#define IGNORE_UNUSED
#endif

#endif
