// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LIGHT_H
#define LIGHT_H

#include "Sprite2D.h"

namespace GemRB {

GEM_EXPORT Holder<Sprite2D> CreateLight(const Size& size, uint8_t intensity) noexcept;

}

#endif /* LIGHT_H */
