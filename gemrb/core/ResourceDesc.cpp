// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ResourceDesc.h"

namespace GemRB {

ResourceDesc::ResourceDesc(const TypeID* type, CreateFunc create, const path_t& ext, ieWord keyType)
	: type(type), ext(ext), keyType(keyType), create(create)
{
}

const path_t& ResourceDesc::GetExt() const
{
	return ext;
}

const TypeID* ResourceDesc::GetType() const
{
	return type;
}

ieWord ResourceDesc::GetKeyType() const
{
	return keyType;
}

ResourceHolder<Resource> ResourceDesc::Create(DataStream* stream) const
{
	return create(stream);
}

}
