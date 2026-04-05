// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GemRB_TTFFont_h
#define GemRB_TTFFont_h

#include "FontManager.h"
#include "Freetype.h"

namespace GemRB {

class TTFFontManager : public FontManager {
private:
	FT_Stream ftStream = nullptr;
	FT_Face face = nullptr;

public:
	TTFFontManager() noexcept = default;
	TTFFontManager(const TTFFontManager&) = delete;
	~TTFFontManager() override;
	TTFFontManager& operator=(const TTFFontManager&) = delete;

	bool Import(DataStream* stream) override;
	void Close();

	Holder<Font> GetFont(unsigned short pxSize, FontStyle style, bool background) override;
};

}

#endif
