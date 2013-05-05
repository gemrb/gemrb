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

#include <vector>

namespace GemRB {

//bah!
class CREItem;
class Trigger;

typedef enum StoreType { STT_STORE=0, STT_TAVERN=1, STT_INN=2, STT_TEMPLE=3,
STT_BG2CONT=4, STT_IWD2CONT=5 } StoreType;

typedef enum StoreActionType { STA_BUYSELL=0, STA_IDENTIFY=1, STA_STEAL=2,
STA_CURE=3, STA_DONATE=4, STA_DRINK=5, STA_ROOMRENT=6, STA_OPTIONAL=0x80} StoreActionType;

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
#define IE_STORE_FENCE    0x1000 //
#define IE_STORE_RECHARGE 0x4000 //gemrb extension, if set, store won't recharge


/**
 * @struct STOItem
 * Item in a store, together with available amount etc.
 */
struct STOItem {
	ieResRef ItemResRef;
	ieWord PurchasedAmount;
	ieWord Usages[CHARGE_COUNTERS];
	ieDword Flags;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	int Weight;
	int MaxStackAmount;
	ieDword AmountInStock;
	ieDwordSigned InfiniteSupply;
	// V1.1
	Trigger *trigger;
	//ieDword TriggerRef; use infinitesupply
	char unknown2[56];
};


/**
 * @struct STODrink
 * Kind of drink in a pub, with its associated rumour and price
 */

struct STODrink {
	ieResRef RumourResRef;
	ieStrRef DrinkName;
	ieDword Price;
	ieDword Strength;
};


/**
 * @struct STOCure
 * Kind of cure available in a temple, with its associated price
 */

struct STOCure {
	ieResRef CureResRef;
	ieDword Price;
};

/**
 * @class Store
 * Class describing shops, temples, pubs, etc.
 */

class GEM_EXPORT Store {
public:
	Store();
	~Store();

	std::vector< STOItem*> items;
	STODrink* drinks;
	STOCure* cures;
	ieDword* purchased_categories;

	ieResRef Name;
	ieDword Type;
	ieStrRef StoreName;
	ieDword Flags;
	ieDword SellMarkup;
	ieDword BuyMarkup;
	ieDword DepreciationRate;
	ieWord StealFailureChance;
	ieWord Capacity;
	char unknown[8];
	ieDword PurchasedCategoriesOffset;
	ieDword PurchasedCategoriesCount;
	ieDword ItemsOffset;
	//don't use this value directly, use GetRealStockSize
	ieDword ItemsCount;
	ieDword Lore;
	ieDword IDPrice;
	ieResRef RumoursTavern;
	ieDword DrinksOffset;
	ieDword DrinksCount;
	ieResRef RumoursTemple;
	ieDword AvailableRooms;
	ieDword RoomPrices[4];
	ieDword CuresOffset;
	ieDword CuresCount;
	bool HasTriggers;
	char unknown2[36];

	// IWD2 only
	char unknown3[80];

	int version;
	// the scripting name of the owner
	ieDword StoreOwnerID;

public: //queries
	int AcceptableItemType(ieDword type, ieDword invflags, bool pc) const;
	STOCure *GetCure(unsigned int idx) const;
	STODrink *GetDrink(unsigned int idx) const;
	STOItem *GetItem(unsigned int idx, bool usetrigger);
	/** Evaluates item availability triggers */
	int GetRealStockSize();
	/** Recharges item */
	void RechargeItem(CREItem *item);
	/** Adds a new item to the store (selling) */
	void AddItem(CREItem* item);
	void RemoveItem(unsigned int idx);
	void RemoveItem( STOItem *itm);
	/** Returns index of item */
	unsigned int FindItem(const ieResRef item, bool usetrigger) const;
	const char *GetOwner() const;
	ieDword GetOwnerID() const;
	void SetOwnerID(ieDword owner);
	bool IsBag() const;
private:
	/** Identifies item according to store lore */
	void IdentifyItem(CREItem *item) const;
	/** Finds a mergeable item in the stock, if exact is set, it checks for usage counts too */
	STOItem *FindItem(CREItem *item, bool exact);
	bool IsItemAvailable(unsigned int slot) const;
};

}

#endif // ! STORE_H
