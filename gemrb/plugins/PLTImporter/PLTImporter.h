// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PLTIMPORTER_H
#define PLTIMPORTER_H

#include "PalettedImageMgr.h"

namespace GemRB {

class PLTImporter : public PalettedImageMgr {
private:
	ieDword Width = 0;
	ieDword Height = 0;
	void* pixels = nullptr;

public:
	PLTImporter() noexcept = default;
	PLTImporter(const PLTImporter&) = delete;
	~PLTImporter() override;
	PLTImporter& operator=(const PLTImporter&) = delete;
	bool Import(DataStream* stream) override;
	Holder<Sprite2D> GetSprite2D(unsigned int type, ieDword col[8]) override;
};

}

#endif
