// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GemRB_BAMFontManager_h
#define GemRB_BAMFontManager_h

#include "FontManager.h"

namespace GemRB {

class BAMImporter;

class BAMFontManager : public FontManager {
private:
	/** private data members */
	BAMImporter* bamImp;
	bool isStateFont = false;
	ResRef resRef;

public:
	/** public methods */
	BAMFontManager(const BAMFontManager&) = delete;
	~BAMFontManager() override;
	BAMFontManager& operator=(const BAMFontManager&) = delete;
	BAMFontManager();

	bool Import(DataStream* stream) override;

	Holder<Font> GetFont(ieWord pxSize, FontStyle style, bool background) override;
};

}

#endif
