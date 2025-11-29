/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Store.h
 * Declares Store, class describing shops, temples and pubs, etc.
 * @author The GemRB Project
 */


#ifndef STORE_H
#define STORE_H

#include "exports.h"
#include "globals.h"
#include "ie_types.h"

#include "Holder.h"

#include <vector>

namespace GemRB {

//bah!
class CREItem;
class Condition;

enum class StoreType : uint8_t { Store = 0,
				 Tavern = 1,
				 Inn = 2,
				 Temple = 3,
				 BG2Cont = 4,
				 IWD2Cont = 5,
				 Bag = 6,
				 count };

enum class StoreActionType : uint8_t { Optional = 0x80,
				       None = 0x7f,
				       BuySell = 0,
				       Identify = 1,
				       Steal = 2,
				       Cure = 3,
				       Donate = 4,
				       Drink = 5,
				       RoomRent = 6,
				       count };

FLAG_ENUM StoreActionFlags : uint32_t {
	None = 0,
	Buy = 1,
	Sell = 2,
	ID = 4,
	Steal = 8,
	Donate = 16, //gemrb extension
	Cure = 32,
	Drink = 64,
	Rent = 128, //gemrb extension
	//QUALITY  = 0x400 | 0x200,  //2 bits
	// unknown 0x800
	Fence = 0x1000, //
	NoRepAdj = 0x2000, // Reputation doesn't affect prices (BGEE)
	ReCharge = 0x4000, //gemrb extension, if set, store won't recharge
	BuyCrits = 0x8000, // User allowed to sell critical items (BGEE)
	Capacity = 0x10000, //used for error reporting purposes
	Select = 0x20000
};

/**
 * @struct STOItem
 * Item in a store, together with available amount etc.
 */
struct GEM_EXPORT STOItem {
	ResRef ItemResRef;
	ieWord PurchasedAmount = 0;
	std::array<ieWord, CHARGE_COUNTERS> Usages;
	ieDword Flags = 0;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	int Weight = 0;
	int MaxStackAmount = 0;
	ieDword AmountInStock = 0;
	ieDwordSigned InfiniteSupply = 0;
	// V1.1
	Holder<Condition> triggers = nullptr;
	//ieDword TriggerRef; use infinitesupply
	char unknown2[56];

	STOItem() noexcept = default;
	explicit STOItem(const CREItem* item);
	STOItem(const STOItem&) = delete;
	STOItem& operator=(const STOItem&) = delete;
	void CopyCREItem(const CREItem* item);
};


/**
 * @struct STODrink
 * Kind of drink in a pub, with its associated rumour and price
 */

struct STODrink {
	ResRef RumourResRef;
	ieStrRef DrinkName;
	ieDword Price;
	ieDword Strength;
};


/**
 * @struct STOCure
 * Kind of cure available in a temple, with its associated price
 */

struct STOCure {
	ResRef CureResRef;
	ieDword Price;
};

/**
 * @class Store
 * Class describing shops, temples, pubs, etc.
 */

class GEM_EXPORT Store {
public:
	Store() noexcept = default;
	Store(const Store&) = delete;
	~Store();
	Store& operator=(const Store&) = delete;

	std::vector<STOItem*> items;
	std::vector<STODrink*> drinks;
	std::vector<STOCure*> cures;
	std::vector<ieDword> purchased_categories;

	ResRef Name;
	StoreType Type = StoreType::Store;
	ieStrRef StoreName = ieStrRef::INVALID;
	StoreActionFlags Flags = StoreActionFlags::None;
	ieDword SellMarkup = 0;
	ieDword BuyMarkup = 0;
	ieDword DepreciationRate = 0;
	ieWord StealFailureChance = 0;
	ieWord Capacity = 0;
	char unknown[8];
	ieDword PurchasedCategoriesOffset = 0;
	ieDword PurchasedCategoriesCount = 0;
	ieDword ItemsOffset = 0;
	// use GetRealStockSize to get the item count
	ieDword Lore = 0;
	ieDword IDPrice = 0;
	ResRef RumoursTavern;
	ieDword DrinksOffset = 0;
	ieDword DrinksCount = 0;
	ResRef RumoursTemple;
	ieDword AvailableRooms = 0;
	std::array<ieDword, 4> RoomPrices;
	ieDword CuresOffset = 0;
	ieDword CuresCount = 0;
	bool HasTriggers = false;
	char unknown2[36];

	// IWD2 only
	char unknown3[80];

	int version = 0;
	// the scripting name of the owner
	ieDword StoreOwnerID = 0;

public: //queries
	StoreActionFlags AcceptableItemType(ieDword type, ieDword invflags, bool pc) const;
	STOCure* GetCure(unsigned int idx) const;
	STODrink* GetDrink(unsigned int idx) const;
	STOItem* GetItem(unsigned int idx, bool usetrigger) const;
	/** Evaluates item availability triggers */
	int GetRealStockSize() const;
	/** Recharges item */
	void RechargeItem(CREItem* item) const;
	/** Identifies item according to store lore */
	void IdentifyItem(CREItem* item) const;
	/** Adds a new item to the store (selling) */
	void AddItem(CREItem* item);
	void RemoveItem(const STOItem* itm);
	void RemoveItemByName(const ResRef& itemName, ieDword count = 0);
	/** Returns index of item */
	unsigned int FindItem(const ResRef& item, bool usetrigger) const;
	unsigned int CountItems(const ResRef& itemRef) const;
	const char* GetOwner() const;
	ieDword GetOwnerID() const;
	void SetOwnerID(ieDword owner);
	bool IsBag() const;

private:
	/** Finds a mergeable item in the stock, if exact is set, it checks for usage counts too */
	STOItem* FindItem(const CREItem* item, bool exact) const;
	bool IsItemAvailable(const STOItem* item) const;
};

}

#endif // ! STORE_H
