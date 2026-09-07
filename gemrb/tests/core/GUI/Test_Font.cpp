// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../../core/GUI/TextSystem/Font.h"

#include <gtest/gtest.h>

namespace GemRB {

namespace {

	class CachedFont : public Font {
	public:
		CachedFont()
			: Font(MakeHolder<Palette>(), 14, 12, false) {}

		using Font::FindGlyph;

		const Glyph& AddGlyph(ieWord chr, int width)
		{
			const Region frame(0, 12, width, 0);
			auto sprite = MakeHolder<Sprite2D>(frame, nullptr, PixelFormat::Paletted8Bit(palette));
			return CreateGlyphForCharSprite(chr, sprite);
		}
	};

}

TEST(FontTest, DeferredGlyphLookupDoesNotReturnFallback)
{
	CachedFont font;
	EXPECT_EQ(font.FindGlyph('A'), nullptr);

	// Match the initial cache of a TrueType font: an empty error glyph and
	// whitespace exist, but ordinary glyphs have not been rasterized yet.
	const Glyph& blank = font.AddGlyph(0, 0);
	font.AddGlyph(' ', 3);
	font.AddGlyph('\t', 12);
	ASSERT_NE(blank.pixels, nullptr);
	EXPECT_EQ(blank.size.w, 0);
	EXPECT_EQ(&font.GetGlyph('A'), &blank);
	EXPECT_EQ(font.FindGlyph('A'), nullptr);
	EXPECT_EQ(font.FindGlyph(31), nullptr); // an uncached hole inside the index

	const Glyph& letter = font.AddGlyph('A', 8);
	EXPECT_EQ(font.FindGlyph('A'), &letter);
	EXPECT_EQ(&font.GetGlyph('A'), &letter);
	EXPECT_EQ(font.StringSize(u"AAA").w, 24);
}

TEST(FontTest, ExplicitWhitespaceAliasesAreCached)
{
	CachedFont font;
	const Glyph& space = font.AddGlyph(' ', 3);
	EXPECT_EQ(font.FindGlyph(0x3000), nullptr);

	font.CreateAliasForChar(' ', 0x3000);
	EXPECT_EQ(font.FindGlyph(0x3000), &space);
	EXPECT_EQ(font.StringSize(u"\u3000").w, 3);
}

}
