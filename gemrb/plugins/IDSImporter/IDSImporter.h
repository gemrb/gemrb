// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IDSIMPORTER_H
	#define IDSIMPORTER_H

	#include "SymbolMgr.h"

	#include "Strings/StringView.h"

	#include <utility>
	#include <vector>

namespace GemRB {

class IDSImporter : public SymbolMgr {
private:
	using Pair = std::pair<int, std::string>;

	std::vector<Pair> pairs;

public:
	IDSImporter() noexcept = default;

	bool Open(std::unique_ptr<DataStream> stream) override;
	int GetValue(StringView txt) const override;
	const std::string& GetValue(int val) const override;
	const std::string& GetStringIndex(size_t Index) const override;
	int GetValueIndex(size_t Index) const override;
	int FindString(StringView str) const override;
	int FindValue(int val) const override;
	void AddSymbol(StringView str, int val) override;
	size_t GetSize() const override { return pairs.size(); }
	int GetHighestValue() const override;
};

#endif
}
