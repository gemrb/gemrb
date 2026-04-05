// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CHUIMPORTER_H
#define CHUIMPORTER_H

#include "GUI/GUIFactory.h"
#include "Streams/DataStream.h"

namespace GemRB {

/**CHU File Importer Class
  *@author GemRB Development Team
  */

class CHUImporter : public GUIFactory {
	ieDword WindowCount = 0;
	ieDword CTOffset = 0;
	ieDword WEOffset = 0;

public:
	/** Returns the number of available windows */
	unsigned int GetWindowsCount() const override;
	bool LoadWindowPack(const ScriptingGroup_t&) override;
	/** Returns the i-th window in the Previously Loaded Stream */
	Window* GetWindow(ScriptingId) const override;
	/** This function loads all available windows from the 'stream' parameter. */
	bool Import(DataStream* stream) override;
};

}

#endif
