// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ImageWriter.h"

namespace GemRB {

class BMPWriter : public ImageWriter {
public:
	BMPWriter() noexcept = default;
	void PutImage(DataStream* output, Holder<Sprite2D> sprite) override;
};

}
