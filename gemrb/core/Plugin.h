// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file Plugin.h
 * Declares Plugin class, base class for all plugins
 * @author The GemRB Project
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include "exports.h"

#include "TypeID.h"

#include "Streams/DataStream.h"
#include "Streams/FileCache.h"

#include <memory>
#include <stdexcept>

namespace GemRB {

/**
 * @class Plugin
 * Base class for all GemRB plugins
 */

class GEM_EXPORT Plugin {
public:
	virtual ~Plugin() noexcept = default;
};

// TODO: this should be constrained to a Plugin type, but I cant get that to work in C++14
template<typename T>
using PluginHolder = std::shared_ptr<T>;

class GEM_EXPORT ImporterBase {
protected:
	DataStream* str = nullptr;
	virtual bool Import(DataStream* stream) = 0;

protected:
	DataStream* DecompressStream(DataStream* stream)
	{
		DataStream* cstream = CacheCompressedStream(stream, std::string(stream->filename));
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
	bool Open(DataStream* stream) noexcept
	{
		delete str;
		str = stream;
		if (!str) return false;
		return Import(str);
	}

	virtual ~ImporterBase()
	{
		delete str;
	}
};

/* MSVC must not see an abstract type _anywhere_ going into a `new` or `make_shared`. */
template<class T>
std::shared_ptr<std::enable_if_t<!std::is_abstract<T>::value, T>>
	MakeImporter() noexcept
{
	return std::make_shared<T>();
}

template<class T>
std::shared_ptr<std::enable_if_t<std::is_abstract<T>::value, T>>
	MakeImporter() noexcept
{
	return {};
}

}

#endif
