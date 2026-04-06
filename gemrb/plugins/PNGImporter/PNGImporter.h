// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PNGIMPORTER_H
#define PNGIMPORTER_H

#include "ImageMgr.h"

namespace GemRB {

struct PNGInternal;

class PNGImporter : public ImageMgr {
private:
	std::unique_ptr<PNGInternal> inf;

	bool hasPalette = false;

public:
	PNGImporter(void);
	PNGImporter(const PNGImporter&) = delete;
	~PNGImporter() override;
	PNGImporter& operator=(const PNGImporter&) = delete;
	void Close();
	bool Import(DataStream* stream) override;
	Holder<Sprite2D> GetSprite2D() override;
	Holder<Sprite2D> GetSprite2D(Region&&) override { return {}; };
	int GetPalette(int colors, Palette& pal) override;
};

}

#endif
