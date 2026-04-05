// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MAPPED_FILE_MEMORY_STREAM_H
#define MAPPED_FILE_MEMORY_STREAM_H

#include "DataStream.h"
#include "MemoryStream.h"

namespace GemRB {

class GEM_EXPORT MappedFileMemoryStream : public MemoryStream {
public:
	explicit MappedFileMemoryStream(const std::string& fileName);
	~MappedFileMemoryStream() override;

	bool isOk() const;

	strret_t Read(void* dest, strpos_t len) override;
	strret_t Seek(stroff_t pos, strpos_t startPos) override;
	strret_t Write(const void* src, strpos_t len) override;
	DataStream* Clone() const noexcept override;

private:
	void* fileHandle;
	bool fileOpened;
	bool fileMapped;
};

}

#endif
