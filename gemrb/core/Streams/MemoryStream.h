// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

#include "exports.h"

#include "DataStream.h"

namespace GemRB {

class GEM_EXPORT MemoryStream : public DataStream {
protected:
	char* data;

public:
	MemoryStream(const path_t& name, void* data, strpos_t size);
	~MemoryStream() override;
	DataStream* Clone() const noexcept override;

	strret_t Read(void* dest, strpos_t length) override;
	strret_t Write(const void* src, strpos_t length) override;
	strret_t Seek(stroff_t pos, strpos_t startpos) override;
};

}

#endif
