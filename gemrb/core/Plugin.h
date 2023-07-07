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

/**
 * @file Plugin.h
 * Declares Plugin class, base class for all plugins
 * @author The GemRB Project
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include "exports.h"

#include "TypeID.h"

#include "Streams/FileCache.h"
#include "Streams/DataStream.h"

#include <memory>
#include <stdexcept>

namespace GemRB {

/**
 * @class Plugin
 * Base class for all GemRB plugins
 */

class GEM_EXPORT Plugin
{
public:
	virtual ~Plugin() noexcept = default;
};

class GEM_EXPORT ImporterBase
{
protected:
	DataStream* str = nullptr;
	virtual bool Import(DataStream* stream) = 0;

protected:
	DataStream* DecompressStream(DataStream* stream) {
		DataStream* cstream = CacheCompressedStream(stream, stream->filename);
		if (!cstream) {
			return nullptr;
		}

		if (stream == str) {
			delete stream;
			str = cstream;
		}
		return cstream;
	}

public:
	bool Open(DataStream* stream) noexcept {
		delete str;
		str = stream;
		if (!str) return false;
		return Import(str);
	}

	virtual ~ImporterBase() {
		delete str;
	}
};

/* MSVC must not see an abstract type _anywhere_ going into a `new` or `make_shared`. */
template<class T>
	std::shared_ptr<std::enable_if_t<!std::is_abstract<T>::value, T>>
MakeImporter() noexcept {
	return std::make_shared<T>();
}

template<class T>
	std::shared_ptr<std::enable_if_t<std::is_abstract<T>::value, T>>
MakeImporter() noexcept {
	return {};
}

}

#endif
