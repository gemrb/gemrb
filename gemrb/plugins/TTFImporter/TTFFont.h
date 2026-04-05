// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __GemRB__TTFFont__
#define __GemRB__TTFFont__

#include "Freetype.h"
#include "Holder.h"

#include "GUI/TextSystem/Font.h"

namespace GemRB {

class TTFFont : public Font {
private:
	FT_Face face = nullptr;

	const Glyph& AliasBlank(ieWord chr) const;

public:
	TTFFont(Holder<Palette> pal, FT_Face face, int lineheight, int baseline);
	~TTFFont(void) override;

	const Glyph& GetGlyph(ieWord chr) const override;
	int GetKerningOffset(ieWord leftChr, ieWord rightChr) const override;
};

}

#endif /* defined(__GemRB__TTFFont__) */
