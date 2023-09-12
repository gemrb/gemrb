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
#include "Resource.h"

#include <vector>

namespace GemRB {

//bah!
class CREItem;
class Condition;

enum class StoreType { STORE = 0, TAVERN = 1, INN = 2, TEMPLE = 3,
BG2CONT = 4, IWD2CONT = 5, BAG = 6 };

using StoreActionType = enum StoreActionType : int16_t { STA_BUYSELL = 0, STA_IDENTIFY = 1, STA_STEAL = 2,
STA_CURE = 3, STA_DONATE = 4, STA_DRINK = 5, STA_ROOMRENT = 6, STA_OPTIONAL = 0x80 };

#define IE_STORE_BUY      1
#define IE_STORE_SELL     2
#define IE_STORE_ID       4
#define IE_STORE_STEAL    8
#define IE_STORE_DONATE   16     //gemrb extension
#define IE_STORE_CURE     32
#define IE_STORE_DRINK    64
#define IE_STORE_SELECT   0x40   //valid when these flags used as store action
#define IE_STORE_RENT     128    //gemrb extension
#define IE_STORE_QUALITY  0x600  //2 bits
// unknown 0x800
#define IE_STORE_FENCE    0x1000 //
#define IE_STORE_NOREPADJ 0x2000 // Reputation doesn't affect prices (BGEE)
#define IE_STORE_RECHARGE 0x4000 //gemrb extension, if set, store won't recharge
#define IE_STORE_BUYCRITS 0x8000 // User allowed to sell critical items (BGEE)
#define IE_STORE_CAPACITY 0x10000 //used for error reporting purposes

/**
 * @struct STOItem
 * Item in a store, together with available amount etc.
 */
struct GEM_EXPORT STOItem {
	ResRef ItemResRef;
	ieWord PurchasedAmount = 0;
	ieWord Usages[CHARGE_COUNTERS] = {};
	ieDword Flags = 0;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	int Weight = 0;
	int MaxStackAmount = 0;
	ieDword AmountInStock = 0;
	ieDwordSigned InfiniteSupply = 0;
	// V1.1
	Condition *triggers = nullptr;
	//ieDword TriggerRef; use infinitesupply
	char unknown2[56];

	STOItem() noexcept = default;
	explicit STOItem(const CREItem *item);
	STOItem(const STOItem&) = delete;
	~STOItem();
	STOItem& operator=(const STOItem&) = delete;
	void CopyCREItem(const CREItem *item);
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
	StoreType Type = StoreType::STORE;
	ieStrRef StoreName = ieStrRef::INVALID;
	ieDword Flags = 0;
	ieDword SellMarkup = 0;
	ieDword BuyMarkup = 0;
	ieDword DepreciationRate = 0;
	ieWord StealFailureChance = 0;
	ieWord Capacity = 0;
	char unknown[8];
	ieDword PurchasedCategoriesOffset = 0;
	ieDword PurchasedCategoriesCount = 0;
	ieDword ItemsOffset = 0;
	//don't use this value directly, use GetRealStockSize
	ieDword ItemsCount = 0;
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
	int AcceptableItemType(ieDword type, ieDword invflags, bool pc) const;
	STOCure *GetCure(unsigned int idx) const;
	STODrink *GetDrink(unsigned int idx) const;
	STOItem *GetItem(unsigned int idx, bool usetrigger) const;
	/** Evaluates item availability triggers */
	int GetRealStockSize() const;
	/** Recharges item */
	void RechargeItem(CREItem *item) const;
	/** Identifies item according to store lore */
	void IdentifyItem(CREItem *item) const;
	/** Adds a new item to the store (selling) */
	void AddItem(CREItem* item);
	void RemoveItem(const STOItem *itm);
	void RemoveItemByName(const ResRef& itemName, ieDword count = 0);
	/** Returns index of item */
	unsigned int FindItem(const ResRef &item, bool usetrigger) const;
	unsigned int CountItems(const ResRef& itemRef) const;
	const char *GetOwner() const;
	ieDword GetOwnerID() const;
	void SetOwnerID(ieDword owner);
	bool IsBag() const;
private:
	/** Finds a mergeable item in the stock, if exact is set, it checks for usage counts too */
	STOItem *FindItem(const CREItem *item, bool exact) const;
	bool IsItemAvailable(unsigned int slot) const;
};

}

#endif // ! STORE_H
