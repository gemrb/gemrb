// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file Resource.h
 * Declares Resource class, base class for all resources
 * @author The GemRB Project
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include "ie_types.h"

#include "Plugin.h"

#include "Strings/CString.h"

#include <unordered_map>

namespace GemRB {

/** Resource reference */
class DataStream;

template<typename T>
using ResRefMap = std::unordered_map<ResRef, T, CstrHashCI>;

/**
 * Base class for all GemRB resources
 */

using Resource = ImporterBase;

template<class T>
using ResourceHolder = std::shared_ptr<T>; // TODO: this should be type constrained

}

#endif
