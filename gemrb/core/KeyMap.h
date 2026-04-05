// SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef KEYMAP_H
#define KEYMAP_H

#include "exports.h"
#include "ie_types.h"

#include "Strings/StringMap.h"
#include "System/VFS.h"

namespace GemRB {

class GEM_EXPORT KeyMap {
public:
	struct Function {
		ieVariable moduleName;
		ieVariable function;
		int group;
		int key;
	};

	bool InitializeKeyMap(const path_t& inifile, const ResRef& keyfile);
	bool ResolveKey(unsigned short key, int group) const;
	bool ResolveName(const StringView& name, int group) const;

	const Function* LookupFunction(const StringView& name);

private:
	StringMap<Function> keymap;
};

}

#endif
