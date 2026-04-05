// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SLICEDSTREAM_H
#define SLICEDSTREAM_H

#include "exports.h"

#include "DataStream.h"

namespace GemRB {

class GEM_EXPORT SlicedStream : public DataStream {
private:
	//	bool autoFree;
	strpos_t startpos;
	DataStream* str;

public:
	SlicedStream(const DataStream* cfs, strpos_t startPos, strpos_t streamSize);
	~SlicedStream() override;
	DataStream* Clone() const noexcept override;

	strret_t Read(void* dest, strpos_t length) override;
	strret_t Write(const void* src, strpos_t length) override;
	stroff_t Seek(stroff_t pos, strpos_t startpos) override;
};

GEM_EXPORT DataStream* SliceStream(DataStream* str, strpos_t startpos, strpos_t size, bool preservepos = false);

}

#endif
