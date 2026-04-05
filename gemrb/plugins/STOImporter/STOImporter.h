// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef STOIMPORTER_H
#define STOIMPORTER_H

#include "StoreMgr.h"

namespace GemRB {


class STOImporter : public StoreMgr {
private:
	DataStream* str = nullptr;
	int version = 0;

public:
	STOImporter() noexcept = default;
	STOImporter(const STOImporter&) = delete;
	~STOImporter() override;
	STOImporter& operator=(const STOImporter&) = delete;
	bool Open(DataStream* stream) override;
	Store* GetStore(Store* store) override;

	//returns saved size, updates internal offsets before save
	void CalculateStoredFileSize(Store* st) const;
	//saves file
	bool PutStore(DataStream* stream, Store* store) override;

private:
	void GetItem(STOItem* item, const Store* s);
	void GetDrink(STODrink* drink);
	void GetCure(STOCure* cure);
	void GetPurchasedCategories(Store* s);

	void PutItems(DataStream* stream, const Store* s) const;
	void PutDrinks(DataStream* stream, const Store* s) const;
	void PutCures(DataStream* stream, const Store* s) const;
	void PutPurchasedCategories(DataStream* stream, const Store* s) const;
	void PutHeader(DataStream* stream, const Store* store);
};


}

#endif
