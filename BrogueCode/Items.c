/*
 *  Items.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/17/09.
 *  Copyright 2012. All rights reserved.
 *  
 *  This file is part of Brogue.
 *
 *  Brogue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Brogue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "Rogue.h"
#include "IncludeGlobals.h"
#include <math.h>

// Allocates space, generates a specified item (or random category/kind if -1)
// and returns a pointer to that item. The item is not given a location here
// and is not inserted into the item chain!
item *generateItem(unsigned short theCategory, short theKind) {
	short i;
	item *theItem;
	
	theItem = (item *) malloc(sizeof(item));
	memset(theItem, '\0', sizeof(item) );
	
	theItem->category = 0;
	theItem->kind = 0;
	theItem->flags = 0;
	theItem->displayChar = '&';
	theItem->foreColor = &itemColor;
	theItem->inventoryColor = &white;
	theItem->armor = 0;
	theItem->strengthRequired = 0;
	theItem->enchant1 = 0;
	theItem->enchant2 = 0;
	theItem->vorpalEnemy = 0;
	theItem->charges = 0;
	theItem->quantity = 1;
	theItem->quiverNumber = 0;
	theItem->keyZ = 0;
	theItem->inscription[0] = '\0';
	theItem->nextItem = NULL;
	
	for (i=0; i < KEY_ID_MAXIMUM; i++) {
		theItem->keyLoc[i].x = 0;
		theItem->keyLoc[i].y = 0;
		theItem->keyLoc[i].machine = 0;
		theItem->keyLoc[i].disposableHere = false;
	}
	
	makeItemInto(theItem, theCategory, theKind);
	
	return theItem;
}

unsigned short pickItemCategory(unsigned short theCategory) {
	short i, sum, randIndex;
	short probabilities[12] =						{50,	42,		50,		3,		3,		10,		8,		5,		3,		0,		0,		0};
	unsigned short correspondingCategories[12] =	{GOLD,	SCROLL,	POTION,	STAFF,	WAND,	WEAPON,	ARMOR,	FOOD,	RING,	AMULET,	GEM,	KEY};
	
	sum = 0;
	
	for (i=0; i<11; i++) {
		if (theCategory & correspondingCategories[i] || theCategory <= 0) {
			sum += probabilities[i];
		}
	}
	
	if (sum == 0) {
		return theCategory; // e.g. when you pass in AMULET or GEM, since they have no frequency
	}
	
	randIndex = rand_range(1, sum);
	
	for (i=0; ; i++) {
		if (theCategory & correspondingCategories[i] || theCategory <= 0) {
			if (randIndex <= probabilities[i]) {
				return correspondingCategories[i];
			}
			randIndex -= probabilities[i];
		}
	}
}

// Sets an item to the given type and category (or chooses randomly if -1) with all other stats
item *makeItemInto(item *theItem, unsigned short itemCategory, short itemKind) {
	itemTable *theEntry;

	if (itemCategory <= 0) {
		itemCategory = ALL_ITEMS;
	}
	
	itemCategory = pickItemCategory(itemCategory);
	
	theItem->category = itemCategory;
	
	switch (itemCategory) {
			
		case FOOD:
			if (itemKind < 0) {
				itemKind = chooseKind(foodTable, NUMBER_FOOD_KINDS);
			}
			theEntry = &foodTable[itemKind];
			theItem->displayChar = FOOD_CHAR;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
			
		case WEAPON:
			if (itemKind < 0) {
				itemKind = chooseKind(weaponTable, NUMBER_WEAPON_KINDS);
			}
			theEntry = &weaponTable[itemKind];
			theItem->damage = weaponTable[itemKind].range;
			theItem->strengthRequired = weaponTable[itemKind].strengthRequired;
			theItem->displayChar = WEAPON_CHAR;
			
			switch (itemKind) {
				case MACE:
				case HAMMER:
					theItem->flags |= ITEM_ATTACKS_SLOWLY;
					break;
				case SPEAR:
				case PIKE:
					theItem->flags |= ITEM_ATTACKS_PENETRATE;
					break;
				case AXE:
				case WAR_AXE:
					theItem->flags |= ITEM_ATTACKS_ALL_ADJACENT;
					break;
				default:
					break;
			}
			
			if (rand_percent(40)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
					if (rand_percent(33)) { // give it a bad runic
						theItem->enchant2 = rand_range(NUMBER_GOOD_WEAPON_ENCHANT_KINDS, NUMBER_WEAPON_RUNIC_KINDS - 1);
						theItem->flags |= ITEM_RUNIC;
					}
				} else if (rand_range(3, 10) * ((theItem->flags & ITEM_ATTACKS_SLOWLY) ? 2 : 1) > theItem->damage.lowerBound) {
					// give it a good runic; lower damage items are more likely to be runic
					theItem->enchant2 = rand_range(0, NUMBER_GOOD_WEAPON_ENCHANT_KINDS - 1);
					theItem->flags |= ITEM_RUNIC;
					if (theItem->enchant2 == W_SLAYING) {
						theItem->vorpalEnemy = chooseVorpalEnemy();
					}
				}
			}
			if (itemKind == DART || itemKind == INCENDIARY_DART || itemKind == JAVELIN) {
				if (itemKind == INCENDIARY_DART) {
					theItem->quantity = rand_range(3, 6);
				} else {
					theItem->quantity = rand_range(5, 18);
				}
				theItem->quiverNumber = rand_range(1, 60000);
				theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC); // throwing weapons can't be cursed or runic
				theItem->enchant1 = 0; // throwing weapons can't be magical
			}
			theItem->charges = WEAPON_KILLS_TO_AUTO_ID; // kill 20 enemies to auto-identify
			break;
			
		case ARMOR:
			if (itemKind < 0) {
				itemKind = chooseKind(armorTable, NUMBER_ARMOR_KINDS);
			}
			theEntry = &armorTable[itemKind];
			theItem->armor = randClump(armorTable[itemKind].range);
			theItem->strengthRequired = armorTable[itemKind].strengthRequired;
			theItem->displayChar = ARMOR_CHAR;
			theItem->charges = ARMOR_DELAY_TO_AUTO_ID; // this many turns until it reveals its enchants and whether runic
			if (rand_percent(40)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
					if (rand_percent(33)) { // give it a bad runic
						theItem->enchant2 = rand_range(NUMBER_GOOD_ARMOR_ENCHANT_KINDS, NUMBER_ARMOR_ENCHANT_KINDS - 1);
						theItem->flags |= ITEM_RUNIC;
					}
				} else if (rand_range(0, 95) > theItem->armor) { // give it a good runic
					theItem->enchant2 = rand_range(0, NUMBER_GOOD_ARMOR_ENCHANT_KINDS - 1);
					theItem->flags |= ITEM_RUNIC;
					if (theItem->enchant2 == A_IMMUNITY) {
						theItem->vorpalEnemy = chooseVorpalEnemy();
					}
				}
			}
			break;
		case SCROLL:
			if (itemKind < 0) {
				itemKind = chooseKind(scrollTable, NUMBER_SCROLL_KINDS);
			}
			theEntry = &scrollTable[itemKind];
			theItem->displayChar = SCROLL_CHAR;
			theItem->flags |= ITEM_FLAMMABLE;
			break;
		case POTION:
			if (itemKind < 0) {
				itemKind = chooseKind(potionTable, NUMBER_POTION_KINDS);
			}
			theEntry = &potionTable[itemKind];
			theItem->displayChar = POTION_CHAR;
			break;
		case STAFF:
			if (itemKind < 0) {
				itemKind = chooseKind(staffTable, NUMBER_STAFF_KINDS);
			}
			theEntry = &staffTable[itemKind];
			theItem->displayChar = STAFF_CHAR;
			theItem->charges = 2;
			if (rand_percent(50)) {
				theItem->charges++;
				if (rand_percent(15)) {
					theItem->charges++;
				}
			}
			theItem->enchant1 = theItem->charges;
			theItem->enchant2 = (itemKind == STAFF_BLINKING || itemKind == STAFF_OBSTRUCTION ? 1000 : 500); // start with no recharging mojo
			break;
		case WAND:
			if (itemKind < 0) {
				itemKind = chooseKind(wandTable, NUMBER_WAND_KINDS);
			}
			theEntry = &wandTable[itemKind];
			theItem->displayChar = WAND_CHAR;
			theItem->charges = randClump(wandTable[itemKind].range);
			break;
		case RING:
			if (itemKind < 0) {
				itemKind = chooseKind(ringTable, NUMBER_RING_KINDS);
			}
			theEntry = &ringTable[itemKind];
			theItem->displayChar = RING_CHAR;
			theItem->enchant1 = randClump(ringTable[itemKind].range);
			theItem->charges = RING_DELAY_TO_AUTO_ID; // how many turns of being worn until it auto-identifies
			if (rand_percent(16)) {
				// cursed
				theItem->enchant1 *= -1;
				theItem->flags |= ITEM_CURSED;
			}
			break;
		case GOLD:
			theEntry = NULL;
			theItem->displayChar = GOLD_CHAR;
			theItem->quantity = rand_range(50 + rogue.depthLevel * 10, 100 + rogue.depthLevel * 15);
			break;
		case AMULET:
			theEntry = NULL;
			theItem->displayChar = AMULET_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case GEM:
			theEntry = NULL;
			theItem->displayChar = GEM_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case KEY:
			theEntry = NULL;
			theItem->displayChar = KEY_CHAR;
			theItem->flags |= (ITEM_IDENTIFIED | ITEM_NAMED | ITEM_PLAYER_AVOIDS);
			break;
		default:
			theEntry = NULL;
			message("something has gone terribly wrong!", true);
			break;
	}
	if (theItem
		&& !(theItem->flags & ITEM_IDENTIFIED)
		&& (!(theItem->category & (POTION | SCROLL) ) || (theEntry && !theEntry->identified))) {
		theItem->flags |= ITEM_CAN_BE_IDENTIFIED;
	}
	theItem->kind = itemKind;
	
	return theItem;
}

short chooseKind(itemTable *theTable, short numKinds) {
	short i, totalFrequencies = 0, randomFrequency;
	for (i=0; i<numKinds; i++) {
		totalFrequencies += max(0, theTable[i].frequency);
	}
	randomFrequency = rand_range(1, totalFrequencies);
	for (i=0; randomFrequency > theTable[i].frequency; i++) {
		randomFrequency -= max(0, theTable[i].frequency);
	}
	return i;
}

// Places an item at (x,y) if provided or else a random location if they're 0. Inserts item into the floor list.
item *placeItem(item *theItem, short x, short y) {
	short loc[2];
	enum dungeonLayers layer;
	char theItemName[DCOLS], buf[DCOLS];
	if (x <= 0 || y <= 0) {
		randomMatchingLocation(&(loc[0]), &(loc[1]), FLOOR, NOTHING, -1);
		theItem->xLoc = loc[0];
		theItem->yLoc = loc[1];
	} else {
		theItem->xLoc = x;
		theItem->yLoc = y;
	}
	
	removeItemFromChain(theItem, floorItems); // just in case; double-placing an item will result in game-crashing loops in the item list
	
	theItem->nextItem = floorItems->nextItem;
	floorItems->nextItem = theItem;
	pmap[theItem->xLoc][theItem->yLoc].flags |= HAS_ITEM;
	if ((theItem->flags & ITEM_MAGIC_DETECTED) && itemMagicChar(theItem)) {
		pmap[theItem->xLoc][theItem->yLoc].flags |= ITEM_DETECTED;
	}
	if (cellHasTerrainFlag(x, y, T_IS_DF_TRAP)
		&& !(pmap[x][y].flags & PRESSURE_PLATE_DEPRESSED)) {
		
		pmap[x][y].flags |= PRESSURE_PLATE_DEPRESSED;
		if (playerCanSee(x, y)) {
			if (cellHasTerrainFlag(x, y, T_IS_SECRET)) {
				discover(x, y);
				refreshDungeonCell(x, y);
			}
			itemName(theItem, theItemName, false, false, NULL);
			sprintf(buf, "a pressure plate clicks underneath the %s!", theItemName);
			message(buf, true);
		}
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & T_IS_DF_TRAP) {
				spawnDungeonFeature(x, y, &(dungeonFeatureCatalog[tileCatalog[pmap[x][y].layers[layer]].fireType]), true, false);
				promoteTile(x, y, layer, false);
			}
		}
	}
	return theItem;
}

void fillItemSpawnHeatMap(unsigned short heatMap[DCOLS][DROWS], unsigned short heatLevel, short x, short y) {
	enum directions dir;
	short newX, newY;
	
	if (pmap[x][y].layers[DUNGEON] == DOOR) {
		heatLevel += 10;
	} else if (pmap[x][y].layers[DUNGEON] == SECRET_DOOR) {
		heatLevel += 3000;
	}
	if (heatMap[x][y] > heatLevel) {
		heatMap[x][y] = heatLevel;
	}
	for (dir = 0; dir < 4; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		if (coordinatesAreInMap(newX, newY)
			&& (!cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY | T_IS_DEEP_WATER | T_LAVA_INSTA_DEATH | T_AUTO_DESCENT)
				|| cellHasTerrainFlag(newX, newY, T_IS_SECRET))
			&& heatLevel < heatMap[newX][newY]) {
			fillItemSpawnHeatMap(heatMap, heatLevel, newX, newY);
		}
	}
}

void coolHeatMapAt(unsigned short heatMap[DCOLS][DROWS], short x, short y, unsigned long *totalHeat) {
	short k, l;
	unsigned short currentHeat;
	
	currentHeat = heatMap[x][y];
	*totalHeat -= heatMap[x][y];
	heatMap[x][y] = 0;
	
	// lower the heat near the chosen location
	for (k = -5; k <= 5; k++) {
		for (l = -5; l <= 5; l++) {
			if (coordinatesAreInMap(x+k, y+l) && heatMap[x+k][y+l] == currentHeat) {
				heatMap[x+k][y+l] = max(1, heatMap[x+k][y+l]/10);
				*totalHeat -= (currentHeat - heatMap[x+k][y+l]);
			}
		}
	}
}

// Returns false if no place could be found.
// That should happen only if the total heat is zero.
boolean getItemSpawnLoc(unsigned short heatMap[DCOLS][DROWS], short *x, short *y, unsigned long *totalHeat) {
	unsigned long randIndex;
	unsigned short currentHeat;
	short i, j;
	
	if (*totalHeat <= 0) {
		return false;
	}
	
	randIndex = rand_range(1, *totalHeat);
	
	//printf("\nrandIndex: %i", randIndex);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			currentHeat = heatMap[i][j];
			if (randIndex <= currentHeat) { // this is the spot!
				*x = i;
				*y = j;
				return true;
			}
			randIndex -= currentHeat;
		}
	}
#ifdef BROGUE_ASSERTS
	assert(0); // should never get here!
#endif
	return false;
}

#define aggregateGoldLowerBound(d)	(pow((double) (d), 3.05) + 320 * (d))
#define aggregateGoldUpperBound(d)	(pow((double) (d), 3.05) + 420 * (d))

// Generates and places items for the level. Must pass the location of the up-stairway on the level.
void populateItems(short upstairsX, short upstairsY) {
	if (!ITEMS_ENABLED) {
		return;
	}
	item *theItem;
	unsigned short itemSpawnHeatMap[DCOLS][DROWS];
	short i, j, numberOfItems, numberOfGoldPiles, goldBonusProbability, x = 0, y = 0;
	unsigned long totalHeat;
	short theCategory, theKind;
	
#ifdef AUDIT_RNG
	char RNGmessage[100];
#endif
	
	if (rogue.depthLevel > AMULET_LEVEL) {
		numberOfItems = 3;
		numberOfGoldPiles = 0;
	} else {
		rogue.strengthPotionFrequency += 17;
		rogue.enchantScrollFrequency += 30;
		numberOfItems = 3;
		while (rand_percent(60)) {
			numberOfItems++;
		}
		if (rogue.depthLevel <= 3) {
			numberOfItems += 2; // 6 extra items to kickstart your career as a rogue
		} else if (rogue.depthLevel <= 5) {
			numberOfItems++; // and 2 more here
		}

		numberOfGoldPiles = min(5, (int) (rogue.depthLevel / 4));
		for (goldBonusProbability = 60;
			 rand_percent(goldBonusProbability) && numberOfGoldPiles <= 10;
			 goldBonusProbability -= 15) {
			
			numberOfGoldPiles++;
		}
		// Adjust the amount of gold if we're past depth 5 and we were below or above
		// the production schedule as of the previous depth.
		if (rogue.depthLevel > 5) {
			if (rogue.goldGenerated < aggregateGoldLowerBound(rogue.depthLevel - 1)) {
				numberOfGoldPiles += 2;
			} else if (rogue.goldGenerated > aggregateGoldUpperBound(rogue.depthLevel - 1)) {
				numberOfGoldPiles -= 2;
			}
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			itemSpawnHeatMap[i][j] = 50000;
		}
	}
	fillItemSpawnHeatMap(itemSpawnHeatMap, 5, upstairsX, upstairsY);
	totalHeat = 0;
	
#ifdef AUDIT_RNG
	sprintf(RNGmessage, "\n\nInitial heat map for level %i:\n", rogue.currentTurnNumber);
	RNGLog(RNGmessage);
#endif
	
	for (j=0; j<DROWS; j++) {
		for (i=0; i<DCOLS; i++) {
			
			if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_ITEMS | T_PATHING_BLOCKER)
				|| (pmap[i][j].flags & (IS_CHOKEPOINT | IN_LOOP | IS_IN_MACHINE))
				|| passableArcCount(i, j) > 1) { // not in hallways or quest rooms, please
				itemSpawnHeatMap[i][j] = 0;
			} else if (itemSpawnHeatMap[i][j] == 50000) {
				itemSpawnHeatMap[i][j] = 0;
				pmap[i][j].layers[DUNGEON] = PERM_WALL; // due to a bug that created occasional isolated one-cell islands;
				// not sure if it's still around, but this is a good-enough failsafe
			}
#ifdef AUDIT_RNG
			sprintf(RNGmessage, "%u%s%s\t%s",
					itemSpawnHeatMap[i][j],
					((pmap[i][j].flags & IS_CHOKEPOINT) ? " (C)": ""), // chokepoint
					((pmap[i][j].flags & IN_LOOP) ? " (L)": ""), // loop
					(i == DCOLS-1 ? "\n" : ""));
			RNGLog(RNGmessage);
#endif
			totalHeat += itemSpawnHeatMap[i][j];
		}
	}

	if (D_INSPECT_LEVELGEN) {
		short **map = allocDynamicGrid();
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				map[i][j] = itemSpawnHeatMap[i][j] * -1;
			}
		}
		dumpLevelToScreen();
		displayMap(map);
		freeDynamicGrid(map);
		message("Item spawn heat map:", true);
	}
	
	for (i=0; i<numberOfItems; i++) {
		theCategory = ALL_ITEMS & ~GOLD; // gold is placed separately, below, so it's not a punishment
		theKind = -1;
		
		scrollTable[SCROLL_ENCHANT_ITEM].frequency = rogue.enchantScrollFrequency;
		potionTable[POTION_GAIN_STRENGTH].frequency = rogue.strengthPotionFrequency;
		
		// Adjust the desired item category if necessary.
		if ((rogue.foodSpawned + foodTable[RATION].strengthRequired / 2) * 3
			<= pow(rogue.depthLevel, 1.3) * foodTable[RATION].strengthRequired * 0.55) {
			// guarantee a certain nutrition minimum of the equivalent of one ration every three levels,
			// with more food on deeper levels since they generally take more turns to complete
			theCategory = FOOD;
			if (rogue.depthLevel > AMULET_LEVEL) {
				numberOfItems++;
			}
		} else if (rogue.depthLevel > AMULET_LEVEL) {
			theCategory = GEM;
		}
		
		// Generate the item.
		theItem = generateItem(theCategory, theKind);
		
		if (theItem->category & FOOD) {
			rogue.foodSpawned += foodTable[theItem->kind].strengthRequired;
		}
		
		// Choose a placement location not in a hallway.
		do {
			if ((theItem->category & FOOD) || ((theItem->category & POTION) && theItem->kind == POTION_GAIN_STRENGTH)) {
				randomMatchingLocation(&x, &y, FLOOR, NOTHING, -1); // food and gain strength don't follow the heat map
			} else {
				getItemSpawnLoc(itemSpawnHeatMap, &x, &y, &totalHeat);
			}
		} while (passableArcCount(x, y) > 1);
#ifdef BROGUE_ASSERTS
		assert(coordinatesAreInMap(x, y));
#endif
		// Cool off the item spawning heat map at the chosen location:
		coolHeatMapAt(itemSpawnHeatMap, x, y, &totalHeat);
		
		// Regulate the frequency of enchantment scrolls and gain strength potions.
		if (theItem->category & SCROLL && theItem->kind == SCROLL_ENCHANT_ITEM) {
			rogue.enchantScrollFrequency -= 50;
			//DEBUG printf("\ndepth %i: %i enchant scrolls generated so far", rogue.depthLevel, ++enchantScrolls);
		} else if (theItem->category & POTION && theItem->kind == POTION_GAIN_STRENGTH) {
			//DEBUG printf("\n*** Depth %i: generated a strength potions at %i frequency!", rogue.depthLevel, rogue.strengthPotionFrequency);
			rogue.strengthPotionFrequency -= 50;
		}
		
		// Place the item.
		placeItem(theItem, x, y); // Random valid location already obtained according to heat map.
		
		if (D_INSPECT_LEVELGEN) {
			short **map = allocDynamicGrid();
			short i2, j2;
			for (i2=0; i2<DCOLS; i2++) {
				for (j2=0; j2<DROWS; j2++) {
					map[i2][j2] = itemSpawnHeatMap[i2][j2] * -1;
				}
			}
			dumpLevelToScreen();
			displayMap(map);
			freeDynamicGrid(map);
			plotCharWithColor(theItem->displayChar, mapToWindowX(x), mapToWindowY(y), black, purple);
			message("Added an item.", true);
		}
	}
	
	// Now generate gold.
	for (i=0; i<numberOfGoldPiles; i++) {
		theItem = generateItem(GOLD, -1);
		getItemSpawnLoc(itemSpawnHeatMap, &x, &y, &totalHeat);
		coolHeatMapAt(itemSpawnHeatMap, x, y, &totalHeat);
		placeItem(theItem, x, y);
		rogue.goldGenerated += theItem->quantity;
	}
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("Added gold.", true);
	}
	
	scrollTable[SCROLL_ENCHANT_ITEM].frequency	= 0;	// No enchant scrolls or strength potions can spawn except via initial
	potionTable[POTION_GAIN_STRENGTH].frequency	= 0;	// item population or blueprints that create them specifically.
	
	//DEBUG printf("\nD:%i: %lu gold generated so far.", rogue.depthLevel, rogue.goldGenerated);
}

// Name of this function is a bit misleading -- basically returns true iff the item will stack without consuming an extra slot
// i.e. if it's a throwing weapon with a sibling already in your pack. False for potions and scrolls.
boolean itemWillStackWithPack(item *theItem) {
	item *tempItem;
	if (!(theItem->quiverNumber)) {
		return false;
	} else {
		for (tempItem = packItems->nextItem;
			 tempItem != NULL && tempItem->quiverNumber != theItem->quiverNumber;
			 tempItem = tempItem->nextItem);
		return (tempItem ? true : false);
	}
}

void removeItemFrom(short x, short y) {
	short layer;
	
	pmap[x][y].flags &= ~HAS_ITEM;
	
	if (cellHasTerrainFlag(x, y, T_PROMOTES_ON_ITEM_PICKUP)) {
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & T_PROMOTES_ON_ITEM_PICKUP) {
				promoteTile(x, y, layer, false);
			}
		}
	}
}

// adds the item at (x,y) to the pack
void pickUpItemAt(short x, short y) {
	item *theItem;
	char buf[COLS], buf2[COLS];
	
	rogue.disturbed = true;
	
	// find the item
	theItem = itemAtLoc(x, y);
	
	if (!theItem) {
		message("Error: Expected item; item not found.", true);
		return;
	}
	
	theItem->flags |= ITEM_NAMED;
	
	if ((theItem->flags & ITEM_CATEGORY_IDS_ON_PICKUP)
		&& !(tableForItemCategory(theItem->category)[theItem->kind].identified)) {
		tableForItemCategory(theItem->category)[theItem->kind].identified = true;
		theItem->flags &= ~ITEM_CATEGORY_IDS_ON_PICKUP;
	}
	
	if (numberOfItemsInPack() < MAX_PACK_ITEMS || theItem->category & GOLD || itemWillStackWithPack(theItem)) {
		
		if (theItem->flags & ITEM_NO_PICKUP) { // no longer used			
			itemName(theItem, buf2, true, false, NULL); // include suffix but not article
			sprintf(buf, "the %s is stuck to the ground.", buf2);
			messageWithColor(buf, &itemMessageColor, false);
			return;
		}
		
		// remove from floor chain
		pmap[x][y].flags &= ~ITEM_DETECTED;
		
#ifdef BROGUE_ASSERTS
		assert(removeItemFromChain(theItem, floorItems));
#else
		removeItemFromChain(theItem, floorItems);
#endif
		
		if (theItem->category & GOLD) {
			rogue.gold += theItem->quantity; 
			sprintf(buf, "you found %i pieces of gold.", theItem->quantity);
			messageWithColor(buf, &itemMessageColor, false);
			deleteItem(theItem);
			removeItemFrom(x, y); // triggers tiles with T_PROMOTES_ON_ITEM_PICKUP
			return;
		}
		
		if (theItem->category & AMULET && numberOfMatchingPackItems(AMULET, 0, 0, false)) {
			message("you already have the Amulet of Yendor.", false); 
			deleteItem(theItem);
			return;
		}
		
		theItem = addItemToPack(theItem);
		
		itemName(theItem, buf2, true, true, NULL); // include suffix, article
		
		sprintf(buf, "you now have %s (%c).", buf2, theItem->inventoryLetter);
		messageWithColor(buf, &itemMessageColor, false);
		
		removeItemFrom(x, y); // triggers tiles with T_PROMOTES_ON_ITEM_PICKUP
	} else {
		theItem->flags |= ITEM_PLAYER_AVOIDS; // explore shouldn't try to pick it up more than once.
		itemName(theItem, buf2, false, true, NULL); // include article
		sprintf(buf, "Your pack is too full to pick up %s.", buf2);
		message(buf, false);
	}
}

item *addItemToPack(item *theItem) {
	item *previousItem, *tempItem;
	char itemLetter;
	
	theItem->flags |= ITEM_NAMED;
	
	// Can the item stack with another in the inventory?
	if (theItem->category & (FOOD|POTION|SCROLL|GEM)) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind) {
				// We found a match! Increment the quantity of the old item...
				tempItem->quantity++;
				
				// let magic detection and other flags propagate to the stack...
				tempItem->flags |= (theItem->flags & (ITEM_MAGIC_DETECTED | ITEM_IDENTIFIED | ITEM_PROTECTED | ITEM_NAMED | ITEM_RUNIC
													  | ITEM_RUNIC_HINTED | ITEM_CAN_BE_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN));
				
				// keep the higher enchantment and lower strength requirement...
				if (theItem->enchant1 > tempItem->enchant1) {
					tempItem->enchant1 = theItem->enchant1;
				}
				if (theItem->strengthRequired < tempItem->strengthRequired) {
					tempItem->strengthRequired = theItem->strengthRequired;
				}
				if (theItem->inscription && !tempItem->inscription) {
					strcpy(tempItem->inscription, theItem->inscription);
				}
				
				// and delete the new item.
				deleteItem(theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	} else if (theItem->category & WEAPON && theItem->quiverNumber > 0) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind
				&& theItem->quiverNumber == tempItem->quiverNumber) {
				
				// We found a match! Increase the quantity of the old item...
				tempItem->quantity += theItem->quantity;
				
				// let magic detection and other flags propagate to the stack...
				tempItem->flags |= (theItem->flags & (ITEM_MAGIC_DETECTED | ITEM_IDENTIFIED | ITEM_PROTECTED | ITEM_NAMED | ITEM_RUNIC
													  | ITEM_RUNIC_HINTED | ITEM_CAN_BE_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN));
				
				// keep the higher enchantment and lower strength requirement...
				if (theItem->enchant1 > tempItem->enchant1) {
					tempItem->enchant1 = theItem->enchant1;
				}
				if (theItem->strengthRequired < tempItem->strengthRequired) {
					tempItem->strengthRequired = theItem->strengthRequired;
				}
				if (theItem->inscription && !tempItem->inscription) {
					strcpy(tempItem->inscription, theItem->inscription);
				}
				
				// and delete the new item.
				deleteItem(theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	}
	
	// assign a reference letter to the item
	itemLetter = nextAvailableInventoryCharacter();
	if (itemLetter) {
		theItem->inventoryLetter = itemLetter;
	}
	
	// insert at proper place in pack chain
	for (previousItem = packItems;
		 previousItem->nextItem != NULL && previousItem->nextItem->category <= theItem->category;
		 previousItem = previousItem->nextItem);
	theItem->nextItem = previousItem->nextItem;
	previousItem->nextItem = theItem;
	
	return theItem;
}

short numberOfItemsInPack() {
	short theCount = 0;
	item *theItem;
	for(theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		theCount += (theItem->category & WEAPON ? 1 : theItem->quantity);
	}
	return theCount;
}

char nextAvailableInventoryCharacter() {
	boolean charTaken[26];
	short i;
	item *theItem;
	char c;
	for(i=0; i<26; i++) {
		charTaken[i] = false;
	}
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		c = theItem->inventoryLetter;
		if (c >= 'a' && c <= 'z') {
			charTaken[c - 'a'] = true;
		}
	}
	for(i=0; i<26; i++) {
		if (!charTaken[i]) {
			return ('a' + i);
		}
	}
	return 0;
}

boolean inscribeItem(item *theItem) {
	char itemText[30], buf[COLS], nameOfItem[COLS], oldInscription[COLS];
	
	strcpy(oldInscription, theItem->inscription);
	theItem->inscription[0] = '\0';
	itemName(theItem, nameOfItem, true, true, NULL);
	strcpy(theItem->inscription, oldInscription);
	
	sprintf(buf, "inscribe: %s \"", nameOfItem);
	if (getInputTextString(itemText, buf, 29, "", "\"", TEXT_INPUT_NORMAL, false)) {
		strcpy(theItem->inscription, itemText);
		confirmMessages();
		itemName(theItem, nameOfItem, true, true, NULL);
		nameOfItem[strlen(nameOfItem) - 1] = '\0';
		sprintf(buf, "%s %s.\"", (theItem->quantity > 1 ? "they're" : "it's"), nameOfItem);
		messageWithColor(buf, &itemMessageColor, false);
		return true;
	} else {
		confirmMessages();
		return false;
	}
}

boolean itemCanBeCalled(item *theItem) {
	
	updateIdentifiableItem(theItem); // Just in case.
	
	if ((theItem->flags & ITEM_IDENTIFIED) || theItem->category & (WEAPON|ARMOR|FOOD|GOLD|AMULET|GEM)) {
		if (theItem->category & (WEAPON | ARMOR | STAFF | WAND | RING)
			&& (theItem->flags & ITEM_CAN_BE_IDENTIFIED)) {
			
			return true;
		} else {
			return false;
		}
	}
	if ((theItem->category & (POTION|SCROLL|WAND|STAFF|RING))
		&& !tableForItemCategory(theItem->category)->identified) {
		return true;
	}
	return false;
}

void call(item *theItem) {
	char itemText[30], buf[COLS];
	short c;
	unsigned char command[100];
	
	c = 0;
	command[c++] = CALL_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType((WEAPON|ARMOR|SCROLL|RING|POTION|STAFF|WAND), 0, 0,
									  "Call what? (a-z, shift for more info; or <esc> to cancel)", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	command[c++] = theItem->inventoryLetter;
	
	confirmMessages();
	
	if ((theItem->flags & ITEM_IDENTIFIED) || theItem->category & (WEAPON|ARMOR|FOOD|GOLD|AMULET|GEM)) {
		if (theItem->category & (WEAPON | ARMOR | STAFF | WAND | RING)
			&& (theItem->flags & ITEM_CAN_BE_IDENTIFIED)) {
			
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
		} else {
			message("you already know what that is.", false);
		}
		return;
	}
	
	if ((theItem->flags & ITEM_CAN_BE_IDENTIFIED)
		&& (theItem->category & (WEAPON | ARMOR | STAFF | WAND | RING))) {
		if (confirm("Inscribe this particular item instead of all similar items?", true)) {
			command[c++] = 'y';
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
			return;
		} else {
			command[c++] = 'n'; // n means no
		}
	}
	
	if (getInputTextString(itemText, "call them: \"", 29, "", "\"", TEXT_INPUT_NORMAL, false)) {
		command[c++] = '\0';
		strcat((char *) command, itemText);
		recordKeystrokeSequence(command);
		recordKeystroke(RETURN_KEY, false, false);
		if (itemText[0]) {
			strcpy(tableForItemCategory(theItem->category)[theItem->kind].callTitle, itemText);
			tableForItemCategory(theItem->category)[theItem->kind].called = true;
		} else {
			tableForItemCategory(theItem->category)[theItem->kind].callTitle[0] = '\0';
			tableForItemCategory(theItem->category)[theItem->kind].called = false;
		}
		confirmMessages();
		itemName(theItem, buf, false, true, NULL);
		messageWithColor(buf, &itemMessageColor, false);
	} else {
		confirmMessages();
	}
}

// Generates the item name and returns it in the "root" string.
// IncludeDetails governs things such as enchantment, charges, strength requirement, times used, etc.
// IncludeArticle governs the article -- e.g. "some" food, "5" darts, "a" pink potion.
// If baseColor is provided, then the suffix will be in gray, flavor portions of the item name (e.g. a "pink" potion,
//	a "sandalwood" staff, a "ruby" ring) will be in dark purple, and the Amulet of Yendor and lumenstones will be in yellow.
//  BaseColor itself will be the color that the name reverts to outside of these colored portions.
void itemName(item *theItem, char *root, boolean includeDetails, boolean includeArticle, color *baseColor) {
	char buf[DCOLS], pluralization[10], article[10] = "",
	grayEscapeSequence[5], purpleEscapeSequence[5], yellowEscapeSequence[5], baseEscapeSequence[5];
	color tempColor;
	
	sprintf(pluralization, (theItem->quantity > 1 ? "s" : ""));
	
	grayEscapeSequence[0] = '\0';
	purpleEscapeSequence[0] = '\0';
	yellowEscapeSequence[0] = '\0';
	baseEscapeSequence[0] = '\0';
	if (baseColor) {
		tempColor = backgroundMessageColor;
		applyColorMultiplier(&tempColor, baseColor); // To gray out the purle if necessary.
		encodeMessageColor(purpleEscapeSequence, 0, &tempColor);
		
		tempColor = gray;
		//applyColorMultiplier(&tempColor, baseColor);
		encodeMessageColor(grayEscapeSequence, 0, &tempColor);
        
		tempColor = itemMessageColor;
		applyColorMultiplier(&tempColor, baseColor);
		encodeMessageColor(yellowEscapeSequence, 0, &tempColor);
		
		encodeMessageColor(baseEscapeSequence, 0, baseColor);
	}
	
	switch (theItem -> category) {
		case FOOD:
			if (theItem -> kind == FRUIT) {
				sprintf(root, "mango%s", pluralization);
			} else {
				if (theItem->quantity == 1) {
					sprintf(article, "some ");
					sprintf(root, "food");
				} else {
					sprintf(root, "ration%s of food", pluralization);
				}
			}
			break;
		case WEAPON:
			sprintf(root, "%s%s", weaponTable[theItem->kind].name, pluralization);
			if (includeDetails) {
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					sprintf(buf, "%s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
					strcpy(root, buf);
				}
				
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						if (theItem->enchant2 == W_SLAYING) {
							sprintf(root, "%s of %s slaying%s",
									root,
									monsterCatalog[theItem->vorpalEnemy].monsterName,
									grayEscapeSequence);
						} else {
							sprintf(root, "%s of %s%s",
									root,
									weaponRunicNames[theItem->enchant2],
									grayEscapeSequence);
						}
					} else if (theItem->flags & (ITEM_IDENTIFIED | ITEM_RUNIC_HINTED)) {
						if (grayEscapeSequence[0]) {
							strcat(root, grayEscapeSequence);
						}
						strcat(root, " (unknown runic)");
					}
				}
				sprintf(root, "%s%s <%i>", root, grayEscapeSequence, theItem->strengthRequired);
			}
			break;
		case ARMOR:
			sprintf(root, "%s", armorTable[theItem->kind].name);
			if (includeDetails) {
				
				if ((theItem->flags & ITEM_RUNIC)
					&& ((theItem->flags & ITEM_RUNIC_IDENTIFIED)
						|| rogue.playbackOmniscience)) {
						if (theItem->enchant2 == A_IMMUNITY) {
							sprintf(root, "%s of %s immunity", root, monsterCatalog[theItem->vorpalEnemy].monsterName);
						} else {
							sprintf(root, "%s of %s", root, armorRunicNames[theItem->enchant2]);
						}
					}
				
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					if (theItem->enchant1 == 0) {
						sprintf(buf, "%s%s [%i]<%i>", root, grayEscapeSequence, theItem->armor/10, theItem->strengthRequired);
					} else {
						sprintf(buf, "%s%i %s%s [%i]<%i>",
								(theItem->enchant1 < 0 ? "" : "+"),
								theItem->enchant1,
								root,
								grayEscapeSequence,
								theItem->armor/10 + theItem->enchant1,
								theItem->strengthRequired);
					}
					strcpy(root, buf);
				} else {
					sprintf(root, "%s%s <%i>", root, grayEscapeSequence, theItem->strengthRequired);
				}
				
				if ((theItem->flags & ITEM_RUNIC)
					&& (theItem->flags & (ITEM_IDENTIFIED | ITEM_RUNIC_HINTED))
					&& !(theItem->flags & ITEM_RUNIC_IDENTIFIED)
					&& !rogue.playbackOmniscience) {
					strcat(root, " (unknown runic)");
				}
			}
			break;
		case SCROLL:
			if (scrollTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "scroll%s of %s", pluralization, scrollTable[theItem->kind].name);
			} else if (scrollTable[theItem->kind].called) {
				sprintf(root, "scroll%s called %s%s%s",
						pluralization,
						purpleEscapeSequence,
						scrollTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "scroll%s entitled %s\"%s\"%s",
						pluralization,
						purpleEscapeSequence,
						scrollTable[theItem->kind].flavor,
						baseEscapeSequence);
			}
			break;
		case POTION:
			if (potionTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "potion%s of %s", pluralization, potionTable[theItem->kind].name);
			} else if (potionTable[theItem->kind].called) {
				sprintf(root, "potion%s called %s%s%s",
						pluralization,
						purpleEscapeSequence,
						potionTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s potion%s",
						purpleEscapeSequence,
						potionTable[theItem->kind].flavor,
						baseEscapeSequence,
						pluralization);
			}
			break;
		case WAND:
			if (wandTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "wand%s of %s",
						pluralization,
						wandTable[theItem->kind].name);
			} else if (wandTable[theItem->kind].called) {
				sprintf(root, "wand%s called %s%s%s",
						pluralization,
						purpleEscapeSequence,
						wandTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s wand%s",
						purpleEscapeSequence,
						wandTable[theItem->kind].flavor,
						baseEscapeSequence,
						pluralization);
			}
			if (includeDetails) {
				if (theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN) || rogue.playbackOmniscience) {
					sprintf(root, "%s%s [%i]",
							root,
							grayEscapeSequence,
							theItem->charges);
				} else if (theItem->enchant2 > 2) {
					sprintf(root, "%s%s (used %i times)",
							root,
							grayEscapeSequence,
							theItem->enchant2);
				} else if (theItem->enchant2) {
					sprintf(root, "%s%s (used %s)",
							root,
							grayEscapeSequence,
							(theItem->enchant2 == 2 ? "twice" : "once"));
				}
			}
			break;
		case STAFF:
			if (staffTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "staff%s of %s", pluralization, staffTable[theItem->kind].name);
			} else if (staffTable[theItem->kind].called) {
				sprintf(root, "staff%s called %s%s%s",
						pluralization,
						purpleEscapeSequence,
						staffTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s staff%s",
						purpleEscapeSequence,
						staffTable[theItem->kind].flavor,
						baseEscapeSequence,
						pluralization);
			}
			if (includeDetails) {
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					sprintf(root, "%s%s [%i/%i]", root, grayEscapeSequence, theItem->charges, theItem->enchant1);
				} else if (theItem->flags & ITEM_MAX_CHARGES_KNOWN) {
					sprintf(root, "%s%s [?/%i]", root, grayEscapeSequence, theItem->enchant1);
				}
			}
			break;
		case RING:
			if (ringTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "ring%s of %s", pluralization, ringTable[theItem->kind].name);
			} else if (ringTable[theItem->kind].called) {
				sprintf(root, "ring%s called %s%s%s",
						pluralization,
						purpleEscapeSequence,
						ringTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s ring%s",
						purpleEscapeSequence,
						ringTable[theItem->kind].flavor,
						baseEscapeSequence,
						pluralization);
			}
			if (includeDetails && ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)) {
				sprintf(buf, "%s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
				strcpy(root, buf);
			}
			break;
		case GOLD:
			sprintf(root, "gold piece%s", pluralization);
			break;
		case AMULET:
			sprintf(root, "%sAmulet%s of Yendor%s", yellowEscapeSequence, pluralization, baseEscapeSequence);
			break;
		case GEM:
			sprintf(root, "%slumenstone%s%s", yellowEscapeSequence, pluralization, baseEscapeSequence);
			break;
		case KEY:
			if (includeDetails && theItem->keyZ > 0 && theItem->keyZ != rogue.depthLevel) {
				sprintf(root, "%s%s%s from depth %i",
						keyTable[theItem->kind].name,
						pluralization,
						grayEscapeSequence,
						theItem->keyZ);
			} else {
				sprintf(root,
						keyTable[theItem->kind].name,
						"%s%s",
						pluralization);
			}
			break;
		default:
			sprintf(root, "unknown item%s", pluralization);
			break;
	}
	
	if (includeArticle) {
		// prepend number if quantity is over 1
		if (theItem->quantity > 1) {
			sprintf(article, "%i ", theItem->quantity);
		} else if (theItem->category & AMULET) {
			sprintf(article, "the ");
		} else if (!(theItem->category & ARMOR) && !(theItem->category & FOOD && theItem->kind == RATION)) {
			// otherwise prepend a/an if the item is not armor and not a ration of food;
			// armor gets no article, and "some food" was taken care of above.
			sprintf(article, "a%s ", (isVowel(root) ? "n" : ""));
		}
	}
	// strcat(buf, suffixID);
	if (includeArticle) {
		sprintf(buf, "%s%s", article, root);
		strcpy(root, buf);
	}
	
	if (includeDetails && theItem->inscription[0]) {
		if (theItem->flags & ITEM_CAN_BE_IDENTIFIED) {
			sprintf(buf, "%s \"%s\"", root, theItem->inscription);
			strcpy(root, buf);
		} else {
			theItem->inscription[0] = '\0';
		}
	}
	return;
}

itemTable *tableForItemCategory(enum itemCategory theCat) {
	switch (theCat) {
		case FOOD:
			return foodTable;
		case WEAPON:
			return weaponTable;
		case ARMOR:
			return armorTable;
		case POTION:
			return potionTable;
		case SCROLL:
			return scrollTable;
		case RING:
			return ringTable;
		case WAND:
			return wandTable;
		case STAFF:
			return staffTable;
		default:
			return NULL;
	}
}

boolean isVowel(char *theChar) {
	while (*theChar == COLOR_ESCAPE) {
		theChar += 4;
	}
	return (*theChar == 'a' || *theChar == 'e' || *theChar == 'i' || *theChar == 'o' || *theChar == 'u' ||
			*theChar == 'A' || *theChar == 'E' || *theChar == 'I' || *theChar == 'O' || *theChar == 'U');
}

float enchantIncrement(item *theItem) {
	if (theItem->category & (WEAPON | ARMOR)) {
		if (theItem->strengthRequired == 0) {
			return 1 + 0;
		} else if (rogue.strength - player.weaknessAmount < theItem->strengthRequired) {
			return 1 + 2.5;
		} else {
			return 1 + 0.25;
		}
	} else {
		return 1 + 0;
	}
}

boolean itemIsCarried(item *theItem) {
	item *tempItem;
	
	for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
		if (tempItem == theItem) {
			return true;
		}
	}
	return false;
}

void itemDetails(char *buf, item *theItem) {
	char buf2[1000], buf3[1000], theName[100];
	boolean singular, carried;
	float enchant;
	short nextLevelState = 0;
	float accuracyChange, damageChange, current, new, currentDamage, newDamage;
	const char weaponRunicEffectDescriptions[NUMBER_WEAPON_RUNIC_KINDS][DCOLS] = {
		"time will stop while you take an extra turn",
		"the enemy will die instantly",
		"the enemy will be paralyzed",
		"[multiplicity]", // never used
		"the enemy will be slowed",
		"the enemy will be confused",
		"[slaying]", // never used
		"the enemy will be healed",
		"the enemy will be cloned"
	};
	
	singular = (theItem->quantity == 1 ? true : false);
	carried = itemIsCarried(theItem);
	
	// Name
	itemName(theItem, theName, true, true, NULL);
	buf[0] = '\0';
	encodeMessageColor(buf, 0, &itemMessageColor);
	upperCase(theName);
	strcat(buf, theName);
	if (carried) {
		sprintf(buf2, " (%c)", theItem->inventoryLetter);
		strcat(buf, buf2);
	}
	encodeMessageColor(buf, strlen(buf), &white);
	strcat(buf, "\n\n");
	
	itemName(theItem, theName, false, false, NULL);
	
	// introductory text
	if (tableForItemCategory(theItem->category)
		&& (tableForItemCategory(theItem->category)[theItem->kind].identified || rogue.playbackOmniscience)) {
		
		strcat(buf, tableForItemCategory(theItem->category)[theItem->kind].description);
	} else {
		switch (theItem->category) {
			case POTION:
				sprintf(buf2, "%s flask%s contain%s a swirling %s liquid. \
Who knows what %s will do when drunk or thrown?",
						(singular ? "This" : "These"),
						(singular ? "" : "s"),
						(singular ? "s" : ""),
						tableForItemCategory(theItem->category)[theItem->kind].flavor,
						(singular ? "it" : "they"));
				break;
			case SCROLL:
				sprintf(buf2, "%s parchment%s %s covered with indecipherable writing, and bear%s a title of \"%s.\" \
Who knows what %s will do when read aloud?",
						(singular ? "This" : "These"),
						(singular ? "" : "s"),
						(singular ? "is" : "are"),
						(singular ? "s" : ""),
						tableForItemCategory(theItem->category)[theItem->kind].flavor,
						(singular ? "it" : "they"));
				break;
			case STAFF:
				sprintf(buf2, "This gnarled %s staff is warm to the touch. \
Who knows what it will do when used?",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case WAND:
				sprintf(buf2, "This thin %s wand is warm to the touch. \
Who knows what it will do when used?",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case RING:
				sprintf(buf2, "This metal band is adorned with a large %s gem that glitters in the darkness. \
Who knows what effect it has when worn? ",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case AMULET:
				strcpy(buf2, "Legends are told about this mysterious golden amulet, \
and hundreds of adventurers have perished in its pursuit. Unfathomable power and riches await anyone with the skill and ambition \
to carry it into the light of day.");
				break;
			case GEM:
				sprintf(buf2, "Mysterious lights swirl and fluoresce beneath the stone%s surface. \
Lumenstones are said to contain mysterious properties of untold power, but for you, they mean one thing: riches.",
						(singular ? "'s" : "s'"));
				break;
			case KEY:
				strcpy(buf2, keyTable[theItem->kind].description);
				break;
			case GOLD:
				sprintf(buf2, "A pile of %i shining gold coins.", theItem->quantity);
				break;
			default:
				break;
		}
		strcat(buf, buf2);
	}
	
	// detailed description
	switch (theItem->category) {
			
		case FOOD:
			sprintf(buf2, "\n\nYou are %shungry enough to fully enjoy a %s.",
					((STOMACH_SIZE - player.status[STATUS_NUTRITION]) >= foodTable[theItem->kind].strengthRequired ? "" : "not yet "),
					(theItem->kind == RATION ? "ration of food" : theName));
			strcat(buf, buf2);
			break;
			
		case WEAPON:
		case ARMOR:
			// enchanted? strength modifier?
			if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
				if (theItem->enchant1) {
					sprintf(buf2, "\n\nThe %s bear%s an intrinsic %s%i",
							theName,
							(singular ? "s" : ""),
							(theItem->enchant1 > 0 ? "enchantment of +" : "penalty of "),
							theItem->enchant1);
				} else {
					sprintf(buf2, "\n\nThe %s bear%s no intrinsic enchantment",
							theName,
							(singular ? "s" : ""));
				}
				strcat(buf, buf2);
				if (strengthModifier(theItem)) {
					sprintf(buf2, ", %s %s %s %s%.2f because of your %s strength. ",
							(theItem->enchant1 ? "and" : "but"),
							(singular ? "carries" : "carry"),
							(theItem->enchant1 && (theItem->enchant1 > 0) == (strengthModifier(theItem) > 0) ? "an additional" : "a"),
							(strengthModifier(theItem) > 0 ? "bonus of +" : "penalty of "),
							strengthModifier(theItem),
							(strengthModifier(theItem) > 0 ? "excess" : "inadequate"));
					strcat(buf, buf2);
				} else {
					strcat(buf, ". ");
				}
			} else {
				if ((theItem->enchant1 > 0) && (theItem->flags & ITEM_MAGIC_DETECTED)) {
					sprintf(buf2, "\n\nYou can feel an aura of benevolent magic radiating from the %s. ",
							theName);
					strcat(buf, buf2);
				}
				if (strengthModifier(theItem)) {
					sprintf(buf2, "\n\nThe %s %s%s a %s%.2f because of your %s strength. ",
							theName,
							((theItem->enchant1 > 0) && (theItem->flags & ITEM_MAGIC_DETECTED) ? "also " : ""),
							(singular ? "carries" : "carry"),
							(strengthModifier(theItem) > 0 ? "bonus of +" : "penalty of "),
							strengthModifier(theItem),
							(strengthModifier(theItem) > 0 ? "excess" : "inadequate"));
					strcat(buf, buf2);
				}
				
				if (theItem->category & WEAPON) {
					sprintf(buf2, "It will reveal its secrets to you if you defeat %i%s %s with it. ",
							theItem->charges,
							(theItem->charges == WEAPON_KILLS_TO_AUTO_ID ? "" : " more"),
							(theItem->charges == 1 ? "enemy" : "enemies"));
				} else {
					sprintf(buf2, "It will reveal its secrets to you if you wear it for %i%s turn%s. ",
							theItem->charges,
							(theItem->charges == ARMOR_DELAY_TO_AUTO_ID ? "" : " more"),
							(theItem->charges == 1 ? "" : "s"));
				}
				strcat(buf, buf2);
			}
			
			// Display the known percentage by which the armor/weapon will increase/decrease accuracy/damage/defense if not already equipped.
			if (!(theItem->flags & ITEM_EQUIPPED)) {
				if (theItem->category & WEAPON) {
					current = player.info.accuracy;
					if (rogue.weapon) {
                        currentDamage = (((float) rogue.weapon->damage.lowerBound) + ((float) rogue.weapon->damage.upperBound)) / 2;
						if ((rogue.weapon->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
							current += 5 * netEnchant(rogue.weapon);
							currentDamage *= pow(WEAPON_ENCHANT_FACTOR, netEnchant(rogue.weapon));
						} else {
							current += 5 * strengthModifier(rogue.weapon);
							currentDamage *= pow(WEAPON_ENCHANT_FACTOR, strengthModifier(rogue.weapon));
						}
					} else {
                        currentDamage = (((float) player.info.damage.lowerBound) + ((float) player.info.damage.upperBound)) / 2;
                    }
					
					new = player.info.accuracy;
					newDamage = (((float) theItem->damage.lowerBound) + ((float) theItem->damage.upperBound)) / 2;
					if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
						new += 5 * netEnchant(theItem);
						newDamage *= pow(WEAPON_ENCHANT_FACTOR, netEnchant(theItem));
					} else {
						new += 5 * strengthModifier(theItem);
						newDamage *= pow(WEAPON_ENCHANT_FACTOR, strengthModifier(theItem));
					}
					accuracyChange	= (new * 100 / current) - 100;
					damageChange	= (newDamage * 100 / currentDamage) - 100;
					sprintf(buf2, "Wielding the %s%s will %s your current accuracy by %i%%, and will %s your current damage by %i%%. ",
							theName,
							((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) ? "" : ", assuming it has no hidden properties,",
							(((short) accuracyChange) < 0) ? "decrease" : "increase",
							abs((short) accuracyChange),
							(((short) damageChange) < 0) ? "decrease" : "increase",
							abs((short) damageChange));
				} else {
					new = theItem->armor;
					if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
						new += 10 * netEnchant(theItem);
					} else {
						new += 10 * strengthModifier(theItem);
					}
					new = max(0, new);
					sprintf(buf2, "Wearing the %s%s will result in an armor rating of %i. ",
							theName,
							((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) ? "" : ", assuming it has no hidden properties,",
							(int) (new / 10));
				}
				strcat(buf, buf2);
			}
			
			// protected?
			if (theItem->flags & ITEM_PROTECTED) {
				sprintf(buf2, "The %s cannot be corroded by acid.",
						theName);
				strcat(buf, buf2);
			}
			
			if (theItem->category & WEAPON) {
				
				// runic?
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						sprintf(buf2, "\n\nGlowing runes of %s adorn the %s. ",
								weaponRunicNames[theItem->enchant2],
								theName);
						strcat(buf, buf2);
						
						// W_SPEED, W_QUIETUS, W_PARALYSIS, W_MULTIPLICITY, W_SLOWING, W_CONFUSION, W_SLAYING, W_MERCY, W_PLENTY
						
						enchant = netEnchant(theItem);
						if (theItem->enchant2 == W_SLAYING) {
							sprintf(buf2, "It will never fail to slay a %s in a single stroke. ",
									monsterCatalog[theItem->vorpalEnemy].monsterName);
							strcat(buf, buf2);
						} else if (theItem->enchant2 == W_MULTIPLICITY) {
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
								sprintf(buf2, "%i%% of the time that it hits an enemy, %i spectral %s%s will spring into being with accuracy and attack power equal to your own, and will dissipate %i turns later. (If the %s is enchanted, %i image%s will appear %i%% of the time, and will last %i turns.)",
										runicWeaponChance(theItem, false, 0),
										weaponImageCount(enchant),
										theName,
										(weaponImageCount(enchant) > 1 ? "s" : ""),
										weaponImageDuration(enchant),
										theName,
										weaponImageCount((float) (enchant + enchantIncrement(theItem))),
										(weaponImageCount((float) (enchant + enchantIncrement(theItem))) > 1 ? "s" : ""),
										runicWeaponChance(theItem, true, (float) (enchant + enchantIncrement(theItem))),
										weaponImageDuration((float) (enchant + enchantIncrement(theItem))));
							} else {
								sprintf(buf2, "Sometimes, when it hits an enemy, spectral %ss will spring into being with accuracy and attack power equal to your own, and will dissipate shortly thereafter.",
										theName);
							}
							strcat(buf, buf2);
						} else {
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
                                if (runicWeaponChance(theItem, false, 0) < 2
                                    && rogue.strength - player.weaknessAmount < theItem->strengthRequired) {
                                    
                                    strcpy(buf2, "Its runic effect will almost never activate because of your inadequate strength, but sometimes, when");
                                } else {
                                    sprintf(buf2, "%i%% of the time that",
                                            runicWeaponChance(theItem, false, 0));
                                }
								strcat(buf, buf2);
							} else {
								strcat(buf, "Sometimes, when");
							}
							sprintf(buf2, " it hits an enemy, %s",
									weaponRunicEffectDescriptions[theItem->enchant2]);
							strcat(buf, buf2);
							
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
								switch (theItem->enchant2) {
									case W_SPEED:
										strcat(buf, ". ");
										break;
									case W_PARALYSIS:
										sprintf(buf2, " for %i turns. ",
												(int) (weaponParalysisDuration(enchant)));
										strcat(buf, buf2);
										nextLevelState = (int) (weaponParalysisDuration((float) (enchant + enchantIncrement(theItem))));
										break;
									case W_SLOWING:
										sprintf(buf2, " for %i turns. ",
												weaponSlowDuration(enchant));
										strcat(buf, buf2);
										nextLevelState = weaponSlowDuration((float) (enchant + enchantIncrement(theItem)));
										break;
									case W_CONFUSION:
										sprintf(buf2, " for %i turns. ",
												weaponConfusionDuration(enchant));
										strcat(buf, buf2);
										nextLevelState = weaponConfusionDuration((float) (enchant + enchantIncrement(theItem)));
										break;
									case W_MERCY:
										strcpy(buf2, " by 50% of its maximum health. ");
										strcat(buf, buf2);
										break;
									default:
										strcpy(buf2, ". ");
										strcat(buf, buf2);
										break;
								}
								
								if (((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)
									&& runicWeaponChance(theItem, false, 0) < runicWeaponChance(theItem, true, (float) (enchant + enchantIncrement(theItem)))){
									sprintf(buf2, "(If the %s is enchanted, the chance will increase to %i%%",
											theName,
											runicWeaponChance(theItem, true, (float) (enchant + enchantIncrement(theItem))));
									strcat(buf, buf2);
									if (nextLevelState) {
										sprintf(buf2, " and the duration will increase to %i turns.)",
												nextLevelState);
									} else {
										strcpy(buf2, ".)");
									}
									strcat(buf, buf2);
								}
							} else {
								strcat(buf, ". ");
							}
						}
						
					} else if (theItem->flags & ITEM_IDENTIFIED) {
						sprintf(buf2, "\n\nGlowing runes of an indecipherable language run down the length of the %s. ",
								theName);
						strcat(buf, buf2);
					}
				}
				
				// equipped? cursed?
				if (theItem->flags & ITEM_EQUIPPED) {
					sprintf(buf2, "\n\nYou hold the %s at the ready%s. ",
							theName,
							((theItem->flags & ITEM_CURSED) ? ", and because it is cursed, you are powerless to let go" : ""));
					strcat(buf, buf2);
				} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
						   && (theItem->flags & ITEM_CURSED)) {
					sprintf(buf2, "\n\nYou can feel a malevolent magic lurking within the %s. ", theName);
					strcat(buf, buf2);
				}
				
			} else if (theItem->category & ARMOR) {
				
				// runic?
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						sprintf(buf2, "\n\nGlowing runes of %s adorn the %s. ",
								armorRunicNames[theItem->enchant2],
								theName);
						strcat(buf, buf2);
						
						// A_MULTIPLICITY, A_MUTUALITY, A_ABSORPTION, A_REPRISAL, A_IMMUNITY, A_REFLECTION, A_BURDEN, A_VULNERABILITY, A_IMMOLATION
						enchant = netEnchant(theItem);
						switch (theItem->enchant2) {
							case A_MULTIPLICITY:
								sprintf(buf2, "When worn, 33%% of the time that an enemy's attack connects, %i allied spectral duplicate%s of your attacker will appear for 3 turns. ",
										armorImageCount(enchant),
										(armorImageCount(enchant) == 1 ? "" : "s"));
								if (armorImageCount((float) enchant + enchantIncrement(theItem)) > armorImageCount(enchant)) {
									sprintf(buf3, "(If the %s is enchanted, the number of duplicates will increase to %i.) ",
											theName,
											(armorImageCount((float) enchant + enchantIncrement(theItem))));
									strcat(buf2, buf3);
								}
								break;
							case A_MUTUALITY:
								strcpy(buf2, "When worn, the damage that you incur from physical attacks will be split evenly among yourself and all other adjacent enemies. ");
								break;
							case A_ABSORPTION:
								sprintf(buf2, "It will reduce the damage of inbound attacks by a random amount between 0 and %i, which is %i%% of your current maximum health. (If the %s is enchanted, this maximum amount will %s %i.) ",
										(int) armorAbsorptionMax(enchant),
										(int) (100 * armorAbsorptionMax(enchant) / player.info.maxHP),
										theName,
										(armorAbsorptionMax(enchant) == armorAbsorptionMax((float) (enchant + enchantIncrement(theItem))) ? "remain at" : "increase to"),
										(int) armorAbsorptionMax((float) (enchant + enchantIncrement(theItem))));
								break;
							case A_REPRISAL:
								sprintf(buf2, "Any enemy that attacks you will itself be wounded by %i%% of the damage that it inflicts. (If the %s is enchanted, this percentage will increase to %i%%.) ",
										armorReprisalPercent(enchant),
										theName,
										armorReprisalPercent((float) (enchant + enchantIncrement(theItem))));
								break;
							case A_IMMUNITY:
								sprintf(buf2, "It offers complete protection from any attacking %s. ",
										monsterCatalog[theItem->vorpalEnemy].monsterName);
								break;
							case A_REFLECTION:
								if (theItem->enchant1 > 0) {
									short reflectChance = reflectionChance(enchant);
									short reflectChance2 = reflectionChance(enchant + enchantIncrement(theItem));
									sprintf(buf2, "When worn, you will deflect %i%% of incoming spells -- including directly back at their source %i%% of the time. (If the armor is enchanted, these will increase to %i%% and %i%%.)",
											reflectChance,
											reflectChance * reflectChance / 100,
											reflectChance2,
											reflectChance2 * reflectChance2 / 100);
								} else {
									short reflectChance = reflectionChance(enchant);
									short reflectChance2 = reflectionChance(enchant + enchantIncrement(theItem));
									sprintf(buf2, "When worn, %i%% of your own spells will deflect from their target -- including directly back at you %i%% of the time. (If the armor is enchanted, these will decrease to %i%% and %i%%.)",
											reflectChance,
											reflectChance * reflectChance / 100,
											reflectChance2,
											reflectChance2 * reflectChance2 / 100);
								}
								break;
							case A_BURDEN:
								strcpy(buf2, "10% of the time it absorbs a blow, it will permanently become heavier. ");
								break;
							case A_VULNERABILITY:
								strcpy(buf2, "While it is worn, inbound attacks will inflict twice as much damage. ");
								break;
                            case A_IMMOLATION:
								strcpy(buf2, "10% of the time it absorbs a blow, it will explode in flames. ");
								break;
							default:
								break;
						}
						strcat(buf, buf2);
					} else if (theItem->flags & ITEM_IDENTIFIED) {
						sprintf(buf2, "\n\nGlowing runes of an indecipherable language spiral around the %s. ",
								theName);
						strcat(buf, buf2);
					}
				}
				
				// equipped? cursed?
				if (theItem->flags & ITEM_EQUIPPED) {
					sprintf(buf2, "\n\nYou are wearing the %s%s. ",
							theName,
							((theItem->flags & ITEM_CURSED) ? ", and because it is cursed, you are powerless to remove it" : ""));
					strcat(buf, buf2);
				} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
						   && (theItem->flags & ITEM_CURSED)) {
					sprintf(buf2, "\n\nYou can feel a malevolent magic lurking within the %s. ", theName);
					strcat(buf, buf2);
				}
				
			}
			break;
			
		case STAFF:
			enchant = theItem->enchant1;
			
			// charges
			if ((theItem->flags & ITEM_IDENTIFIED)  || rogue.playbackOmniscience) {
				sprintf(buf2, "\n\nThe %s has %i charges remaining out of a maximum of %i charges, and like all staffs, recovers its charges gradually over time. ",
						theName,
						theItem->charges,
						theItem->enchant1);
				strcat(buf, buf2);
			} else if (theItem->flags & ITEM_MAX_CHARGES_KNOWN) {
				sprintf(buf2, "\n\nThe %s has a maximum of %i charges, and like all staffs, recovers its charges gradually over time. ",
						theName,
						theItem->enchant1);
				strcat(buf, buf2);
			}
			
			// effect description
			if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) && staffTable[theItem->kind].identified)
				|| rogue.playbackOmniscience) {
				switch (theItem->kind) {
						// STAFF_LIGHTNING, STAFF_FIRE, STAFF_POISON, STAFF_TUNNELING, STAFF_BLINKING, STAFF_ENTRANCEMENT, STAFF_HEALING,
						// STAFF_HASTE, STAFF_OBSTRUCTION, STAFF_DISCORD, STAFF_CONJURATION
					case STAFF_LIGHTNING:
						sprintf(buf2, "This staff deals damage to every creature in its line of fire; nothing is immune. (If the staff is enchanted, its average damage will increase by %i%%.)",
								(int) (100 * (staffDamageLow(enchant + 1) + staffDamageHigh(enchant + 1)) / (staffDamageLow(enchant) + staffDamageHigh(enchant)) - 100));
						break;
					case STAFF_FIRE:
						sprintf(buf2, "This staff deals damage to any creature that it hits, unless the creature is immune to fire. (If the staff is enchanted, its average damage will increase by %i%%.) It also sets creatures and flammable terrain on fire.",
								(int) (100 * (staffDamageLow(enchant + 1) + staffDamageHigh(enchant + 1)) / (staffDamageLow(enchant) + staffDamageHigh(enchant)) - 100));
						break;
					case STAFF_POISON:
						sprintf(buf2, "The bolt from this staff will poison any creature that it hits for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffPoison(enchant),
								staffPoison(enchant + 1));
						break;
					case STAFF_TUNNELING:
						sprintf(buf2, "The bolt from this staff will dissolve %i layers of obstruction. (If the staff is enchanted, this will increase to %i layers.)",
								theItem->enchant1,
								theItem->enchant1 + 1);
						break;
					case STAFF_BLINKING:
						sprintf(buf2, "This staff enables you to teleport up to %i meters. (If the staff is enchanted, this will increase to %i meters.) It recharges half as quickly as most other kinds of staffs.",
								staffBlinkDistance(theItem->enchant1),
								staffBlinkDistance(theItem->enchant1 + 1));
						break;
					case STAFF_ENTRANCEMENT:
						sprintf(buf2, "This staff will compel its target to mirror your movements for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffEntrancementDuration(theItem->enchant1),
								staffEntrancementDuration(theItem->enchant1 + 1));
						break;
					case STAFF_HEALING:
						if (enchant < 10) {
							sprintf(buf2, "This staff will heal its target by %i%% of its maximum health. (If the staff is enchanted, this will increase to %i%%.)",
									theItem->enchant1 * 10,
									(theItem->enchant1 + 1) * 10);
						} else {
							strcpy(buf2, "This staff will completely heal its target.");	
						}
						break;
					case STAFF_HASTE:
						sprintf(buf2, "This staff will cause its target to move twice as fast for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffHasteDuration(theItem->enchant1),
								staffHasteDuration(theItem->enchant1 + 1));
						break;
					case STAFF_OBSTRUCTION:
						strcpy(buf2, "This staff recharges half as quickly as most other kinds of staffs.");
						break;
					case STAFF_DISCORD:
						sprintf(buf2, "This staff will cause discord for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffDiscordDuration(theItem->enchant1),
								staffDiscordDuration(theItem->enchant1 + 1));
						break;
					case STAFF_CONJURATION:
						sprintf(buf2, "%i phantom blades will be called into service. (If the staff is enchanted, this will increase to %i blades.)",
								staffBladeCount(theItem->enchant1),
								staffBladeCount(theItem->enchant1 + 1));
						break;
					case STAFF_PROTECTION:
						sprintf(buf2, "This staff will shield a creature for up to 20 turns against up to %i damage. (If the staff is enchanted, this will increase to %i damage.)",
								staffProtection(theItem->enchant1) / 10,
								staffProtection(theItem->enchant1 + 1) / 10);
						break;
					default:
						strcpy(buf2, "No one knows what this staff does.");
						break;
				}
				strcat(buf, "\n\n");
				strcat(buf, buf2);
			}
			break;
			
		case WAND:
			strcat(buf, "\n\n");
			if ((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) || rogue.playbackOmniscience) {
				if (theItem->charges) {
					sprintf(buf2, "%i charge%s remain%s. A charge can be added to the wand with a scroll of recharging or a scroll of enchantment.",
							theItem->charges,
							(theItem->charges == 1 ? "" : "s"),
							(theItem->charges == 1 ? "s" : ""));
				} else {
					strcpy(buf2, "No charges remain. A charge can be added to the wand with a scroll of recharging or a scroll of enchantment.");
				}
			} else {
				if (theItem->enchant2) {
					sprintf(buf2, "You have used this wand %i time%s, but do not know how many charges, if any, remain.",
							theItem->enchant2,
							(theItem->enchant2 == 1 ? "" : "s"));
				} else {
					strcpy(buf2, "You have not yet used this wand.");
				}
				
				if (wandTable[theItem->kind].identified) {
					strcat(buf, buf2);
					sprintf(buf2, " Wands of this type can be found with %i to %i charges.",
							wandTable[theItem->kind].range.lowerBound,
							wandTable[theItem->kind].range.upperBound);
				}
			}
			strcat(buf, buf2);
			break;
			
		case RING:
			// RING_CLAIRVOYANCE, RING_STEALTH, RING_REGENERATION, RING_TRANSFERENCE, RING_LIGHT, RING_AWARENESS, RING_WISDOM
			if ((((theItem->flags & ITEM_IDENTIFIED) && ringTable[theItem->kind].identified) || rogue.playbackOmniscience)
				&& theItem->enchant1) {
				switch (theItem->kind) {
					case RING_CLAIRVOYANCE:
						if (theItem->enchant1 > 0) {
							sprintf(buf2, "\n\nThis ring provides magical sight with a radius of %i. (If the ring is enchanted, this will increase to %i.)",
									theItem->enchant1 + 1,
									theItem->enchant1 + 2);
						} else {
							sprintf(buf2, "\n\nThis ring magically blinds you to a radius of %i. (If the ring is enchanted, this will decrease to %i.)",
									(theItem->enchant1 * -1) + 1,
									(theItem->enchant1 * -1));
						}
						strcat(buf, buf2);
						break;
					case RING_REGENERATION:
						sprintf(buf2, "\n\nWith this ring equipped, you will regenerate all of your health in %li turns (instead of %li). (If the ring is enchanted, this will decrease to %li turns.)",
								(long) (turnsForFullRegen(theItem->enchant1) / 1000),
								(long) TURNS_FOR_FULL_REGEN,
								(long) (turnsForFullRegen(theItem->enchant1 + 1) / 1000));
						strcat(buf, buf2);
						break;
					case RING_TRANSFERENCE:
						sprintf(buf2, "\n\nEach blow you land will %s you by %i%% of the damage you inflict. (If the ring is enchanted, this will %s to %i%%.)",
								(theItem->enchant1 > 0 ? "heal" : "harm"),
								abs(theItem->enchant1) * 10,
								(theItem->enchant1 > 0 ? "increase" : "decrease"),
								abs(theItem->enchant1 + 1) * 10);
						strcat(buf, buf2);
						break;
					case RING_WISDOM:
						sprintf(buf2, "\n\nWhen worn, your staffs will recharge at %i%% of their normal rate. (If the ring is enchanted, the rate will increase to %i%% of the normal rate.)",
								(int) (100 * pow(1.3, min(27, theItem->enchant1))),
								(int) (100 * pow(1.3, min(27, (theItem->enchant1 + 1)))));
						strcat(buf, buf2);
						break;
					default:
						break;
				}
			} else {
				sprintf(buf2, " It will reveal its secrets to you if you wear it for %i%s turn%s.",
						theItem->charges,
						(theItem->charges == RING_DELAY_TO_AUTO_ID ? "" : " more"),
						(theItem->charges == 1 ? "" : "s"));
				strcat(buf, buf2);
			}
			
			// equipped? cursed?
			if (theItem->flags & ITEM_EQUIPPED) {
				sprintf(buf2, "\n\nThe %s is on your finger%s. ",
						theName,
						((theItem->flags & ITEM_CURSED) ? ", and because it is cursed, you are powerless to remove it" : ""));
				strcat(buf, buf2);
			} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
					   && (theItem->flags & ITEM_CURSED)) {
				sprintf(buf2, "\n\nYou can feel a malevolent magic lurking within the %s. ", theName);
				strcat(buf, buf2);
			}
			break;
			
		default:
			break;
	}
}

boolean displayMagicCharForItem(item *theItem) {
	if (!(theItem->flags & ITEM_MAGIC_DETECTED)
		|| (theItem->category & PRENAMED_CATEGORY)) {
		return false;
	}
	if (theItem->category & (STAFF | POTION | SCROLL | WAND)) {
		return !(tableForItemCategory(theItem->category)[theItem->kind].identified);
	}
	return true;
}

char displayInventory(unsigned short categoryMask,
					  unsigned long requiredFlags,
					  unsigned long forbiddenFlags,
					  boolean waitForAcknowledge,
					  boolean includeButtons) {
	item *theItem;
	short i, j, m, maxLength = 0, itemNumber, itemCount, equippedItemCount;
	short extraLineCount = 0;
	item *itemList[DROWS];
	char buf[DCOLS*3];
	char theKey;
	rogueEvent theEvent;
	boolean magicDetected, repeatDisplay;
	short highlightItemLine, itemSpaceRemaining;
	cellDisplayBuffer dbuf[COLS][ROWS];
	cellDisplayBuffer rbuf[COLS][ROWS];
	brogueButton buttons[50] = {{{0}}};
	short actionKey;
	color darkItemColor;
	
	char whiteColorEscapeSequence[20],
	grayColorEscapeSequence[20],
	yellowColorEscapeSequence[20],
	darkYellowColorEscapeSequence[20],
	goodColorEscapeSequence[20],
	badColorEscapeSequence[20];
	char *magicEscapePtr;
	
	assureCosmeticRNG;
	
	clearCursorPath();
	clearDisplayBuffer(dbuf);
	
	whiteColorEscapeSequence[0] = '\0';
	encodeMessageColor(whiteColorEscapeSequence, 0, &white);
	grayColorEscapeSequence[0] = '\0';
	encodeMessageColor(grayColorEscapeSequence, 0, &gray);
	yellowColorEscapeSequence[0] = '\0';
	encodeMessageColor(yellowColorEscapeSequence, 0, &itemColor);
	darkItemColor = itemColor;
	applyColorAverage(&darkItemColor, &black, 50);
	darkYellowColorEscapeSequence[0] = '\0';
	encodeMessageColor(darkYellowColorEscapeSequence, 0, &darkItemColor);
	goodColorEscapeSequence[0] = '\0';
	encodeMessageColor(goodColorEscapeSequence, 0, &goodMessageColor);
	badColorEscapeSequence[0] = '\0';
	encodeMessageColor(badColorEscapeSequence, 0, &badMessageColor);
	
	if (packItems->nextItem == NULL) {
		confirmMessages();
		message("Your pack is empty!", false);
		restoreRNG;
		return 0;
	}
	
	magicDetected = false;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (displayMagicCharForItem(theItem) && (theItem->flags & ITEM_MAGIC_DETECTED)) {
			magicDetected = true;
		}
	}
	
	// List the items in the order we want to display them, with equipped items at the top.
	itemNumber = 0;
	equippedItemCount = 0;
	// First, the equipped weapon if any.
	if (rogue.weapon) {
		itemList[itemNumber] = rogue.weapon;
		itemNumber++;
		equippedItemCount++;
	}
	// Now, the equipped armor if any.
	if (rogue.armor) {
		itemList[itemNumber] = rogue.armor;
		itemNumber++;
		equippedItemCount++;
	}
	// Now, the equipped rings, if any.
	if (rogue.ringLeft) {
		itemList[itemNumber] = rogue.ringLeft;
		itemNumber++;
		equippedItemCount++;
	}
	if (rogue.ringRight) {
		itemList[itemNumber] = rogue.ringRight;
		itemNumber++;
		equippedItemCount++;
	}
	// Now all of the non-equipped items.
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (!(theItem->flags & ITEM_EQUIPPED)) {
			itemList[itemNumber] = theItem;
			itemNumber++;
		}
	}
	
	// Initialize the buttons:
	for (i=0; i < max(MAX_PACK_ITEMS, ROWS); i++) {
		buttons[i].y = mapToWindowY(i + (equippedItemCount && i >= equippedItemCount ? 1 : 0));
		buttons[i].buttonColor = black;
		buttons[i].opacity = INTERFACE_OPACITY;
		buttons[i].flags |= B_DRAW;
	}
	// Now prepare the buttons.
	//for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
	for (i=0; i<itemNumber; i++) {
		theItem = itemList[i];
		// Set button parameters for the item:
		buttons[i].flags |= (B_DRAW | B_GRADIENT | B_ENABLED);
		if (!waitForAcknowledge) {
			buttons[i].flags |= B_KEYPRESS_HIGHLIGHT;
		}
		buttons[i].hotkey[0] = theItem->inventoryLetter;
		buttons[i].hotkey[1] = theItem->inventoryLetter + 'A' - 'a';
		
		if ((theItem->category & categoryMask) &&
			!(~(theItem->flags) & requiredFlags) &&
			!(theItem->flags & forbiddenFlags)) {
			
			buttons[i].flags |= (B_HOVER_ENABLED);
		}
		
		// Set the text for the button:
		itemName(theItem, buf, true, true, (buttons[i].flags & B_HOVER_ENABLED) ? &white : &gray);
		upperCase(buf);
		
		if ((theItem->flags & ITEM_MAGIC_DETECTED)
			&& !(theItem->category & AMULET)) { // Won't include food, keys, lumenstones or amulet.
			buttons[i].symbol[0] = (itemMagicChar(theItem) ? itemMagicChar(theItem) : '-');
			if (buttons[i].symbol[0] == '-') {
				magicEscapePtr = yellowColorEscapeSequence;
			} else if (buttons[i].symbol[0] == GOOD_MAGIC_CHAR) {
				magicEscapePtr = goodColorEscapeSequence;
			} else {
				magicEscapePtr = badColorEscapeSequence;
			}
			
			// The first '*' is the magic detection symbol, e.g. '-' for non-magical.
			// The second '*' is the item character, e.g. ':' for food.
			sprintf(buttons[i].text, " %c%s %s* %s* %s%s%s%s",
					theItem->inventoryLetter,
					(theItem->flags & ITEM_PROTECTED ? "}" : ")"),
					magicEscapePtr,
					(buttons[i].flags & B_HOVER_ENABLED) ? yellowColorEscapeSequence : darkYellowColorEscapeSequence,
					(buttons[i].flags & B_HOVER_ENABLED) ? whiteColorEscapeSequence : grayColorEscapeSequence,
					buf,
					grayColorEscapeSequence,
					(theItem->flags & ITEM_EQUIPPED ? ((theItem->category & WEAPON) ? " (in hand) " : " (being worn) ") : ""));
			buttons[i].symbol[1] = theItem->displayChar;
		} else {
			sprintf(buttons[i].text, " %c%s %s%s* %s%s%s%s", // The '*' is the item character, e.g. ':' for food.
					theItem->inventoryLetter,
					(theItem->flags & ITEM_PROTECTED ? "}" : ")"),
					(magicDetected ? "  " : ""), // For proper spacing when this item is not detected but another is.
					(buttons[i].flags & B_HOVER_ENABLED) ? yellowColorEscapeSequence : darkYellowColorEscapeSequence,
					(buttons[i].flags & B_HOVER_ENABLED) ? whiteColorEscapeSequence : grayColorEscapeSequence,
					buf,
					grayColorEscapeSequence,
					(theItem->flags & ITEM_EQUIPPED ? ((theItem->category & WEAPON) ? " (in hand) " : " (being worn) ") : ""));
			buttons[i].symbol[0] = theItem->displayChar;
		}
		
		// Keep track of the maximum width needed:
		maxLength = max(maxLength, strLenWithoutEscapes(buttons[i].text));

//		itemList[itemNumber] = theItem;
//		
//		itemNumber++;
	}
	//printf("\nMaxlength: %i", maxLength);
	itemCount = itemNumber;
	if (!itemNumber) {
		confirmMessages();
		message("Nothing of that type!", false);
		restoreRNG;
		return 0;
	}
	if (waitForAcknowledge) {
		// Add the two extra lines as disabled buttons.
		itemSpaceRemaining = MAX_PACK_ITEMS - numberOfItemsInPack();
		if (itemSpaceRemaining) {
			sprintf(buttons[itemNumber + extraLineCount].text, "%s%s    You have room for %i more item%s.",
					grayColorEscapeSequence,
					(magicDetected ? "  " : ""),
					itemSpaceRemaining,
					(itemSpaceRemaining == 1 ? "" : "s"));
		} else {
			sprintf(buttons[itemNumber + extraLineCount].text, "%s%s    Your pack is full.",
					grayColorEscapeSequence,
					(magicDetected ? "  " : ""));	
		}
		maxLength = max(maxLength, (strLenWithoutEscapes(buttons[itemNumber + extraLineCount].text)));
		extraLineCount++;
		
		sprintf(buttons[itemNumber + extraLineCount].text, "%s%s -- press (a-z) for more info -- ",
				grayColorEscapeSequence,
				(magicDetected ? "  " : ""));
		maxLength = max(maxLength, (strLenWithoutEscapes(buttons[itemNumber + extraLineCount].text)));
		extraLineCount++;
	}
	if (equippedItemCount) {
		// Add a separator button to fill in the blank line between equipped and unequipped items.
		sprintf(buttons[itemNumber + extraLineCount].text, "      %s%s---",
				(magicDetected ? "  " : ""),
				grayColorEscapeSequence);
		buttons[itemNumber + extraLineCount].y = mapToWindowY(equippedItemCount);
		extraLineCount++;
	}
		
	for (i=0; i < itemNumber + extraLineCount; i++) {
		
		// Position the button.
		buttons[i].x = COLS - maxLength;
		
		// Pad the button label with space, so the button reaches to the right edge of the screen.
		m = strlen(buttons[i].text);
		for (j=buttons[i].x + strLenWithoutEscapes(buttons[i].text); j < COLS; j++) {
			buttons[i].text[m] = ' ';
			m++;
		}
		buttons[i].text[m] = '\0';
		
		// Display the button. This would be redundant with the button loop,
		// except that we want the display to stick around until we get rid of it.
		drawButton(&(buttons[i]), BUTTON_NORMAL, dbuf);
	}
	
	// Add invisible previous and next buttons, so up and down arrows can select items.
	// Previous
	buttons[itemNumber + extraLineCount + 0].flags = B_ENABLED; // clear everything else
	buttons[itemNumber + extraLineCount + 0].hotkey[0] = NUMPAD_8;
	buttons[itemNumber + extraLineCount + 0].hotkey[1] = UP_ARROW;
	// Next
	buttons[itemNumber + extraLineCount + 1].flags = B_ENABLED; // clear everything else
	buttons[itemNumber + extraLineCount + 1].hotkey[0] = NUMPAD_2;
	buttons[itemNumber + extraLineCount + 1].hotkey[1] = DOWN_ARROW;
	
	overlayDisplayBuffer(dbuf, rbuf);
	
	do {
		repeatDisplay = false;
		
		// Do the button loop.
		highlightItemLine = -1;
		overlayDisplayBuffer(rbuf, NULL);	// Remove the inventory display while the buttons are active,
											// since they look the same and we don't want their opacities to stack.
		
		highlightItemLine = buttonInputLoop(buttons,
											itemCount + extraLineCount + 2, // the 2 is for up/down hotkeys
											COLS - maxLength,
											mapToWindowY(0),
											maxLength,
											itemNumber + extraLineCount,
											&theEvent);
		if (highlightItemLine == itemNumber + extraLineCount + 0) {
			// Up key
			highlightItemLine = itemNumber - 1;
			theEvent.shiftKey = true;
		} else if (highlightItemLine == itemNumber + extraLineCount + 1) {
			// Down key
			highlightItemLine = 0;
			theEvent.shiftKey = true;
		}
		
		if (highlightItemLine >= 0) {
			theKey = itemList[highlightItemLine]->inventoryLetter;
			theItem = itemList[highlightItemLine];
		} else {
			theKey = ESCAPE_KEY;
		}
		
		// Was an item selected?
		if (highlightItemLine > -1 && (waitForAcknowledge || theEvent.shiftKey || theEvent.controlKey)) {
			
			do {
				// Yes. Highlight the selected item. Do this by changing the button color and re-displaying it.
				
				overlayDisplayBuffer(dbuf, NULL);
				
				//buttons[highlightItemLine].buttonColor = interfaceBoxColor;
				drawButton(&(buttons[highlightItemLine]), BUTTON_PRESSED, NULL);
				//buttons[highlightItemLine].buttonColor = black;
				
				if (theEvent.shiftKey || theEvent.controlKey || waitForAcknowledge) {
					// Display an information window about the item.
					actionKey = printCarriedItemDetails(theItem, max(2, mapToWindowX(DCOLS - maxLength - 42)), mapToWindowY(2), 40, includeButtons, NULL);
					
					overlayDisplayBuffer(rbuf, NULL); // remove the item info window
					
					if (actionKey == -1) {
						repeatDisplay = true;
						overlayDisplayBuffer(dbuf, NULL); // redisplay the inventory
					} else {
						restoreRNG;
						repeatDisplay = false;
						overlayDisplayBuffer(rbuf, NULL); // restore the original screen
					}
					
					switch (actionKey) {
						case APPLY_KEY:
							apply(theItem, true);
							break;
						case EQUIP_KEY:
							equip(theItem);
							break;
						case UNEQUIP_KEY:
							unequip(theItem);
							break;
						case DROP_KEY:
							drop(theItem);
							break;
						case THROW_KEY:
							throwCommand(theItem);
							break;
						case CALL_KEY:
							call(theItem);
							break;
						case UP_KEY:
							highlightItemLine = highlightItemLine - 1;
							if (highlightItemLine < 0) {
								highlightItemLine = itemNumber - 1;
							}
							break;
						case DOWN_KEY:
							highlightItemLine = highlightItemLine + 1;
							if (highlightItemLine >= itemNumber) {
								highlightItemLine = 0;
							}
							break;
						default:
							break;
					}
					
					if (actionKey == UP_KEY || actionKey == DOWN_KEY) {
						theKey = itemList[highlightItemLine]->inventoryLetter;
						theItem = itemList[highlightItemLine];
					} else if (actionKey > -1) {
						// Player took an action directly from the item screen; we're done here.
						restoreRNG;
						return 0;
					}
				}
			} while (actionKey == UP_KEY || actionKey == DOWN_KEY);
		}
	} while (repeatDisplay); // so you can get info on multiple items sequentially
	
	overlayDisplayBuffer(rbuf, NULL); // restore the original screen

	restoreRNG;
	return theKey;
}

short numberOfMatchingPackItems(unsigned short categoryMask,
								unsigned long requiredFlags, unsigned long forbiddenFlags,
								boolean displayErrors) {
	item *theItem;
	short matchingItemCount = 0;
	
	if (packItems->nextItem == NULL) {
		if (displayErrors) {
			confirmMessages();
			message("Your pack is empty!", false);
		}
		return 0;
	}
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		
		if (theItem->category & categoryMask &&
			!(~(theItem->flags) & requiredFlags) &&
			!(theItem->flags & forbiddenFlags)) {
			
			matchingItemCount++;
		}
	}
	
	if (matchingItemCount == 0) {
		if (displayErrors) {
			confirmMessages();
			message("You have nothing suitable.", false);
		}
		return 0;
	}
	
	return matchingItemCount;
}

void updateEncumbrance() {
	short moveSpeed, attackSpeed;
	
	moveSpeed = player.info.movementSpeed;
	attackSpeed = player.info.attackSpeed;
	
	/*if (rogue.weapon && rogue.weapon->strengthRequired > rogue.strength - player.weaknessAmount) {
	 attackSpeed += 25 * (rogue.weapon->strengthRequired - (rogue.strength - player.weaknessAmount));
	 }
	 
	 if (rogue.armor && rogue.armor->strengthRequired > rogue.strength - player.weaknessAmount) {
	 moveSpeed += 25 * (rogue.armor->strengthRequired - (rogue.strength - player.weaknessAmount));
	 }*/
	
	if (player.status[STATUS_HASTED]) {
		moveSpeed /= 2;
		attackSpeed /= 2;
	} else if (player.status[STATUS_SLOWED]) {
		moveSpeed *= 2;
		attackSpeed *= 2;
	}
	
	player.movementSpeed = moveSpeed;
	player.attackSpeed = attackSpeed;
	
	recalculateEquipmentBonuses();
}

void strengthCheck(item *theItem) {
	char buf1[COLS], buf2[COLS*2];
	short strengthDeficiency;
	
	updateEncumbrance();
	if (theItem) {
		if (theItem->category & WEAPON && theItem->strengthRequired > rogue.strength - player.weaknessAmount) {
			strengthDeficiency = theItem->strengthRequired - (rogue.strength - player.weaknessAmount);
			strcpy(buf1, "");
			itemName(theItem, buf1, false, false, NULL);
			sprintf(buf2, "You can barely lift the %s; %i more strength would be ideal.", buf1, strengthDeficiency);
			message(buf2, false);
		}
		
		if (theItem->category & ARMOR && theItem->strengthRequired > rogue.strength - player.weaknessAmount) {
			strengthDeficiency = theItem->strengthRequired - (rogue.strength - player.weaknessAmount);
			strcpy(buf1, "");
			itemName(theItem, buf1, false, false, NULL);
			sprintf(buf2, "You stagger under the weight of the %s; %i more strength would be ideal.",
					buf1, strengthDeficiency);
			message(buf2, false);
		}
	}
}

boolean canEquip(item *theItem) {
	item *previouslyEquippedItem = NULL;
	
	if (theItem->category & WEAPON) {
		previouslyEquippedItem = rogue.weapon;
	} else if (theItem->category & ARMOR) {
		previouslyEquippedItem = rogue.armor;
	}
	if (previouslyEquippedItem && (previouslyEquippedItem->flags & ITEM_CURSED)) {
		return false; // already using a cursed item
	}
	
	if ((theItem->category & RING) && rogue.ringLeft && rogue.ringRight) {
		return false;
	}
	return true;
}

// Will prompt for an item if none is given.
// Equips the item and records input if successful.
// Player's failure to select an item will result in failure.
// Failure does not record input.
void equip(item *theItem) {
	char buf1[COLS], buf2[COLS];
	unsigned char command[10];
	short c = 0;
	item *theItem2;
	
	command[c++] = EQUIP_KEY;
	if (!theItem) {
		theItem = promptForItemOfType((WEAPON|ARMOR|RING), 0, ITEM_EQUIPPED, "Equip what? (a-z, shift for more info; or <esc> to cancel)", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	command[c++] = theItem->inventoryLetter;
	
	if (theItem->category & (WEAPON|ARMOR|RING)) {
		
		if (theItem->category & RING) {
			if (theItem->flags & ITEM_EQUIPPED) {
				confirmMessages();
				message("you are already wearing that ring.", false);
				return;
			} else if (rogue.ringLeft && rogue.ringRight) {
				confirmMessages();
				theItem2 = promptForItemOfType((RING), ITEM_EQUIPPED, 0,
											   "You are already wearing two rings; remove which first?", true);
				if (!theItem2 || theItem2->category != RING || !(theItem2->flags & ITEM_EQUIPPED)) {
					if (theItem2) { // No message if canceled or did an inventory action instead.
						message("Invalid entry.", false);
					}
					return;
				} else {
					if (theItem2->flags & ITEM_CURSED) {
						itemName(theItem2, buf1, false, false, NULL);
						sprintf(buf2, "You can't remove your %s: it appears to be cursed.", buf1);
						confirmMessages();
						messageWithColor(buf2, &itemMessageColor, false);
						return;
					}
					unequipItem(theItem2, false);
					command[c++] = theItem2->inventoryLetter;
				}
			}
		}
		
		if (theItem->flags & ITEM_EQUIPPED) {
			confirmMessages();
			message("already equipped.", false);
			return;
		}
		
		if (!canEquip(theItem)) {
			// equip failed because current item is cursed
			if (theItem->category & WEAPON) {
				itemName(rogue.weapon, buf1, false, false, NULL);
			} else if (theItem->category & ARMOR) {
				itemName(rogue.armor, buf1, false, false, NULL);
			} else {
				sprintf(buf1, "one");
			}
			sprintf(buf2, "You can't; the %s you are using appears to be cursed.", buf1);
			confirmMessages();
			messageWithColor(buf2, &itemMessageColor, false);
			return;
		}
		command[c] = '\0';
		recordKeystrokeSequence(command);
		
		equipItem(theItem, false);
		
		itemName(theItem, buf2, true, true, NULL);
		sprintf(buf1, "Now %s %s.", (theItem->category & WEAPON ? "wielding" : "wearing"), buf2);
		confirmMessages();
		messageWithColor(buf1, &itemMessageColor, false);
		
		strengthCheck(theItem);
		
		if (theItem->flags & ITEM_CURSED) {
			itemName(theItem, buf2, false, false, NULL);
			switch(theItem->category) {
				case WEAPON:
					sprintf(buf1, "you wince as your grip involuntarily tightens around your %s.", buf2);
					break;
				case ARMOR:
					sprintf(buf1, "your %s constricts around you painfully.", buf2);
					break;
				case RING:
					sprintf(buf1, "your %s tightens around your finger painfully.", buf2);
					break;
				default:
					sprintf(buf1, "your %s seizes you with a malevolent force.", buf2);
					break;
			}
			messageWithColor(buf1, &itemMessageColor, false);
		}
		playerTurnEnded();
	} else {
		confirmMessages();
		message("You can't equip that.", false);
	}
}

// Returns whether the given item is a key that can unlock the given location.
// An item qualifies if:
// (1) it's a key (has ITEM_IS_KEY flag),
// (2) its keyZ matches the depth, and
// (3) either its key (x, y) location matches (x, y), or its machine number matches the machine number at (x, y).
boolean keyMatchesLocation(item *theItem, short x, short y) {
	short i;
	
	if ((theItem->flags & ITEM_IS_KEY)
		&& theItem->keyZ == rogue.depthLevel) {
		
		for (i=0; i < KEY_ID_MAXIMUM && (theItem->keyLoc[i].x || theItem->keyLoc[i].machine); i++) {
			if (theItem->keyLoc[i].x == x && theItem->keyLoc[i].y == y) {
				return true;
			} else if (theItem->keyLoc[i].machine == pmap[x][y].machineNumber) {
				return true;
			}
		}
	}
	return false;
}

item *keyInPackFor(short x, short y) {
	item *theItem;
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (keyMatchesLocation(theItem, x, y)) {
			return theItem;
		}
	}
	return NULL;
}

item *keyOnTileAt(short x, short y) {
	item *theItem;
	creature *monst;
	
	if ((pmap[x][y].flags & HAS_PLAYER)
		&& player.xLoc == x
		&& player.yLoc == y
		&& keyInPackFor(x, y)) {
		
		return keyInPackFor(x, y);
	}
	if (pmap[x][y].flags & HAS_ITEM) {
		theItem = itemAtLoc(x, y);
		if (keyMatchesLocation(theItem, x, y)) {
			return theItem;
		}
	}
	if (pmap[x][y].flags & HAS_MONSTER) {
		monst = monsterAtLoc(x, y);
		if (monst->carriedItem) {
			theItem = monst->carriedItem;
			if (keyMatchesLocation(theItem, x, y)) {
				return theItem;
			}
		}
	}
	return NULL;
}

void aggravateMonsters() {
	creature *monst;
	short i, j, **grid;
	for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->creatureState == MONSTER_SLEEPING) {
			wakeUp(monst);
		}
		if (monst->creatureState != MONSTER_ALLY && monst->leader != &player) {
			monst->creatureState = MONSTER_TRACKING_SCENT;
		}
	}
	
	grid = allocDynamicGrid();
	fillDynamicGrid(grid, 0);
	
	calculateDistances(grid, player.xLoc, player.yLoc, T_PATHING_BLOCKER, NULL, true, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j] >= 0 && grid[i][j] < 30000) {
				scentMap[i][j] = 0;
				addScentToCell(i, j, 2 * grid[i][j]);
			}
		}
	}
	
	freeDynamicGrid(grid);
}

// returns the number of items on the list; includes (-1, -1) as an additional terminus indicator after the end of the list
short getLineCoordinates(short listOfCoordinates[][2], short originLoc[2], short targetLoc[2]) {
	float targetVector[2], error[2];
	short largerTargetComponent, currentVector[2], previousVector[2], quadrantTransform[2], i;
	short currentLoc[2], previousLoc[2];
	short cellNumber = 0;
	
//#ifdef BROGUE_ASSERTS
//	assert(originLoc[0] != targetLoc[0] || originLoc[1] != targetLoc[1]);
//#else
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		return 0;
	}
//#endif
	
	// Neither vector is negative. We keep track of negatives with quadrantTransform.
	for (i=0; i<= 1; i++) {
		targetVector[i] = targetLoc[i] - originLoc[i];
		if (targetVector[i] < 0) {
			targetVector[i] *= -1;
			quadrantTransform[i] = -1;
		} else {
			quadrantTransform[i] = 1;
		}
		currentVector[i] = previousVector[i] = error[i] = 0;
		currentLoc[i] = originLoc[i];
	}
	
	// normalize target vector such that one dimension equals 1 and the other is in [0, 1].
	largerTargetComponent = max(targetVector[0], targetVector[1]);
	targetVector[0] /= largerTargetComponent;
	targetVector[1] /= largerTargetComponent;
	
	do {
		for (i=0; i<= 1; i++) {
			
			previousLoc[i] = currentLoc[i];
			
			currentVector[i] += targetVector[i];
			error[i] += (targetVector[i] == 1 ? 0 : targetVector[i]);
			
			if (error[i] >= 0.5) {
				currentVector[i]++;
				error[i] -= 1;
			}
			
			currentLoc[i] = quadrantTransform[i]*currentVector[i] + originLoc[i];
			
			listOfCoordinates[cellNumber][i] = currentLoc[i];
		}
		
		//DEBUG printf("\ncell %i: (%i, %i)", cellNumber, listOfCoordinates[cellNumber][0], listOfCoordinates[cellNumber][1]);
		cellNumber++;
		
	} while (coordinatesAreInMap(currentLoc[0], currentLoc[1]));
	
	cellNumber--;
	
	listOfCoordinates[cellNumber][0] = listOfCoordinates[cellNumber][1] = -1; // demarcates the end of the list
	return cellNumber;
}

void getImpactLoc(short returnLoc[2], short originLoc[2], short targetLoc[2],
				  short maxDistance, boolean returnLastEmptySpace) {
	float targetVector[2], error[2];
	short largerTargetComponent, currentVector[2], previousVector[2], quadrantTransform[2], i;
	short currentLoc[2], previousLoc[2];
	creature *monst;
	
	monst = NULL;
	
	// Neither vector is negative. We keep track of negatives with quadrantTransform.
	for (i=0; i<= 1; i++) {
		targetVector[i] = targetLoc[i] - originLoc[i];
		if (targetVector[i] < 0) {
			targetVector[i] *= -1;
			quadrantTransform[i] = -1;
		} else {
			quadrantTransform[i] = 1;
		}
		currentVector[i] = previousVector[i] = error[i] = 0;
		currentLoc[i] = originLoc[i];
	}
	
	// normalize target vector such that one dimension equals 1 and the other is in [0, 1].
	largerTargetComponent = max(targetVector[0], targetVector[1]);
	targetVector[0] /= largerTargetComponent;
	targetVector[1] /= largerTargetComponent;
	
	do {
		for (i=0; i<= 1; i++) {
			
			previousLoc[i] = currentLoc[i];
			
			currentVector[i] += targetVector[i];
			error[i] += (targetVector[i] == 1 ? 0 : targetVector[i]);
			
			if (error[i] >= 0.5) {
				currentVector[i]++;
				error[i] -= 1;
			}
			
			currentLoc[i] = quadrantTransform[i]*currentVector[i] + originLoc[i];
		}
		
		if (!coordinatesAreInMap(currentLoc[0], currentLoc[1])) {
			break;
		}
		
		if (pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER) {
			monst = monsterAtLoc(currentLoc[0], currentLoc[1]);
		}
		
	} while ((!(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER)
			  || !monst
			  || (monst->status[STATUS_INVISIBLE] || (monst->bookkeepingFlags & MONST_SUBMERGED)))
			 && !(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_PLAYER)
			 && !cellHasTerrainFlag(currentLoc[0], currentLoc[1], (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
			 && max(currentVector[0], currentVector[1]) <= maxDistance);
	
	if (returnLastEmptySpace) {
		returnLoc[0] = previousLoc[0];
		returnLoc[1] = previousLoc[1];
	} else {
		returnLoc[0] = currentLoc[0];
		returnLoc[1] = currentLoc[1];
	}
}

boolean tunnelize(short x, short y) {
	enum dungeonLayers layer;
	boolean didSomething = false;
	creature *monst;
	
	if (pmap[x][y].flags & IMPREGNABLE) {
		return false;
	}
	
	freeCaptivesEmbeddedAt(x, y);
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION)) {
			pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
			didSomething = true;
		}
	}
	if (didSomething) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_TUNNELIZE], true, false);
		if (pmap[x][y].flags & HAS_MONSTER) {
			// Kill turrets and sentinels if you tunnelize them.
			monst = monsterAtLoc(x, y);
			if (monst->info.flags & MONST_ATTACKABLE_THRU_WALLS) {
				inflictDamage(&player, monst, monst->currentHP, NULL);
			}
		}
	}
	return didSomething;
}

void negate(creature *monst) {
	if (monst->info.flags & MONST_DIES_IF_NEGATED) {
		char buf[DCOLS * 3], monstName[DCOLS];
		monsterName(monstName, monst, true);
		if ((monst->info.flags & MONST_INANIMATE) && (monst->status[STATUS_LEVITATING])) {
			sprintf(buf, "%s dissipates into thin air", monstName);
		} else {
			sprintf(buf, "%s falls to the ground, lifeless", monstName);
		}
		killCreature(monst, false);
		combatMessage(buf, messageColorFromVictim(monst));
	} else {
		// works on inanimates
		monst->info.abilityFlags = 0; // negated monsters lose all special abilities
		monst->status[STATUS_IMMUNE_TO_FIRE] = 0;
		monst->status[STATUS_SLOWED] = 0;
		monst->status[STATUS_HASTED] = 0;
		monst->status[STATUS_ENTRANCED] = 0;
		monst->status[STATUS_DISCORDANT] = 0;
		monst->status[STATUS_SHIELDED] = 0;
		monst->status[STATUS_INVISIBLE] = 0;
		if (monst == &player) {
			monst->status[STATUS_TELEPATHIC] = min(monst->status[STATUS_TELEPATHIC], 1);
			monst->status[STATUS_MAGICAL_FEAR] = min(monst->status[STATUS_MAGICAL_FEAR], 1);
			monst->status[STATUS_LEVITATING] = min(monst->status[STATUS_LEVITATING], 1);
			if (monst->status[STATUS_DARKNESS]) {
				monst->status[STATUS_DARKNESS] = 0;
				updateMinersLightRadius();
				updateVision(true);
			}
		} else {
			monst->status[STATUS_TELEPATHIC] = 0;
			monst->status[STATUS_MAGICAL_FEAR] = 0;
			monst->status[STATUS_LEVITATING] = 0;
		}
		monst->info.flags &= ~MONST_IMMUNE_TO_FIRE;
		monst->movementSpeed = monst->info.movementSpeed;
		monst->attackSpeed = monst->info.attackSpeed;
		if (monst != &player && monst->info.flags & NEGATABLE_TRAITS) {
			if ((monst->info.flags & MONST_FIERY) && monst->status[STATUS_BURNING]) {
				extinguishFireOnCreature(monst);
			}
			monst->info.flags &= ~NEGATABLE_TRAITS;
			refreshDungeonCell(monst->xLoc, monst->yLoc);
			refreshSideBar(-1, -1, false);
		}
		applyInstantTileEffectsToCreature(monst); // in case it should immediately die or fall into a chasm
	}
}

void refreshMonsterWeaknessPenalties(creature *monst) {
	// Assume that the monster can ordinarily barely carry its weapon and armor.
	monst->info.accuracy = monsterCatalog[monst->info.monsterID].accuracy * pow(WEAPON_ENCHANT_FACTOR, -2.5 * (float) monst->weaknessAmount);
	monst->info.damage.lowerBound = monsterCatalog[monst->info.monsterID].damage.lowerBound * pow(WEAPON_ENCHANT_FACTOR, -2.5 * (float) monst->weaknessAmount);
	monst->info.damage.upperBound = monsterCatalog[monst->info.monsterID].damage.upperBound * pow(WEAPON_ENCHANT_FACTOR, -2.5 * (float) monst->weaknessAmount);
	monst->info.defense = monsterCatalog[monst->info.monsterID].defense - 25 * monst->weaknessAmount;	
}

// Adds one to the creature's weakness, sets the weakness status duration to maxDuration.
void weaken(creature *monst, short maxDuration) {
	monst->weaknessAmount++;
	monst->status[STATUS_WEAKENED] = max(monst->status[STATUS_WEAKENED], maxDuration);
	monst->maxStatus[STATUS_WEAKENED] = max(monst->maxStatus[STATUS_WEAKENED], maxDuration);
	if (monst == &player) {
        messageWithColor("your muscles weaken as an enervating toxin fills your veins.", &badMessageColor, false);
		strengthCheck(rogue.weapon);
		strengthCheck(rogue.armor);
	} else {
		refreshMonsterWeaknessPenalties(monst);
	}
}

// True if the creature polymorphed; false if not.
boolean polymorph(creature *monst) {
	short previousExperience, previousDamageTaken;
	float healthFraction;
	
	if (monst == &player || (monst->info.flags & MONST_INANIMATE)) {
		return false; // Sorry, this is not Nethack.
	}
	
	unAlly(monst); // Sorry, no cheap dragon allies.
	healthFraction = monst->currentHP / monst->info.maxHP;
	previousDamageTaken = monst->info.maxHP - monst->currentHP;
	previousExperience = monst->info.expForKilling;
	
	do {
		monst->info = monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)]; // abra kadabra
	} while (monst->info.flags & MONST_INANIMATE); // Can't turn something into an inanimate object.
	
	monst->currentHP = max(1, max(healthFraction * monst->info.maxHP, monst->info.maxHP - previousDamageTaken));
	monst->info.expForKilling = healthFraction * monst->info.expForKilling + (1 - healthFraction) * previousExperience;
	
	monst->movementSpeed = monst->info.movementSpeed;
	monst->attackSpeed = monst->info.attackSpeed;
	if (monst->status[STATUS_HASTED]) {
		monst->movementSpeed /= 2;
		monst->attackSpeed /= 2;
	}
	if (monst->status[STATUS_SLOWED]) {
		monst->movementSpeed *= 2;
		monst->attackSpeed *= 2;
	}
	
	clearStatus(monst);
	
	if (monst->info.flags & MONST_FIERY) {
		monst->status[STATUS_BURNING] = monst->maxStatus[STATUS_BURNING] = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_FLIES) {
		monst->status[STATUS_LEVITATING] = monst->maxStatus[STATUS_LEVITATING] = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_IMMUNE_TO_FIRE) {
		monst->status[STATUS_IMMUNE_TO_FIRE] = monst->maxStatus[STATUS_IMMUNE_TO_FIRE] = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_INVISIBLE) {
		monst->status[STATUS_INVISIBLE] = monst->maxStatus[STATUS_INVISIBLE] = 1000; // won't decrease
	}
	monst->status[STATUS_NUTRITION] = monst->maxStatus[STATUS_NUTRITION] = 1000;
	
	if (monst->bookkeepingFlags & MONST_CAPTIVE) {
		demoteMonsterFromLeadership(monst);
		monst->creatureState = MONSTER_TRACKING_SCENT;
		monst->bookkeepingFlags &= ~MONST_CAPTIVE;
	}
	
	monst->ticksUntilTurn = max(monst->ticksUntilTurn, 101);
	
	refreshDungeonCell(monst->xLoc, monst->yLoc);
	flashMonster(monst, boltColors[BOLT_POLYMORPH], 100);
	return true;
}

void slow(creature *monst, short turns) {
	if (!(monst->info.flags & MONST_INANIMATE)) {
		monst->status[STATUS_SLOWED] = monst->maxStatus[STATUS_SLOWED] = turns;
		monst->status[STATUS_HASTED] = 0;
		if (monst == &player) {
			updateEncumbrance();
			message("you feel yourself slow down.", false);
		} else {
			monst->movementSpeed = monst->info.movementSpeed * 2;
			monst->attackSpeed = monst->info.attackSpeed * 2;
		}
	}
}

void haste(creature *monst, short turns) {
	if (monst && !(monst->info.flags & MONST_INANIMATE)) {
		monst->status[STATUS_SLOWED] = 0;
		monst->status[STATUS_HASTED] = monst->maxStatus[STATUS_HASTED] = turns;
		if (monst == &player) {
			updateEncumbrance();
			message("you feel yourself speed up.", false);
		} else {
			monst->movementSpeed = monst->info.movementSpeed / 2;
			monst->attackSpeed = monst->info.attackSpeed / 2;
		}
	}
}

void heal(creature *monst, short percent) {	
	char buf[COLS], monstName[COLS];
	monst->currentHP = min(monst->info.maxHP, monst->currentHP + percent * monst->info.maxHP / 100);
	if (canDirectlySeeMonster(monst) && monst != &player) {
		monsterName(monstName, monst, true);
		sprintf(buf, "%s looks healthier", monstName);
		combatMessage(buf, NULL);
	}
}

boolean projectileReflects(creature *attacker, creature *defender) {	
	short prob, netReflectionLevel;
	
	// immunity armor always reflects its vorpal enemy's projectiles
	if (defender == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_IMMUNITY
		&& rogue.armor->vorpalEnemy == attacker->info.monsterID) {
		return true;
	}
	
	if (defender == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_REFLECTION) {
		netReflectionLevel = (short) netEnchant(rogue.armor);
	} else {
		netReflectionLevel = 0;
	}
	
	if (defender && (defender->info.flags & MONST_REFLECT_4)) {
        if (defender->info.flags & MONST_ALWAYS_USE_ABILITY) {
            return true;
        }
		netReflectionLevel += 4;
	}
	
	if (netReflectionLevel <= 0) {
		return false;
	}
	
	prob = reflectionChance(netReflectionLevel);
	
	return rand_percent(prob);
}

// returns the path length of the reflected path, alters listOfCoordinates to describe reflected path
short reflectBolt(short targetX, short targetY, short listOfCoordinates[][2], short kinkCell, boolean retracePath) {
	short k, target[2], origin[2], newPath[DCOLS][2], newPathLength, failsafe, finalLength;
	boolean needRandomTarget;
	
	needRandomTarget = (targetX < 0 || targetY < 0
						|| (targetX == listOfCoordinates[kinkCell][0] && targetY == listOfCoordinates[kinkCell][1]));
	
	if (retracePath) {
		// if reflecting back at caster, follow precise trajectory until we reach the caster
		for (k = 1; k <= kinkCell && kinkCell + k < MAX_BOLT_LENGTH; k++) {
			listOfCoordinates[kinkCell + k][0] = listOfCoordinates[kinkCell - k][0];
			listOfCoordinates[kinkCell + k][1] = listOfCoordinates[kinkCell - k][1];
		}
		
		// Calculate a new "extension" path, with an origin at the caster, and a destination at
		// the caster's location translated by the vector from the reflection point to the caster.
		// 
		// For example, if the player is at (0,0), and the caster is at (2,3), then the newpath
		// is from (2,3) to (4,6):
		// (2,3) + ((2,3) - (0,0)) = (4,6).
		
		origin[0] = listOfCoordinates[2 * kinkCell][0];
		origin[1] = listOfCoordinates[2 * kinkCell][1];
		target[0] = targetX + (targetX - listOfCoordinates[kinkCell][0]);
		target[1] = targetY + (targetY - listOfCoordinates[kinkCell][1]);
		newPathLength = getLineCoordinates(newPath, origin, target);
		
		for (k=0; k<=newPathLength; k++) {
			listOfCoordinates[2 * kinkCell + k + 1][0] = newPath[k][0];
			listOfCoordinates[2 * kinkCell + k + 1][1] = newPath[k][1];
		}
		finalLength = 2 * kinkCell + newPathLength + 1;
	} else {
		failsafe = 50;
		do {
			if (needRandomTarget) {
				// pick random target
				perimeterCoords(target, rand_range(0, 39));
				target[0] += listOfCoordinates[kinkCell][0];
				target[1] += listOfCoordinates[kinkCell][1];
			} else {
				target[0] = targetX;
				target[1] = targetY;
			}
			
			newPathLength = getLineCoordinates(newPath, listOfCoordinates[kinkCell], target);
			
			if (!cellHasTerrainFlag(newPath[0][0], newPath[0][1], (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))) {
				needRandomTarget = false;
			}
			
		} while (needRandomTarget && --failsafe);
		
		for (k = 0; k < newPathLength; k++) {
			listOfCoordinates[kinkCell + k + 1][0] = newPath[k][0];
			listOfCoordinates[kinkCell + k + 1][1] = newPath[k][1];
		}
		
		finalLength = kinkCell + newPathLength + 1;
	}
	
	listOfCoordinates[finalLength][0] = -1;
	listOfCoordinates[finalLength][1] = -1;
	return finalLength;
}

// Update stuff that promotes without keys so players can't abuse item libraries with blinking/haste shenanigans
void checkForMissingKeys(short x, short y) {
	short layer;

	if (cellHasTerrainFlag(x, y, T_PROMOTES_WITHOUT_KEY) && !keyOnTileAt(x, y)) {
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & T_PROMOTES_WITHOUT_KEY) {
				promoteTile(x, y, layer, false);
			}
		}
	}
}

void backUpLighting(short lights[DCOLS][DROWS][3]) {
	short i, j, k;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			for (k=0; k<3; k++) {
				lights[i][j][k] = tmap[i][j].light[k];
			}
		}
	}
}

void restoreLighting(short lights[DCOLS][DROWS][3]) {
	short i, j, k;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			for (k=0; k<3; k++) {
				tmap[i][j].light[k] = lights[i][j][k];
			}
		}
	}
}

// returns whether the bolt effect should autoID any staff or wand it came from, if it came from a staff or wand
boolean zap(short originLoc[2], short targetLoc[2], enum boltType bolt, short boltLevel, boolean hideDetails) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2];
	short i, j, k, x, y, x2, y2, numCells, blinkDistance, boltLength, initialBoltLength, newLoc[2], lights[DCOLS][DROWS][3];
	short poisonDamage;
	creature *monst = NULL, *shootingMonst, *newMonst;
	char buf[COLS], monstName[COLS];
	boolean autoID = false;
	boolean fastForward = false;
	boolean beckonedMonster = false;
	boolean alreadyReflected = false;
	boolean boltInView;
	color *boltColor;
    //color boltImpactColor;
	dungeonFeature feat;
	
#ifdef BROGUE_ASSERTS
	assert(originLoc[0] != targetLoc[0] || originLoc[1] != targetLoc[1]);
#else
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		return false;
	}
#endif
	
	x = originLoc[0];
	y = originLoc[1];
	
	initialBoltLength = boltLength = 5 * boltLevel;
    
	lightSource boltLights[initialBoltLength];
	color boltLightColors[initialBoltLength];
    
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	shootingMonst = monsterAtLoc(originLoc[0], originLoc[1]);
	
	if (!hideDetails) {
		boltColor = boltColors[bolt];
	} else {
		boltColor = &gray;
	}
    
    //boltImpactColor = *boltColor;
    //applyColorScalar(&boltImpactColor, 5000);
	
	refreshSideBar(-1, -1, false);
    displayCombatText(); // To announce who fired the bolt while the animation plays.
	
	if (bolt == BOLT_BLINKING) {
		if (cellHasTerrainFlag(listOfCoordinates[0][0], listOfCoordinates[0][1], (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION))
			|| (pmap[listOfCoordinates[0][0]][listOfCoordinates[0][1]].flags & (HAS_PLAYER | HAS_MONSTER)
				&& !(monsterAtLoc(listOfCoordinates[0][0], listOfCoordinates[0][1])->bookkeepingFlags & MONST_SUBMERGED))) {
				// shooting blink point-blank into an obstruction does nothing.
				return false;
			}
		pmap[originLoc[0]][originLoc[1]].flags &= ~(HAS_PLAYER | HAS_MONSTER);
		refreshDungeonCell(originLoc[0], originLoc[1]);
		blinkDistance = boltLevel * 2 + 1;
		checkForMissingKeys(originLoc[0], originLoc[1]);
	}
	
	for (i=0; i<initialBoltLength; i++) {
		boltLightColors[i] = *boltColor;
		boltLights[i] = lightCatalog[BOLT_LIGHT_SOURCE];
		boltLights[i].lightColor = &boltLightColors[i];
		boltLights[i].lightRadius.lowerBound = boltLights[i].lightRadius.upperBound = 50 * (3 + boltLevel * 1.33) * (initialBoltLength - i) / initialBoltLength;
	}
	
	if (bolt == BOLT_TUNNELING) {
		tunnelize(originLoc[0], originLoc[1]);
	}
	
	backUpLighting(lights);
	boltInView = true;
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		monst = monsterAtLoc(x, y);
		
		// Player travels inside the bolt when it is blinking.
		if (bolt == BOLT_BLINKING && shootingMonst == &player) {
			player.xLoc = x;
			player.yLoc = y;
			updateVision(true);
			backUpLighting(lights);
		}
		
		// Firebolts light things on fire, and the effect is updated in realtime.
		if (!monst && bolt == BOLT_FIRE) {
			if (exposeTileToFire(x, y, true)) {
				updateVision(true);
				backUpLighting(lights);
				autoID = true;
			}
		}
		
		// Update the visual effect of the bolt. This lighting effect is expensive; do it only if the player can see the bolt.
		if (boltInView) {
			demoteVisibility();
			restoreLighting(lights);
			for (k = min(i, boltLength + 2); k >= 0; k--) {
				if (k < initialBoltLength) {
					paintLight(&boltLights[k], listOfCoordinates[i-k][0], listOfCoordinates[i-k][1], false, false);
				}
			}
		}
		boltInView = false;
		updateFieldOfViewDisplay(false, true);
		for (k = min(i, boltLength + 2); k >= 0; k--) {
			if (rogue.playbackOmniscience || (pmap[listOfCoordinates[i-k][0]][listOfCoordinates[i-k][1]].flags & ANY_KIND_OF_VISIBLE)) {
				hiliteCell(listOfCoordinates[i-k][0], listOfCoordinates[i-k][1], boltColor, max(0, 100 - k * 100 / (boltLength)), false);
			}
            if (pmap[listOfCoordinates[i-k][0]][listOfCoordinates[i-k][1]].flags & IN_FIELD_OF_VIEW) {
                boltInView = true;
            }
		}
		if (!fastForward && (boltInView || rogue.playbackOmniscience)) {
			fastForward = rogue.playbackFastForward || pauseBrogue(10);
		}
		
		// Handle bolt reflection off of creatures (reflection off of terrain is handled further down).
		if (monst && projectileReflects(shootingMonst, monst) && i < DCOLS*2) {
			if (projectileReflects(shootingMonst, monst)) { // if it scores another reflection roll, reflect at caster
				numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, !alreadyReflected);
			} else {
				numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
			}
			
			alreadyReflected = true;
			
			if (boltInView) {
				monsterName(monstName, monst, true);
				sprintf(buf, "%s deflect%s the bolt", monstName, (monst == &player ? "" : "s"));
				combatMessage(buf, 0);
				
				if (monst == &player
					&& rogue.armor
					&& rogue.armor->enchant2 == A_REFLECTION
					&& !(rogue.armor->flags & ITEM_RUNIC_IDENTIFIED)) {
					
					rogue.armor->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
				}
			}
			continue;
		}
		
		if (bolt == BOLT_BLINKING) {
			boltLevel = (blinkDistance - i) / 2 + 1;
			boltLength = boltLevel * 5;
			for (j=0; j<i; j++) {
				refreshDungeonCell(listOfCoordinates[j][0], listOfCoordinates[j][1]);
			}
			if (i >= blinkDistance) {
				break;
			}
		}
		
		// Some bolts halt at the square before they hit something.
		if ((bolt == BOLT_BLINKING || bolt == BOLT_OBSTRUCTION)
			&& i + 1 < numCells
			&& (cellHasTerrainFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1],
								   (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
				|| (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & (HAS_PLAYER | HAS_MONSTER)))) {
				if (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & HAS_MONSTER) {
					monst = monsterAtLoc(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1]);
					if (!(monst->bookkeepingFlags & MONST_SUBMERGED)) {
						break;
					}
				} else {
					break;
				}
			}
		
		// Lightning hits monsters as it travels.
		if (monst && (pmap[x][y].flags & (HAS_PLAYER | HAS_MONSTER)) && (bolt == BOLT_LIGHTNING) && (!monst || !(monst->bookkeepingFlags & MONST_SUBMERGED))) {
			monsterName(monstName, monst, true);
			
			autoID = true;
			
			if (inflictDamage(shootingMonst, monst, staffDamage(boltLevel), &lightningColor)) {
				// killed monster
				if (player.currentHP <= 0) {
					if (shootingMonst == &player) {
						gameOver("Killed by a reflected lightning bolt", true);
					}
					return false;
				}
				if (boltInView) {
					sprintf(buf, "%s lightning bolt %s %s",
							canSeeMonster(shootingMonst) ? "the" : "a",
							((monst->info.flags & MONST_INANIMATE) ? "destroys" : "kills"),
							monstName);
					combatMessage(buf, messageColorFromVictim(monst));
				} else {
					sprintf(buf, "you hear %s %s", monstName, ((monst->info.flags & MONST_INANIMATE) ? "be destroyed" : "die"));
					combatMessage(buf, messageColorFromVictim(monst));
				}
			} else {
				// monster lives
				if (monst->creatureMode != MODE_PERM_FLEEING
					&& monst->creatureState != MONSTER_ALLY
					&& (monst->creatureState != MONSTER_FLEEING || monst->status[STATUS_MAGICAL_FEAR])) {
					monst->creatureState = MONSTER_TRACKING_SCENT;
					monst->status[STATUS_MAGICAL_FEAR] = 0;
				}
				if (boltInView) {
					sprintf(buf, "%s lightning bolt hits %s",
							canSeeMonster(shootingMonst) ? "the" : "a",
							monstName);
					combatMessage(buf, messageColorFromVictim(monst));
				}
				
				moralAttack(shootingMonst, monst);
			}
		}
		
        // Stop when we hit something -- a wall or a non-submerged creature.
        // However, lightning and tunneling don't stop when they hit creatures, and tunneling continues through walls too.
		if (cellHasTerrainFlag(x, y, (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION))
			|| (((pmap[x][y].flags & HAS_PLAYER) || ((pmap[x][y].flags & HAS_MONSTER) && monst && !(monst->bookkeepingFlags & MONST_SUBMERGED))) && bolt != BOLT_LIGHTNING)) {
			
			if (bolt == BOLT_TUNNELING && x > 0 && y > 0 && x < DCOLS - 1 && y < DROWS - 1) { // don't tunnelize the outermost walls
				tunnelize(x, y);
				if (i > 0 && x != listOfCoordinates[i-1][0] && y != listOfCoordinates[i-1][1]) {
					if (rand_percent(50)) {
						tunnelize(listOfCoordinates[i-1][0], y);
					} else {
						tunnelize(x, listOfCoordinates[i-1][1]);
					}
				} else if (i == 0 && x > 0 && y > 0 && x < DCOLS - 1 && y < DROWS - 1) {
					if (rand_percent(50)) {
						tunnelize(originLoc[0], y);
					} else {
						tunnelize(x, originLoc[1]);
					}
				}
				updateVision(true);
				backUpLighting(lights);
				autoID = true;
				boltLength = --boltLevel * 5;
				for (j=0; j<i; j++) {
					refreshDungeonCell(listOfCoordinates[j][0], listOfCoordinates[j][1]);
				}
				if (!boltLevel) {
					refreshDungeonCell(listOfCoordinates[i-1][0], listOfCoordinates[i-1][1]);
					refreshDungeonCell(x, y);
					break;
				}
			} else {
				break;
			}
		}
		
		// does the bolt bounce off the wall?
		// Can happen with a cursed deflection ring, or when shooting a tunneling bolt into an impregnable wall.
		if (i + 1 < numCells
			&& cellHasTerrainFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1],
								  (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
			&& (projectileReflects(shootingMonst, NULL)
				|| (bolt == BOLT_TUNNELING && (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & IMPREGNABLE)))
			&& i < DCOLS*2) {
			
			sprintf(buf, "the bolt reflects off of %s", tileText(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1]));
			
			if (projectileReflects(shootingMonst, NULL)) { // if it scores another reflection roll, reflect at caster
				numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, !alreadyReflected);
			} else {
				numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
			}
			
			alreadyReflected = true;
			
			if (boltInView) {
				combatMessage(buf, 0);
			}
			continue;
		}
	}
	
	if (bolt == BOLT_BLINKING) {
		if (pmap[x][y].flags & HAS_MONSTER) { // We're blinking onto an area already occupied by a submerged monster.
			// Make sure we don't get the shooting monster by accident.
			shootingMonst->xLoc = shootingMonst->yLoc = -1; // Will be set back to the destination in a moment.
			monst = monsterAtLoc(x, y);
			findAlternativeHomeFor(monst, &x2, &y2, true);
			if (x2 >= 0) {
				// Found an alternative location.
				monst->xLoc = x2;
				monst->yLoc = y2;
				pmap[x][y].flags &= ~HAS_MONSTER;
				pmap[x2][y2].flags |= HAS_MONSTER;
			} else {
				// No alternative location?? Hard to imagine how this could happen.
				// Just bury the monster and never speak of this incident again.
				killCreature(monst, true);
				pmap[x][y].flags &= ~HAS_MONSTER;
				monst = NULL;
			}
		}
		pmap[x][y].flags |= (shootingMonst == &player ? HAS_PLAYER : HAS_MONSTER);
		shootingMonst->xLoc = x;
		shootingMonst->yLoc = y;
		applyInstantTileEffectsToCreature(shootingMonst);
		
		if (shootingMonst == &player) {
			updateVision(true);
		}
		autoID = true;
	} else if (bolt == BOLT_BECKONING) {
		if (monst && !(monst->info.flags & MONST_INANIMATE)
			&& distanceBetween(originLoc[0], originLoc[1], monst->xLoc, monst->yLoc) > 1) {
			beckonedMonster = true;
			fastForward = true;
		}
	}
	
	if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
		monst = monsterAtLoc(x, y);
		monsterName(monstName, monst, true);
	} else {
		monst = NULL;
	}
	
	switch(bolt) {
		case BOLT_TELEPORT:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					freeCaptive(monst);
				}
				teleport(monst);
				refreshSideBar(-1, -1, false);
			}
			break;
		case BOLT_BECKONING:
			if (beckonedMonster && monst) {
				if (canSeeMonster(monst)) {
					autoID = true;
				}
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					freeCaptive(monst);
				}
				newLoc[0] = monst->xLoc;
				newLoc[1] = monst->yLoc;
				zap(newLoc, originLoc, BOLT_BLINKING, max(1, (distanceBetween(originLoc[0], originLoc[1], newLoc[0], newLoc[1]) - 2) / 2), false);
				if (monst->ticksUntilTurn < player.attackSpeed+1) {
					monst->ticksUntilTurn = player.attackSpeed+1;
				}
				if (canSeeMonster(monst)) {
					autoID = true;
				}
			}
			break;
		case BOLT_SLOW:
			if (monst) {
				slow(monst, boltLevel);
				refreshSideBar(-1, -1, false);
				flashMonster(monst, boltColors[BOLT_SLOW], 100);
				autoID = true;
			}
			break;
		case BOLT_HASTE:
			if (monst) {
				haste(monst, staffHasteDuration(boltLevel));
				refreshSideBar(-1, -1, false);
				flashMonster(monst, boltColors[BOLT_HASTE], 100);
				autoID = true;
			}
			break;
		case BOLT_POLYMORPH:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				polymorph(monst);
				refreshSideBar(-1, -1, false);
				if (!monst->status[STATUS_INVISIBLE]) {
					autoID = true;
				}
			}
			break;
		case BOLT_INVISIBILITY:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				if (monst == &player || monst->creatureState == MONSTER_ALLY) {
					autoID = true;
				}
				monst->status[STATUS_INVISIBLE] = monst->maxStatus[STATUS_INVISIBLE] = 150;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
				refreshSideBar(-1, -1, false);
				if (!hideDetails) {
					flashMonster(monst, boltColors[BOLT_INVISIBILITY], 100);	
				}
			}
			break;
		case BOLT_DOMINATION:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				if (monst->currentHP * 5 < monst->info.maxHP
					|| rand_range(0, monst->info.maxHP) > monst->currentHP) {
					// domination succeeded
					monst->status[STATUS_DISCORDANT] = 0;
					becomeAllyWith(monst);
					refreshSideBar(-1, -1, false);
					refreshDungeonCell(monst->xLoc, monst->yLoc);
					if (canSeeMonster(monst)) {
						autoID = true;
						sprintf(buf, "%s is bound to your will!", monstName);
						message(buf, false);
						flashMonster(monst, boltColors[BOLT_DOMINATION], 100);
					}
				} else if (canSeeMonster(monst)) {
					autoID = true;
					sprintf(buf, "%s resists the bolt of domination.", monstName);
					message(buf, false);
				}
			}
			break;
		case BOLT_NEGATION:
			if (monst) { // works on inanimates
				negate(monst);
				refreshSideBar(-1, -1, false);
				flashMonster(monst, boltColors[BOLT_NEGATION], 100);
			}
			break;
		case BOLT_LIGHTNING:
			// already handled above
			break;
		case BOLT_POISON:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				poisonDamage = staffPoison(boltLevel);
				monst->status[STATUS_POISONED] += poisonDamage;
				monst->maxStatus[STATUS_POISONED] = monst->info.maxHP;
				refreshSideBar(-1, -1, false);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_POISON], 100);
					autoID = true;
					if (monst != &player) {
						sprintf(buf, "%s %s %s sick",
								monstName,
								(monst == &player ? "feel" : "looks"),
								(monst->status[STATUS_POISONED] > monst->currentHP ? "fatally" : "very"));
						combatMessage(buf, messageColorFromVictim(monst));
					}
				}
			}
			break;
		case BOLT_FIRE:
			if (monst) {
				autoID = true;
				
				if (monst->status[STATUS_IMMUNE_TO_FIRE] > 0) {
					if (canSeeMonster(monst)) {
						sprintf(buf, "%s ignore%s %s firebolt",
								monstName,
								(monst == &player ? "" : "s"),
								canSeeMonster(shootingMonst) ? "the" : "a");
						combatMessage(buf, 0);
					}
				} else if (inflictDamage(shootingMonst, monst, staffDamage(boltLevel), &orange)) {
					// killed creature
					
					if (player.currentHP <= 0) {
						if (shootingMonst == &player) {
							gameOver("Killed by a reflected firebolt", true);
						}
						return false;
					}
					
					if (boltInView) {
						sprintf(buf, "%s firebolt %s %s",
								canSeeMonster(shootingMonst) ? "the" : "a",
								((monst->info.flags & MONST_INANIMATE) ? "destroys" : "kills"),
								monstName);
						combatMessage(buf, messageColorFromVictim(monst));
					} else {
						sprintf(buf, "you hear %s %s", monstName, ((monst->info.flags & MONST_INANIMATE) ? "be destroyed" : "die"));
						combatMessage(buf, messageColorFromVictim(monst));
					}

				} else {
					// monster lives
					if (monst->creatureMode != MODE_PERM_FLEEING
						&& monst->creatureState != MONSTER_ALLY
						&& (monst->creatureState != MONSTER_FLEEING || monst->status[STATUS_MAGICAL_FEAR])) {
						monst->creatureState = MONSTER_TRACKING_SCENT;
					}
					if (boltInView) {
						sprintf(buf, "%s firebolt hits %s",
								canSeeMonster(shootingMonst) ? "the" : "a",
								monstName);
						combatMessage(buf, messageColorFromVictim(monst));
					}
					exposeCreatureToFire(monst);
					
					moralAttack(shootingMonst, monst);
				}
				refreshSideBar(-1, -1, false);
			}
			exposeTileToFire(x, y, true); // burninate
			break;
		case BOLT_BLINKING:
			if (shootingMonst == &player) {
				// handled above for visual effect (i.e. before contrail fades)
				// increase scent turn number so monsters don't sniff around at the old cell like idiots
				rogue.scentTurnNumber += 30;
				// get any items at the destination location
				if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
					pickUpItemAt(player.xLoc, player.yLoc);
				}
			}
			break;
		case BOLT_ENTRANCEMENT:
			if (monst && monst == &player) {
				flashMonster(monst, &confusionGasColor, 100);
				monst->status[STATUS_CONFUSED] = staffEntrancementDuration(boltLevel);
				monst->maxStatus[STATUS_CONFUSED] = max(monst->status[STATUS_CONFUSED], monst->maxStatus[STATUS_CONFUSED]);
				refreshSideBar(-1, -1, false);
				message("the bolt hits you and you suddently feel disoriented.", true);
				autoID = true;
			} else if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status[STATUS_ENTRANCED] = monst->maxStatus[STATUS_ENTRANCED] = boltLevel * 2;
				refreshSideBar(-1, -1, false);
				wakeUp(monst);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_ENTRANCEMENT], 100);
					autoID = true;
					sprintf(buf, "%s is entranced!", monstName);
					message(buf, false);
				}
			}
			break;
		case BOLT_HEALING:
			if (monst) {
				heal(monst, boltLevel * 10);
				refreshSideBar(-1, -1, false);
				if (canSeeMonster(monst)) {
					autoID = true;
				}
			}
			break;
		case BOLT_OBSTRUCTION:
			feat = dungeonFeatureCatalog[DF_FORCEFIELD];
			feat.probabilityDecrement = max(1, 75 * pow(0.8, boltLevel));
			spawnDungeonFeature(x, y, &feat, true, false);
			autoID = true;
			break;
		case BOLT_TUNNELING:
			if (autoID) {
				setUpWaypoints(); // recompute based on the new situation
			}
			break;
		case BOLT_PLENTY:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				newMonst = cloneMonster(monst, true, true);
				if (newMonst) {
					newMonst->currentHP = (newMonst->currentHP + 1) / 2;
					monst->currentHP = (monst->currentHP + 1) / 2;
					refreshSideBar(-1, -1, false);
					flashMonster(monst, boltColors[BOLT_PLENTY], 100);
					flashMonster(newMonst, boltColors[BOLT_PLENTY], 100);
					autoID = true;
				}
			}
			break;
		case BOLT_DISCORD:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status[STATUS_DISCORDANT] = monst->maxStatus[STATUS_DISCORDANT] = max(staffDiscordDuration(boltLevel), monst->status[STATUS_DISCORDANT]);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_DISCORD], 100);
					autoID = true;
				}
			}
			break;
		case BOLT_CONJURATION:
			
			for (j = 0; j < (staffBladeCount(boltLevel)); j++) {
				getQualifyingLocNear(newLoc, x, y, true, 0,
									 T_PATHING_BLOCKER & ~(T_LAVA_INSTA_DEATH | T_IS_DEEP_WATER | T_AUTO_DESCENT),
									 (HAS_PLAYER | HAS_MONSTER), false, false);
				monst = generateMonster(MK_SPECTRAL_BLADE, true);
				monst->xLoc = newLoc[0];
				monst->yLoc = newLoc[1];
				monst->bookkeepingFlags |= (MONST_FOLLOWER | MONST_BOUND_TO_LEADER | MONST_DOES_NOT_TRACK_LEADER);
				monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
				monst->leader = &player;
				monst->creatureState = MONSTER_ALLY;
				monst->ticksUntilTurn = monst->info.attackSpeed + 1; // So they don't move before the player's next turn.
				pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
				//fadeInMonster(monst);
			}
			refreshSideBar(-1, -1, false);
			monst = NULL;
			autoID = true;
			break;
		case BOLT_SHIELDING:
			if (monst) {
				monst->status[STATUS_SHIELDED] = max(monst->status[STATUS_SHIELDED], monst->status[STATUS_SHIELDED] + staffProtection(boltLevel));
				monst->maxStatus[STATUS_SHIELDED] = max(monst->status[STATUS_SHIELDED], monst->maxStatus[STATUS_SHIELDED]);
				flashMonster(monst, boltColors[BOLT_SHIELDING], 100);
				autoID = true;
			}
			break;
		default:
			break;
	}
	
	updateLighting();
	backUpLighting(lights);
	boltInView = true;
	refreshSideBar(-1, -1, false);
	if (boltLength > 0) {
		// j is where the front tip of the bolt would be if it hadn't collided at i
		for (j=i; j < i + boltLength + 2; j++) { // j can imply a bolt tip position that is off the map
			
			// dynamic lighting
			if (boltInView) {
				demoteVisibility();
				restoreLighting(lights);
                
                // k = j-i;
                // boltLights[k].lightRadius.lowerBound *= 2;
                // boltLights[k].lightRadius.upperBound *= 2;
                // boltLights[k].lightColor = &boltImpactColor;
                
				for (k = min(j, boltLength + 2); k >= j-i; k--) {
					if (k < initialBoltLength) {
						paintLight(&boltLights[k], listOfCoordinates[j-k][0], listOfCoordinates[j-k][1], false, false);
					}
				}
				updateFieldOfViewDisplay(false, true);
			}
			
			boltInView = false;
			
			// beam graphic
			// k iterates from the tail tip of the visible portion of the bolt to the head
			for (k = min(j, boltLength + 2); k >= j-i; k--) {
				if (pmap[listOfCoordinates[j-k][0]][listOfCoordinates[j-k][1]].flags & ANY_KIND_OF_VISIBLE) {
					hiliteCell(listOfCoordinates[j-k][0], listOfCoordinates[j-k][1], boltColor, max(0, 100 - k * 100 / (boltLength)), false);
					if (pmap[listOfCoordinates[j-k][0]][listOfCoordinates[j-k][1]].flags & ANY_KIND_OF_VISIBLE) {
						boltInView = true;
					}
				}
			}
			
			if (!fastForward && boltInView) {
				fastForward = rogue.playbackFastForward || pauseBrogue(10);
			}
		}
	}
	return autoID;
}

// Relies on the sidebar entity list. If one is already selected, select the next qualifying. Otherwise, target the first qualifying.
boolean nextTargetAfter(short *returnX,
                        short *returnY,
                        short targetX,
                        short targetY,
                        boolean targetEnemies,
                        boolean targetAllies,
                        boolean targetItems,
                        boolean requireOpenPath) {
    short i, n;
    short selectedIndex = -1;
    creature *monst;
    item *theItem;
    
    for (i=0; i<ROWS; i++) {
        if (rogue.sidebarLocationList[i][0] == targetX
            && rogue.sidebarLocationList[i][1] == targetY
            && (i == ROWS - 1 || rogue.sidebarLocationList[i+1][0] != targetX || rogue.sidebarLocationList[i+1][1] != targetY)) {
            
            selectedIndex = i;
            break;
        }
    }
    
    for (i=1; i<=ROWS; i++) {
        n = (selectedIndex + i) % ROWS;
        targetX = rogue.sidebarLocationList[n][0];
        targetY = rogue.sidebarLocationList[n][1];
        if (targetX != -1
            && (targetX != player.xLoc || targetY != player.yLoc)
            && (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, targetX, targetY))) {

#ifdef BROGUE_ASSERTS
            assert(coordinatesAreInMap(targetX, targetY));
#endif

            monst = monsterAtLoc(targetX, targetY);
            if (monst) {
                
                if (monstersAreEnemies(&player, monst)) {
                    if (targetEnemies) {
                        *returnX = targetX;
                        *returnY = targetY;
                        return true;
                    }
                } else {
                    if (targetAllies) {
                        *returnX = targetX;
                        *returnY = targetY;
                        return true;
                    }
                }
            }
            
            theItem = itemAtLoc(targetX, targetY);
            if (!monst && theItem && targetItems) {
                *returnX = targetX;
                *returnY = targetY;
                return true;
            }
        }
    }
    return false;
}
    
//	creature *currentTarget, *monst, *returnMonst = NULL;
//	short currentDistance, shortestDistance;
//	
//	currentTarget = monsterAtLoc(targetX, targetY);
//	
//	if (!currentTarget || currentTarget == &player) {
//		currentTarget = monsters;
//		currentDistance = 0;
//	} else {
//		currentDistance = distanceBetween(player.xLoc, player.yLoc, targetX, targetY);
//	}
//	
//	// first try to find a monster with the same distance later in the chain.
//	for (monst = currentTarget->nextCreature; monst != NULL; monst = monst->nextCreature) {
//		if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) == currentDistance
//			&& canSeeMonster(monst)
//			&& (targetAllies == (monst->creatureState == MONSTER_ALLY || (monst->bookkeepingFlags & MONST_CAPTIVE)))
//			&& (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
//			
//			// got one!
//			returnMonst = monst;
//			break;
//		}
//	}
//	
//	if (!returnMonst) {
//		// okay, instead pick the qualifying monster (excluding the current target)
//		// with the shortest distance greater than currentDistance.
//		shortestDistance = max(DCOLS, DROWS);
//		for (monst = currentTarget->nextCreature;; monst = monst->nextCreature) {
//			if (monst == NULL) {
//				monst = monsters;
//			} else if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) < shortestDistance
//					   && distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) > currentDistance
//					   && canSeeMonster(monst)
//					   && (targetAllies == (monst->creatureState == MONSTER_ALLY || (monst->bookkeepingFlags & MONST_CAPTIVE)))
//					   && (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
//				// potentially this one
//				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
//				returnMonst = monst;
//			}
//			if (monst == currentTarget) {
//				break;
//			}
//		}
//	}
//	
//	if (!returnMonst) {
//		// okay, instead pick the qualifying monster (excluding the current target)
//		// with the shortest distance period.
//		shortestDistance = max(DCOLS, DROWS);
//		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
//			if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) < shortestDistance
//				&& canSeeMonster(monst)
//				&& (targetAllies == (monst->creatureState == MONSTER_ALLY || (monst->bookkeepingFlags & MONST_CAPTIVE)))
//				&& (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
//				// potentially this one
//				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
//				returnMonst = monst;
//			}
//		}
//	}
//	
//	if (returnMonst) {
//		plotCharWithColor(returnMonst->info.displayChar, mapToWindowX(returnMonst->xLoc),
//						  mapToWindowY(returnMonst->yLoc), *(returnMonst->info.foreColor), white);
//	}
//	
//	return returnMonst;
//}

// Returns how far it went before hitting something.
short hiliteTrajectory(short coordinateList[DCOLS][2], short numCells, boolean eraseHiliting, boolean passThroughMonsters) {
	short x, y, i;
	creature *monst;
    
	for (i=0; i<numCells; i++) {
		x = coordinateList[i][0];
		y = coordinateList[i][1];
		if (eraseHiliting) {
			refreshDungeonCell(x, y);
		} else {
			//hiliteCell(x, y, &hiliteColor, 50, true); // yellow
			hiliteCell(x, y, &red, 50, true); // red
		}
		
		if (cellHasTerrainFlag(x, y, (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
			|| pmap[x][y].flags & (HAS_PLAYER)) {
			i++;
			break;
		} else if (!(pmap[x][y].flags & DISCOVERED)) {
			break;
		} else if (!passThroughMonsters && pmap[x][y].flags & (HAS_MONSTER)
				   && (playerCanSee(x, y) || player.status[STATUS_TELEPATHIC])) {
			monst = monsterAtLoc(x, y);
			if (!(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& (!monst->status[STATUS_INVISIBLE] || player.status[STATUS_TELEPATHIC])) {
                
				i++;
				break;
			}
		}
	}
	return i;
}

// Event is optional. Returns true if the event should be executed by the parent function.
boolean moveCursor(boolean *targetConfirmed,
				   boolean *canceled,
				   boolean *tabKey,
				   short targetLoc[2],
				   rogueEvent *event,
				   buttonState *state,
				   boolean colorsDance,
				   boolean keysMoveCursor,
				   boolean targetCanLeaveMap) {
	signed long keystroke;
	short moveIncrement;
	short buttonInput;
	boolean cursorMovementCommand, again, movementKeystroke, sidebarHighlighted;
	rogueEvent theEvent;
	
	short *cursor = rogue.cursorLoc; // shorthand
	
	cursor[0] = targetLoc[0];
	cursor[1] = targetLoc[1];
	
	*targetConfirmed = *canceled = *tabKey = false;
	sidebarHighlighted = false;
	
	do {
		again = false;
		cursorMovementCommand = false;
		movementKeystroke = false;
		
		assureCosmeticRNG;
		
		if (state) { // Also running a button loop.
			
			// Update the display.
			overlayDisplayBuffer(state->dbuf, NULL);
			
			// Get input.
			nextBrogueEvent(&theEvent, false, colorsDance, true);
			
			// Process the input.
			buttonInput = processButtonInput(state, NULL, &theEvent);
			
			if (buttonInput != -1) {
				state->buttonDepressed = state->buttonFocused = -1;
				drawButtonsInState(state);
			}
			
			// Revert the display.
			overlayDisplayBuffer(state->rbuf, NULL);
			
		} else { // No buttons to worry about.
			nextBrogueEvent(&theEvent, false, colorsDance, true);
		}
		restoreRNG;
		
		if (theEvent.eventType == MOUSE_UP || theEvent.eventType == MOUSE_ENTERED_CELL) {
			if (theEvent.param1 >= 0
				&& theEvent.param1 < mapToWindowX(0)
				&& theEvent.param2 >= 0
				&& theEvent.param2 < ROWS - 1
				&& rogue.sidebarLocationList[theEvent.param2][0] > -1) {
				
				// If the cursor is on an entity in the sidebar.
				cursor[0] = rogue.sidebarLocationList[theEvent.param2][0];
				cursor[1] = rogue.sidebarLocationList[theEvent.param2][1];
				sidebarHighlighted = true;
				cursorMovementCommand = true;
				refreshSideBar(cursor[0], cursor[1], false);
				if (theEvent.eventType == MOUSE_UP) {
					*targetConfirmed = true;
				}
			} else if (coordinatesAreInMap(windowToMapX(theEvent.param1), windowToMapY(theEvent.param2))
				|| targetCanLeaveMap && theEvent.eventType != MOUSE_UP) {
				
				// If the cursor is in the map area, or is allowed to leave the map and it isn't a click.
				if (theEvent.eventType == MOUSE_UP
					&& !theEvent.shiftKey
					&& (theEvent.controlKey || (cursor[0] == windowToMapX(theEvent.param1) && cursor[1] == windowToMapY(theEvent.param2)))) {
					
					*targetConfirmed = true;
				}
				cursor[0] = windowToMapX(theEvent.param1);
				cursor[1] = windowToMapY(theEvent.param2);
				cursorMovementCommand = true;
			} else {
				cursorMovementCommand = false;
				again = theEvent.eventType != MOUSE_UP;
			}
		} else if (theEvent.eventType == KEYSTROKE) {
			keystroke = theEvent.param1;
			moveIncrement = ( (theEvent.controlKey || theEvent.shiftKey) ? 5 : 1 );
			stripShiftFromMovementKeystroke(&keystroke);
			switch(keystroke) {
				case LEFT_ARROW:
				case LEFT_KEY:
				case NUMPAD_4:
					if (keysMoveCursor && cursor[0] > 0) {
						cursor[0] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case RIGHT_ARROW:
				case RIGHT_KEY:
				case NUMPAD_6:
					if (keysMoveCursor && cursor[0] < DCOLS - 1) {
						cursor[0] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case UP_ARROW:
				case UP_KEY:
				case NUMPAD_8:
					if (keysMoveCursor && cursor[1] > 0) {
						cursor[1] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case DOWN_ARROW:
				case DOWN_KEY:
				case NUMPAD_2:
					if (keysMoveCursor && cursor[1] < DROWS - 1) {
						cursor[1] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case UPLEFT_KEY:
				case NUMPAD_7:
					if (keysMoveCursor && cursor[0] > 0 && cursor[1] > 0) {
						cursor[0] -= moveIncrement;
						cursor[1] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case UPRIGHT_KEY:
				case NUMPAD_9:
					if (keysMoveCursor && cursor[0] < DCOLS - 1 && cursor[1] > 0) {
						cursor[0] += moveIncrement;
						cursor[1] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case DOWNLEFT_KEY:
				case NUMPAD_1:
					if (keysMoveCursor && cursor[0] > 0 && cursor[1] < DROWS - 1) {
						cursor[0] -= moveIncrement;
						cursor[1] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case DOWNRIGHT_KEY:
				case NUMPAD_3:
					if (keysMoveCursor && cursor[0] < DCOLS - 1 && cursor[1] < DROWS - 1) {
						cursor[0] += moveIncrement;
						cursor[1] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case TAB_KEY:
				case NUMPAD_0:
					*tabKey = true;
					break;
				case RETURN_KEY:
				case ENTER_KEY:
					*targetConfirmed = true;
					break;
				case ESCAPE_KEY:
				case ACKNOWLEDGE_KEY:
					*canceled = true;
					break;
				default:
					break;
			}
		} else if (theEvent.eventType == RIGHT_MOUSE_UP) {
			// do nothing
		} else {
			again = true;
		}
		
		if (sidebarHighlighted
			&& (!(pmap[cursor[0]][cursor[1]].flags & (HAS_PLAYER | HAS_MONSTER))
								   || !canSeeMonster(monsterAtLoc(cursor[0], cursor[1])))
			&& (!(pmap[cursor[0]][cursor[1]].flags & HAS_ITEM)) || (playerCanSeeOrSense(cursor[0], cursor[1]))) {
			
			// The sidebar is highlighted but the cursor is not on a visible item or monster. Un-highlight the sidebar.
			refreshSideBar(-1, -1, false);
			sidebarHighlighted = false;
		}
		
		if (targetCanLeaveMap && !movementKeystroke) {
			// permit it to leave the map by up to 1 space in any direction if mouse controlled.
			cursor[0] = clamp(cursor[0], -1, DCOLS);
			cursor[1] = clamp(cursor[1], -1, DROWS);
		} else {
			cursor[0] = clamp(cursor[0], 0, DCOLS - 1);
			cursor[1] = clamp(cursor[1], 0, DROWS - 1);
		}
	} while (again && (!event || !cursorMovementCommand));
	
	if (event) {
		*event = theEvent;
	}
	
	if (sidebarHighlighted) {
		// Don't leave the sidebar highlighted when we exit.
		refreshSideBar(-1, -1, false);
		sidebarHighlighted = false;
	}
	
	targetLoc[0] = cursor[0];
	targetLoc[1] = cursor[1];
	
	return !cursorMovementCommand;
}

void pullMouseClickDuringPlayback(short loc[2]) {
	rogueEvent theEvent;
	
#ifdef BROGUE_ASSERTS
	assert(rogue.playbackMode);
#endif
	nextBrogueEvent(&theEvent, false, false, false);
	loc[0] = windowToMapX(theEvent.param1);
	loc[1] = windowToMapY(theEvent.param2);
}

// Return true if a target is chosen, or false if canceled.
boolean chooseTarget(short returnLoc[2],
					 short maxDistance,
					 boolean stopAtTarget,
					 boolean autoTarget,
					 boolean targetAllies,
					 boolean passThroughCreatures) {
	short originLoc[2], targetLoc[2], oldTargetLoc[2], coordinates[DCOLS][2], numCells, i, distance, newX, newY;
	creature *monst;
	boolean canceled, targetConfirmed, tabKey, cursorInTrajectory, focusedOnMonster = false;
	rogueEvent event;
		
	if (rogue.playbackMode) {
		// In playback, pull the next event (a mouseclick) and use that location as the target.
		pullMouseClickDuringPlayback(returnLoc);
		rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
		return true;
	}
	
	assureCosmeticRNG;
	
	originLoc[0] = player.xLoc;
	originLoc[1] = player.yLoc;
	
	targetLoc[0] = oldTargetLoc[0] = player.xLoc;
	targetLoc[1] = oldTargetLoc[1] = player.yLoc;
	
	if (autoTarget) {
	
		if (rogue.lastTarget
			&& canSeeMonster(rogue.lastTarget)
			&& (targetAllies == (rogue.lastTarget->creatureState == MONSTER_ALLY))
			&& rogue.lastTarget->depth == rogue.depthLevel
			&& !(rogue.lastTarget->bookkeepingFlags & MONST_IS_DYING)
			&& openPathBetween(player.xLoc, player.yLoc, rogue.lastTarget->xLoc, rogue.lastTarget->yLoc)) {
			
			monst = rogue.lastTarget;
		} else {
			//rogue.lastTarget = NULL;
			if (nextTargetAfter(&newX, &newY, targetLoc[0], targetLoc[1], !targetAllies, targetAllies, false, true)) {
                targetLoc[0] = newX;
                targetLoc[1] = newY;
            }
            monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
		}
		if (monst) {
			targetLoc[0] = monst->xLoc;
			targetLoc[1] = monst->yLoc;
			refreshSideBar(monst->xLoc, monst->yLoc, false);
			focusedOnMonster = true;
		}
	}
	
	numCells = getLineCoordinates(coordinates, originLoc, targetLoc);
	if (maxDistance > 0) {
		numCells = min(numCells, maxDistance);
	}
	if (stopAtTarget) {
		numCells = min(numCells, distanceBetween(player.xLoc, player.yLoc, targetLoc[0], targetLoc[1]));
	}
	
	targetConfirmed = canceled = tabKey = false;
	
	do {
		printLocationDescription(targetLoc[0], targetLoc[1]);
		
		if (canceled) {
			refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
			hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
			confirmMessages();
			rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
			restoreRNG;
			return false;
		}
		
		if (tabKey) {
			if (nextTargetAfter(&newX, &newY, targetLoc[0], targetLoc[1], !targetAllies, targetAllies, false, true)) {
                targetLoc[0] = newX;
                targetLoc[1] = newY;
            }
		}
		
		monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
		if (monst != NULL && monst != &player && canSeeMonster(monst)) {
			refreshSideBar(monst->xLoc, monst->yLoc, false);
			focusedOnMonster = true;
		} else if (focusedOnMonster) {
			refreshSideBar(-1, -1, false);
			focusedOnMonster = false;
		}
		
		refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
		hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
		
		if (!targetConfirmed) {
			numCells = getLineCoordinates(coordinates, originLoc, targetLoc);
			if (maxDistance > 0) {
				numCells = min(numCells, maxDistance);
			}
			
			if (stopAtTarget) {
				numCells = min(numCells, distanceBetween(player.xLoc, player.yLoc, targetLoc[0], targetLoc[1]));
			}
			distance = hiliteTrajectory(coordinates, numCells, false, passThroughCreatures);
			cursorInTrajectory = false;
			for (i=0; i<distance; i++) {
				if (coordinates[i][0] == targetLoc[0] && coordinates[i][1] == targetLoc[1]) {
					cursorInTrajectory = true;
					break;
				}
			}
			hiliteCell(targetLoc[0], targetLoc[1], &white, (cursorInTrajectory ? 100 : 35), true);	
		}
		
		oldTargetLoc[0] = targetLoc[0];
		oldTargetLoc[1] = targetLoc[1];
		moveCursor(&targetConfirmed, &canceled, &tabKey, targetLoc, &event, NULL, false, true, false);
		if (event.eventType == RIGHT_MOUSE_UP) { // Right mouse cancels.
			canceled = true;
		}
	} while (!targetConfirmed);
	if (maxDistance > 0) {
		numCells = min(numCells, maxDistance);
	}
	hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
	refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
	
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		confirmMessages();
		restoreRNG;
		rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
		return false;
	}
	
	monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
	if (monst && monst != &player && canSeeMonster(monst)) {
		rogue.lastTarget = monst;
	}
	
	returnLoc[0] = targetLoc[0];
	returnLoc[1] = targetLoc[1];
	restoreRNG;
	rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
	return true;
}

void autoIdentify(item *theItem) {
	short quantityBackup;
	char buf[COLS], buf2[COLS];
	
	tableForItemCategory(theItem->category)[theItem->kind].identified = true;
	quantityBackup = theItem->quantity;
	theItem->quantity = 1;
	itemName(theItem, buf2, false, false, NULL);
	theItem->quantity = quantityBackup;
	sprintf(buf, "(It must %s a %s.)",
			((theItem->category & (POTION | SCROLL)) ? "have been" : "be"),
			buf2);
	messageWithColor(buf, &itemMessageColor, false);
}

// returns whether the item disappeared
boolean hitMonsterWithProjectileWeapon(creature *thrower, creature *monst, item *theItem) {
	char buf[DCOLS], theItemName[DCOLS], targetName[DCOLS], armorRunicString[DCOLS];
	boolean thrownWeaponHit;
	item *equippedWeapon;
	short damage;
	
	if (!(theItem->category & WEAPON)) {
		return false;
	}
	
	armorRunicString[0] = '\0';
	
	itemName(theItem, theItemName, false, false, NULL);
	monsterName(targetName, monst, true);
	
	monst->status[STATUS_ENTRANCED] = 0;
	
	if (monst != &player
		&& monst->creatureMode != MODE_PERM_FLEEING
		&& (monst->creatureState != MONSTER_FLEEING || monst->status[STATUS_MAGICAL_FEAR])
		&& !(monst->bookkeepingFlags & MONST_CAPTIVE)) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
		if (monst->status[STATUS_MAGICAL_FEAR]) {
			monst->status[STATUS_MAGICAL_FEAR] = 1;
		}
	}
	
	if (thrower == &player) {
		equippedWeapon = rogue.weapon;
		equipItem(theItem, true);
		thrownWeaponHit = attackHit(&player, monst);
		if (equippedWeapon) {
			equipItem(equippedWeapon, true);
		} else {
			unequipItem(theItem, true);
		}
	} else {
		thrownWeaponHit = attackHit(thrower, monst);
	}
	
	if (thrownWeaponHit) {
		damage = (monst->info.flags & MONST_IMMUNE_TO_WEAPONS ? 0 :
				  randClump(theItem->damage)) * pow(WEAPON_ENCHANT_FACTOR, netEnchant(theItem));
		
		if (monst == &player) {
			applyArmorRunicEffect(armorRunicString, thrower, &damage, false);
		}
		
		if (inflictDamage(thrower, monst, damage, &red)) { // monster killed
			sprintf(buf, "the %s killed %s.", theItemName, targetName);
			messageWithColor(buf, messageColorFromVictim(monst), false);
		} else {
			sprintf(buf, "the %s hit %s.", theItemName, targetName);
			if (theItem->flags & ITEM_RUNIC) {
				magicWeaponHit(monst, theItem, false);
			}
			messageWithColor(buf, messageColorFromVictim(monst), false);
			
			moralAttack(thrower, monst);
		}
		if (armorRunicString[0]) {
			message(armorRunicString, false);
		}
		return true;
	} else {
		theItem->flags &= ~ITEM_PLAYER_AVOIDS; // Don't avoid thrown weapons that missed.
		sprintf(buf, "the %s missed %s.", theItemName, targetName);
		message(buf, false);
		return false;
	}
}

void throwItem(item *theItem, creature *thrower, short targetLoc[2], short maxDistance) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2], originLoc[2];
	short i, x, y, numCells;
	creature *monst = NULL;
	char buf[COLS*3], buf2[COLS], buf3[COLS];
	uchar displayChar;
	color foreColor, backColor, multColor;
	short dropLoc[2];
	boolean hitSomethingSolid = false, fastForward = false;
	
	theItem->flags |= ITEM_PLAYER_AVOIDS; // Avoid thrown items, unless it's a weapon that misses a monster.
	
	x = originLoc[0] = thrower->xLoc;
	y = originLoc[1] = thrower->yLoc;
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	thrower->ticksUntilTurn = thrower->attackSpeed;
	
	if (thrower != &player && pmap[originLoc[0]][originLoc[1]].flags & IN_FIELD_OF_VIEW) {
		monsterName(buf2, thrower, true);
		itemName(theItem, buf3, false, true, NULL);
		sprintf(buf, "%s hurls %s.", buf2, buf3);
		message(buf, false);
	}
	
	for (i=0; i<numCells && i < maxDistance; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(x, y);
			
//			if (projectileReflects(thrower, monst) && i < DCOLS*2) {
//				if (projectileReflects(thrower, monst)) { // if it scores another reflection roll, reflect at caster
//					numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, true);
//				} else {
//					numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
//				}
//				
//				monsterName(buf2, monst, true);
//				itemName(theItem, buf3, false, false, NULL);
//				sprintf(buf, "%s deflect%s the %s", buf2, (monst == &player ? "" : "s"), buf3);
//				combatMessage(buf, 0);
//				continue;
//			}
			
			if ((theItem->category & WEAPON)
				&& theItem->kind != INCENDIARY_DART
				&& hitMonsterWithProjectileWeapon(thrower, monst, theItem)) {
				return;
			}
			
			break;
		}
		
		// We hit something!
		if (cellHasTerrainFlag(x, y, (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION))) {
			if ((theItem->category & WEAPON)
				&& (theItem->kind == INCENDIARY_DART)
				&& (cellHasTerrainFlag(x, y, T_IS_FLAMMABLE) || (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)))) {
				// Incendiary darts thrown at flammable obstructions (foliage, wooden barricades, doors) will hit the obstruction
				// instead of bursting a cell earlier.
			} else {
				i--;
				if (i >= 0) {
					x = listOfCoordinates[i][0];
					y = listOfCoordinates[i][1];
				} else { // it was aimed point-blank into an obstruction
					x = thrower->xLoc;
					y = thrower->yLoc;
				}
			}
			hitSomethingSolid = true;
			break;
		}
		
		if (playerCanSee(x, y)) { // show the graphic
			getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
			foreColor = *(theItem->foreColor);
			if (pmap[x][y].flags & VISIBLE) {
				colorMultiplierFromDungeonLight(x, y, &multColor);
				applyColorMultiplier(&foreColor, &multColor);
			} else { // clairvoyant visible
				applyColorMultiplier(&foreColor, &clairvoyanceColor);
			}
			plotCharWithColor(theItem->displayChar, mapToWindowX(x), mapToWindowY(y), foreColor, backColor);
			
			if (!fastForward) {
				fastForward = rogue.playbackFastForward || pauseBrogue(25);
			}
			
			refreshDungeonCell(x, y);
		}
		
		if (x == targetLoc[0] && y == targetLoc[1]) { // reached its target
			break;
		}
	}	
	
	if ((theItem->category & POTION) && (hitSomethingSolid || !cellHasTerrainFlag(x, y, T_AUTO_DESCENT))) {
		if (theItem->kind == POTION_CONFUSION || theItem->kind == POTION_POISON
			|| theItem->kind == POTION_PARALYSIS || theItem->kind == POTION_INCINERATION
			|| theItem->kind == POTION_DARKNESS || theItem->kind == POTION_LICHEN
			|| theItem->kind == POTION_DESCENT) {
			switch (theItem->kind) {
				case POTION_POISON:
					strcpy(buf, "the flask shatters and a deadly purple cloud billows out!");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_POISON_GAS_CLOUD_POTION], true, false);
					message(buf, false);
					break;
				case POTION_CONFUSION:
					strcpy(buf, "the flask shatters and a multi-hued cloud billows out!");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_CONFUSION_GAS_CLOUD_POTION], true, false);
					message(buf, false);
					break;
				case POTION_PARALYSIS:
					strcpy(buf, "the flask shatters and a cloud of pink gas billows out!");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_PARALYSIS_GAS_CLOUD_POTION], true, false);
					message(buf, false);
					break;
				case POTION_INCINERATION:
					strcpy(buf, "the flask shatters and its contents burst violently into flame!");
					message(buf, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_INCINERATION_POTION], true, false);
					break;
				case POTION_DARKNESS:
					strcpy(buf, "the flask shatters and the lights in the area start fading.");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_DARKNESS_POTION], true, false);
					message(buf, false);
					break;
				case POTION_DESCENT:
					strcpy(buf, "as the flask shatters, the ground vanishes!");
					message(buf, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_HOLE_POTION], true, false);
					break;
				case POTION_LICHEN:
					strcpy(buf, "the flask shatters and deadly spores spill out!");
					message(buf, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_LICHEN_PLANTED], true, false);
					break;
			}
			
			autoIdentify(theItem);
			
			refreshDungeonCell(x, y);
			
			//if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			//	monst = monsterAtLoc(x, y);
			//	applyInstantTileEffectsToCreature(monst);
			//}
		} else {
			if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
				strcpy(buf2, "against");
			} else if (tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].flags & T_STAND_IN_TILE) {
				strcpy(buf2, "into");
			} else {
				strcpy(buf2, "on");
			}
			sprintf(buf, "the flask shatters and %s liquid splashes harmlessly %s %s.",
					potionTable[theItem->kind].flavor, buf2, tileText(x, y));
			message(buf, false);
			if (theItem->kind == POTION_HALLUCINATION && (theItem->flags & ITEM_MAGIC_DETECTED)) {
				autoIdentify(theItem);
			}
		}
		return; // potions disappear when they break
	}
	if ((theItem->category & WEAPON) && theItem->kind == INCENDIARY_DART) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_DART_EXPLOSION], true, false);
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			exposeCreatureToFire(monsterAtLoc(x, y));
		}
		return;
	}
	getQualifyingLocNear(dropLoc, x, y, true, 0, (T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_PASSABILITY), (HAS_ITEM), false, false);
	placeItem(theItem, dropLoc[0], dropLoc[1]);
	refreshDungeonCell(dropLoc[0], dropLoc[1]);
}

void throwCommand(item *theItem) {
	item *thrownItem;
	char buf[COLS], theName[COLS];
	unsigned char command[10];
	short maxDistance, zapTarget[2], originLoc[2], quantity;
	boolean autoTarget;
	
	command[0] = THROW_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType((ALL_ITEMS), 0, 0, "Throw what? (a-z, shift for more info; or <esc> to cancel)", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	quantity = theItem->quantity;
	theItem->quantity = 1;
	itemName(theItem, theName, false, false, NULL);
	theItem->quantity = quantity;
	
	command[1] = theItem->inventoryLetter;
	confirmMessages();
	
	if ((theItem->flags & ITEM_EQUIPPED) && theItem->quantity <= 1) {
		sprintf(buf, "Are you sure you want to throw your %s?", theName);
		if (!confirm(buf, false)) {
			return;
		}
		if (theItem->flags & ITEM_CURSED) {
			sprintf(buf, "You cannot unequip your %s; it appears to be cursed.", theName);
			messageWithColor(buf, &itemMessageColor, false);
			return;
		} else {
			unequipItem(theItem, false);
		}
	}
	
	sprintf(buf, "Throw %s %s where? (<hjklyubn>, mouse, or <tab>)",
			(theItem->quantity > 1 ? "a" : "your"),
			theName);
	temporaryMessage(buf, false);
	maxDistance = (12 + 2 * max(rogue.strength - player.weaknessAmount - 12, 0));
	autoTarget = (theItem->category & (WEAPON | POTION)) ? true : false;
	if (chooseTarget(zapTarget, maxDistance, true, autoTarget, false, false)) {
		command[2] = '\0';
		recordKeystrokeSequence(command);
		recordMouseClick(mapToWindowX(zapTarget[0]), mapToWindowY(zapTarget[1]), true, false);
		
		confirmMessages();
		
		thrownItem = generateItem(ALL_ITEMS, -1);
		*thrownItem = *theItem; // clone the item
		thrownItem->flags &= ~ITEM_EQUIPPED;
		thrownItem->quantity = 1;
		
		itemName(thrownItem, theName, false, false, NULL);
		originLoc[0] = player.xLoc;
		originLoc[1] = player.yLoc;
		
		throwItem(thrownItem, &player, zapTarget, maxDistance);
	} else {
		return;
	}
	
	// Now decrement or delete the thrown item out of the inventory.
	if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		removeItemFromChain(theItem, packItems);
		deleteItem(theItem);
	}
	playerTurnEnded();
}

void apply(item *theItem, boolean recordCommands) {
	char buf[COLS], buf2[COLS];
	unsigned char command[10];
	short zapTarget[2], originLoc[2], maxDistance, c;
	boolean autoTarget, targetAllies, autoID, passThroughCreatures, commandsRecorded, revealItemType;
	
	commandsRecorded = !recordCommands;
	c = 0;
	command[c++] = APPLY_KEY;
	
	revealItemType = false;
	
	if (!theItem) {
		theItem = promptForItemOfType((SCROLL|FOOD|POTION|STAFF|WAND), 0, 0,
									  "Apply what? (a-z, shift for more info; or <esc> to cancel)", true);
	}
	
	if (theItem == NULL) {
		return;
	}
	command[c++] = theItem->inventoryLetter;
	confirmMessages();
	switch (theItem->category) {
		case FOOD:
			if (STOMACH_SIZE - player.status[STATUS_NUTRITION] < foodTable[theItem->kind].strengthRequired) { // Not hungry enough.
				sprintf(buf, "You're not hungry enough to fully enjoy the %s. Eat it anyway?",
						(theItem->kind == RATION ? "food" : "mango"));
				if (!confirm(buf, false)) {
					return;
				}
			}
			player.status[STATUS_NUTRITION] = min(foodTable[theItem->kind].strengthRequired + player.status[STATUS_NUTRITION], STOMACH_SIZE);
			if (theItem->kind == RATION) {
				messageWithColor("That food tasted delicious!", &itemMessageColor, false);
			} else {
				messageWithColor("My, what a yummy mango!", &itemMessageColor, false);
			}
			break;
		case POTION:
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true; // have to record in case further keystrokes are necessary (e.g. enchant scroll)
			if (!potionTable[theItem->kind].identified) {
				revealItemType = true;
			}
			drinkPotion(theItem);
			break;
		case SCROLL:
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true; // have to record in case further keystrokes are necessary (e.g. enchant scroll)
			if (!scrollTable[theItem->kind].identified
				&& theItem->kind != SCROLL_ENCHANT_ITEM
				&& theItem->kind != SCROLL_IDENTIFY) {
				
				revealItemType = true;
			}
			readScroll(theItem);
			break;
		case STAFF:
		case WAND:
			if (theItem->charges <= 0 && (theItem->flags & ITEM_IDENTIFIED)) {
				itemName(theItem, buf2, false, false, NULL);
				sprintf(buf, "Your %s has no charges.", buf2);
				messageWithColor(buf, &itemMessageColor, false);
				return;
			}
			temporaryMessage("Direction? (<hjklyubn>, mouse, or <tab>; <return> to confirm)", false);
			itemName(theItem, buf2, false, false, NULL);
			sprintf(buf, "Zapping your %s:", buf2);
			printString(buf, mapToWindowX(0), 1, &itemMessageColor, &black, NULL);
			if (theItem->category & STAFF && theItem->kind == STAFF_BLINKING
				&& theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) {
				maxDistance = theItem->enchant1 * 2 + 2;
			} else {
				maxDistance = -1;
			}
			autoTarget = true;
			if (theItem->category & STAFF && staffTable[theItem->kind].identified &&
				(theItem->kind == STAFF_BLINKING
				 || theItem->kind == STAFF_TUNNELING)) {
					autoTarget = false;
				}
			targetAllies = false;
			if (((theItem->category & STAFF) && staffTable[theItem->kind].identified &&
				 (theItem->kind == STAFF_HEALING || theItem->kind == STAFF_HASTE || theItem->kind == STAFF_PROTECTION))
				|| ((theItem->category & WAND) && wandTable[theItem->kind].identified &&
					(theItem->kind == WAND_INVISIBILITY || theItem->kind == WAND_PLENTY))) {
					targetAllies = true;
				}
			passThroughCreatures = false;
			if ((theItem->category & STAFF) && staffTable[theItem->kind].identified &&
				theItem->kind == STAFF_LIGHTNING) {
				passThroughCreatures = true;
			}
			if (chooseTarget(zapTarget, maxDistance, false, autoTarget, targetAllies, passThroughCreatures)) {
				command[c] = '\0';
				if (!commandsRecorded) {
					recordKeystrokeSequence(command);
					recordMouseClick(mapToWindowX(zapTarget[0]), mapToWindowY(zapTarget[1]), true, false);
					commandsRecorded = true;
				}
				confirmMessages();
				
				originLoc[0] = player.xLoc;
				originLoc[1] = player.yLoc;
				
				if (theItem->charges > 0) {
					autoID = (zap(originLoc, zapTarget,
								  (theItem->kind + (theItem->category == STAFF ? NUMBER_WAND_KINDS : 0)),		// bolt type
								  (theItem->category == STAFF ? theItem->enchant1 : 10),						// bolt level
								  !(((theItem->category & WAND) && wandTable[theItem->kind].identified)
									|| ((theItem->category & STAFF) && staffTable[theItem->kind].identified))));	// hide bolt details
					if (autoID) {
						if (theItem->category & STAFF) {
							if (!staffTable[theItem->kind].identified) {
								staffTable[theItem->kind].identified = true;
								revealItemType = true;
							}
						} else {
							if (!wandTable[theItem->kind].identified) {
								wandTable[theItem->kind].identified = true;
								revealItemType = true;
							}
						}
					}
				} else {
					itemName(theItem, buf2, false, false, NULL);
					if (theItem->category == STAFF) {
						sprintf(buf, "Your %s fizzles; it must be out of charges for now.", buf2);
					} else {
						sprintf(buf, "Your %s fizzles; it must be depleted.", buf2);
					}
					messageWithColor(buf, &itemMessageColor, false);
					theItem->flags |= ITEM_MAX_CHARGES_KNOWN;
					playerTurnEnded();
					return;
				}
			} else {
				return;
			}
			break;
		default:
			itemName(theItem, buf2, false, true, NULL);
			sprintf(buf, "you can't apply %s.", buf2);
			message(buf, false);
			return;
	}
	
	if (!commandsRecorded) { // to make sure we didn't already record the keystrokes above with staff/wand targeting
		command[c] = '\0';
		recordKeystrokeSequence(command);
	}
	
	// Reveal the item type if appropriate.
	if (revealItemType) {
		autoIdentify(theItem);
	}
	
	if (theItem->charges > 0) {
		theItem->charges--;
		if (theItem->category == WAND) {
			theItem->enchant2++; // keeps track of how many times the wand has been discharged for the player's convenience
		}
	} else if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		removeItemFromChain(theItem, packItems);
		deleteItem(theItem);
	}
	playerTurnEnded();
}

void identify(item *theItem) {
	itemTable *theTable;
	short tableCount, i, lastItem;
	
	tableCount = 0;
	lastItem = -1;
	
	theItem->flags |= ITEM_IDENTIFIED;
	theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	if (theItem->flags & ITEM_RUNIC) {
		theItem->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
	}
	
	switch (theItem->category) {
		case SCROLL:
			scrollTable[theItem->kind].identified = true;
			theTable = scrollTable;
			tableCount = NUMBER_SCROLL_KINDS;
			break;
		case POTION:
			potionTable[theItem->kind].identified = true;
			theTable = potionTable;
			tableCount = NUMBER_POTION_KINDS;
			break;
		case WAND:
			wandTable[theItem->kind].identified = true;
			theTable = wandTable;
			tableCount = NUMBER_WAND_KINDS;
			break;
		case STAFF:
			staffTable[theItem->kind].identified = true;
			theTable = staffTable;
			tableCount = NUMBER_STAFF_KINDS;
			break;
		case RING:
			ringTable[theItem->kind].identified = true;
			theTable = ringTable;
			tableCount = NUMBER_RING_KINDS;
			break;
		default:
			break;
	}
	
	if (tableCount) {
		theTable[theItem->kind].identified = true;
		// TODO: this part doesn't work for some reason.
		for (i=0; i<tableCount; i++) {
			if (!(theTable[i].identified)) {
				if (lastItem != -1) {
					return; // at least two unidentified items remain
				}
				lastItem = i;
			}
		}
		if (lastItem != -1) {
			// exactly one unidentified item remains
			theTable[lastItem].identified = true;
		}
	}
}

enum monsterTypes chooseVorpalEnemy() {
	short i, index, possCount = 0, deepestLevel = 0, deepestHorde, chosenHorde, failsafe = 25;
	enum monsterTypes candidate;
	
	do {
		for (i=0; i<NUMBER_HORDES; i++) {
			if (hordeCatalog[i].minLevel >= rogue.depthLevel && !hordeCatalog[i].flags) {
				possCount += hordeCatalog[i].frequency;
			}
			if (hordeCatalog[i].minLevel > deepestLevel) {
				deepestHorde = i;
				deepestLevel = hordeCatalog[i].minLevel;
			}
		}
		
		if (possCount == 0) {
			chosenHorde = deepestHorde;
		} else {
			index = rand_range(1, possCount);
			for (i=0; i<NUMBER_HORDES; i++) {
				if (hordeCatalog[i].minLevel >= rogue.depthLevel && !hordeCatalog[i].flags) {
					if (index <= hordeCatalog[i].frequency) {
						chosenHorde = i;
						break;
					}
					index -= hordeCatalog[i].frequency;
				}
			}
		}
		
		index = rand_range(-1, hordeCatalog[chosenHorde].numberOfMemberTypes - 1);
		if (index == -1) {
			candidate = hordeCatalog[chosenHorde].leaderType;
		} else {
			candidate = hordeCatalog[chosenHorde].memberType[index];
		}
	} while ((monsterCatalog[candidate].flags & MONST_NEVER_VORPAL_ENEMY) && --failsafe > 0);
	return candidate;
}

void updateIdentifiableItem(item *theItem) {
	if (((theItem->category & SCROLL) && scrollTable[theItem->kind].identified == true)
		|| ((theItem->category & POTION) && potionTable[theItem->kind].identified == true)) {
		
		theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	} else if ((theItem->category & (RING | STAFF | WAND))
			   && (theItem->flags & ITEM_IDENTIFIED)
			   && tableForItemCategory(theItem->category)[theItem->kind].identified) {
		
		theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	} else if ((theItem->category & (WEAPON | ARMOR))
			   && (theItem->flags & ITEM_IDENTIFIED)
			   && (!(theItem->flags & ITEM_RUNIC) || (theItem->flags & ITEM_RUNIC_IDENTIFIED))) {
		
		theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	}
}

void updateIdentifiableItems() {
	item *theItem;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		updateIdentifiableItem(theItem);
	}
	for (theItem = floorItems; theItem != NULL; theItem = theItem->nextItem) {
		updateIdentifiableItem(theItem);
	}
}

void crystalize(short radius) {
	extern color forceFieldColor;
	short i, j;
	creature *monst;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j < DROWS; j++) {
			if ((player.xLoc - i) * (player.xLoc - i) + (player.yLoc - j) * (player.yLoc - j) <= radius * radius
				&& !(pmap[i][j].flags & IMPREGNABLE)) {
				
				if (i == 0 || i == DCOLS - 1 || j == 0 || j == DROWS - 1) {
					pmap[i][j].layers[DUNGEON] = CRYSTAL_WALL; // don't dissolve the boundary walls
				} else if (tileCatalog[pmap[i][j].layers[DUNGEON]].flags & (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION)) {
					
					pmap[i][j].layers[DUNGEON] = FORCEFIELD;
					
					if (pmap[i][j].flags & HAS_MONSTER) {
						monst = monsterAtLoc(i, j);
						if (monst->info.flags & MONST_ATTACKABLE_THRU_WALLS) {
							inflictDamage(&player, monst, monst->currentHP, NULL);
						} else {
							freeCaptivesEmbeddedAt(i, j);
						}
					}
				}
			}
		}
	}
	updateVision(false);
	lightFlash(&forceFieldColor, 0, 0, radius, radius, player.xLoc, player.yLoc);
	displayLevel();
	refreshSideBar(-1, -1, false);
}

void readScroll(item *theItem) {
	short i, j, x, y, numberOfMonsters = 0;
	item *tempItem;
	creature *monst, *nextMonst;
	boolean hadEffect = false;
	char buf[2*COLS], buf2[COLS];
	
	switch (theItem->kind) {
		case SCROLL_IDENTIFY:
			identify(theItem);
			updateIdentifiableItems();
			messageWithColor("this is a scroll of identify.", &itemMessageColor, true);
			if (numberOfMatchingPackItems(ALL_ITEMS, ITEM_CAN_BE_IDENTIFIED, 0, false) == 0) {
				message("everything in your pack is already identified.", false);
				break;
			}
			do {
				theItem = promptForItemOfType((ALL_ITEMS), ITEM_CAN_BE_IDENTIFIED, 0,
											  "Identify what? (a-z; shift for more info)", false);
				if (rogue.gameHasEnded) {
					return;
				}
				if (theItem && !(theItem->flags & ITEM_CAN_BE_IDENTIFIED)) {
					confirmMessages();
					itemName(theItem, buf2, true, true, NULL);
					sprintf(buf, "you already know %s %s.", (theItem->quantity > 1 ? "they're" : "it's"), buf2);
					messageWithColor(buf, &itemMessageColor, false);
				}
			} while (theItem == NULL || !(theItem->flags & ITEM_CAN_BE_IDENTIFIED));
			recordKeystroke(theItem->inventoryLetter, false, false);
			confirmMessages();
			identify(theItem);
			itemName(theItem, buf, true, true, NULL);
			sprintf(buf2, "%s %s.", (theItem->quantity == 1 ? "this is" : "these are"), buf);
			messageWithColor(buf2, &itemMessageColor, false);
			break;
		case SCROLL_TELEPORT:
			teleport(&player);
			break;
		case SCROLL_REMOVE_CURSE:
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->flags & ITEM_CURSED) {
					hadEffect = true;
					tempItem->flags &= ~ITEM_CURSED;
				}
			}
			if (hadEffect) {
				message("your pack glows with a cleansing light, and a malevolent energy disperses.", false);
			} else {
				message("your pack glows with a cleansing light, but nothing happens.", false);
			}
			break;
		case SCROLL_ENCHANT_ITEM:
			identify(theItem);
			messageWithColor("this is a scroll of enchantment.", &itemMessageColor, true);
			if (!numberOfMatchingPackItems(WEAPON | ARMOR | RING | STAFF | WAND, 0, 0, false)) {
				confirmMessages();
				message("you have nothing that can be enchanted.", false);
				break;
			}
			do {
				theItem = promptForItemOfType((WEAPON | ARMOR | RING | STAFF | WAND), 0, 0,
											  "Enchant what? (a-z; shift for more info)", false);
				confirmMessages();
				if (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND))) {
					message("Can't enchant that.", true);
				}
				if (rogue.gameHasEnded) {
					return;
				}
			} while (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND)));
			recordKeystroke(theItem->inventoryLetter, false, false);
			confirmMessages();
			switch (theItem->category) {
				case WEAPON:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
					break;
				case ARMOR:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
					break;
				case RING:
					theItem->enchant1++;
					updateRingBonuses();
					if (theItem->kind == RING_CLAIRVOYANCE) {
						updateClairvoyance();
						displayLevel();
					}
					break;
				case STAFF:
					theItem->enchant1++;
					theItem->charges++;
					theItem->enchant2 = 500 / theItem->enchant1;
					break;
				case WAND:
					theItem->charges++;
					break;
				default:
					break;
			}
			if (theItem->flags & ITEM_EQUIPPED) {
				equipItem(theItem, true);
			}
			itemName(theItem, buf, false, false, NULL);
			sprintf(buf2, "your %s gleam%s in the darkness.", buf, (theItem->quantity == 1 ? "s" : ""));
			messageWithColor(buf2, &itemMessageColor, false);
			if (theItem->flags & ITEM_CURSED) {
				sprintf(buf2, "a malevolent force leaves your %s.", buf);
				messageWithColor(buf2, &itemMessageColor, false);
				theItem->flags &= ~ITEM_CURSED;
			}
			break;
		case SCROLL_RECHARGING:
			x = y = 0; // x counts staffs, y counts wands
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->category & STAFF) {
					x++;
					tempItem->charges = tempItem->enchant1;
					tempItem->enchant2 = 5000;
				}
				if (tempItem->category & WAND) {
					y++;
					tempItem->charges++;
				}
			}
			if (x || y) {
				sprintf(buf, "a surge of energy courses through your pack, recharging your %s%s%s.",
						(x > 1 ? "staffs" : (x == 1 ? "staff" : "")),
						(x && y ? " and " : ""),
						(y > 1 ? "wands" : (y == 1 ? "wand" : "")));
				message(buf, false);
			} else {
				message("a surge of energy courses through your pack, but nothing happens.", false);
			}
			break;
		case SCROLL_PROTECT_ARMOR:
			if (rogue.armor) {
				tempItem = rogue.armor;
				tempItem->flags |= ITEM_PROTECTED;
				itemName(tempItem, buf2, false, false, NULL);
				sprintf(buf, "a protective golden light covers your %s.", buf2);
				messageWithColor(buf, &itemMessageColor, false);
				if (tempItem->flags & ITEM_CURSED) {
					sprintf(buf, "a malevolent force leaves your %s.", buf2);
					messageWithColor(buf, &itemMessageColor, false);
					tempItem->flags &= ~ITEM_CURSED;
				}
			} else {
				message("a protective golden light surrounds you, but it quickly disperses.", false);
			}
			break;
		case SCROLL_PROTECT_WEAPON:
			if (rogue.weapon) {
				tempItem = rogue.weapon;
				tempItem->flags |= ITEM_PROTECTED;
				itemName(tempItem, buf2, false, false, NULL);
				sprintf(buf, "a protective golden light covers your %s.", buf2);
				messageWithColor(buf, &itemMessageColor, false);
				if (tempItem->flags & ITEM_CURSED) {
					sprintf(buf, "a malevolent force leaves your %s.", buf2);
					messageWithColor(buf, &itemMessageColor, false);
					tempItem->flags &= ~ITEM_CURSED;
				}
			} else {
				message("a protective golden light covers your empty hands, but it quickly disperses.", false);
			}
			break;
		case SCROLL_MAGIC_MAPPING:
			confirmMessages();
			messageWithColor("this scroll has a map on it!", &itemMessageColor, false);
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (!(pmap[i][j].flags & DISCOVERED) && pmap[i][j].layers[DUNGEON] != GRANITE) {
						pmap[i][j].flags |= MAGIC_MAPPED;
					}
				}
			}
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (cellHasTerrainFlag(i, j, T_IS_SECRET)) {
						discover(i, j);
						pmap[i][j].flags |= MAGIC_MAPPED;
						pmap[i][j].flags &= ~STABLE_MEMORY;
					}
				}
			}
			lightFlash(&magicMapFlashColor, 0, MAGIC_MAPPED, 15, DCOLS, player.xLoc, player.yLoc);
			break;
		case SCROLL_AGGRAVATE_MONSTER:
			aggravateMonsters();
			lightFlash(&gray, 0, (DISCOVERED | MAGIC_MAPPED), 10, DCOLS / 2, player.xLoc, player.yLoc);
			message("the scroll emits a piercing shriek that echoes throughout the dungeon!", false);
			break;
		case SCROLL_SUMMON_MONSTER:
			for (j=0; j<25 && numberOfMonsters < 3; j++) {
				for (i=0; i<8; i++) {
					x = player.xLoc + nbDirs[i][0];
					y = player.yLoc + nbDirs[i][1];
					if (!cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY) && !(pmap[x][y].flags & HAS_MONSTER)
						&& rand_percent(10) && (numberOfMonsters < 3)) {
						monst = spawnHorde(0, x, y, (HORDE_LEADER_CAPTIVE | HORDE_NO_PERIODIC_SPAWN | HORDE_IS_SUMMONED | HORDE_MACHINE_ONLY), 0);
						if (monst) {
							// refreshDungeonCell(x, y);
							// monst->creatureState = MONSTER_TRACKING_SCENT;
							// monst->ticksUntilTurn = player.movementSpeed;
							wakeUp(monst);
							fadeInMonster(monst);
							numberOfMonsters++;
						}
					}
				}
			}
			if (numberOfMonsters > 1) {
				message("the fabric of space ripples, and monsters appear!", false);
			} else if (numberOfMonsters == 1) {
				message("the fabric of space ripples, and a monster appears!", false);
			} else {
				message("the fabric of space boils violently around you, but nothing happens.", false);
			}
			break;
		case SCROLL_CAUSE_FEAR:
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW
					&& monst->creatureState != MONSTER_FLEEING
					&& !(monst->info.flags & MONST_INANIMATE)) {
					monst->status[STATUS_MAGICAL_FEAR] = monst->maxStatus[STATUS_MAGICAL_FEAR] = rand_range(150, 225);
					monst->creatureState = MONSTER_FLEEING;
					chooseNewWanderDestination(monst);
					if (canSeeMonster(monst)) {
						numberOfMonsters++;
						monsterName(buf2, monst, true);
					}
				}
			}
			if (numberOfMonsters > 1) {
				message("the scroll emits a brilliant flash of red light, and the monsters flee!", false);
			} else if (numberOfMonsters == 1) {
				sprintf(buf, "the scroll emits a brilliant flash of red light, and %s flees!", buf2);
				message(buf, false);
			} else {
				message("the scroll emits a brilliant flash of red light!", false);
			}
			lightFlash(&redFlashColor, 0, IN_FIELD_OF_VIEW, 15, DCOLS, player.xLoc, player.yLoc);
			break;
		case SCROLL_NEGATION:
			messageWithColor("the scroll emits a numbing torrent of anti-magic!", &itemMessageColor, false);
			lightFlash(&pink, 0, IN_FIELD_OF_VIEW, 15, DCOLS, player.xLoc, player.yLoc);
			negate(&player);
			flashMonster(&player, &pink, 100);
			for (monst = monsters->nextCreature; monst != NULL;) {
				nextMonst = monst->nextCreature;
				if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW) {
					if (canSeeMonster(monst)) {
						flashMonster(monst, &pink, 100);
					}
					negate(monst); // This can be fatal.
				}
				monst = nextMonst;
			}
			for (theItem = floorItems; theItem != NULL; theItem = theItem->nextItem) {
				if (pmap[theItem->xLoc][theItem->yLoc].flags & IN_FIELD_OF_VIEW) {
					theItem->flags &= ~(ITEM_MAGIC_DETECTED | ITEM_CURSED);
					switch (theItem->category) {
						case WEAPON:
						case ARMOR:
							hadEffect = true;
							theItem->enchant1 = theItem->enchant2 = theItem->charges = 0;
							theItem->flags &= ~(ITEM_RUNIC | ITEM_RUNIC_HINTED | ITEM_RUNIC_IDENTIFIED | ITEM_PROTECTED);
							identify(theItem);
							break;
						case STAFF:
							hadEffect = true;
							theItem->charges = 0;
							break;
						case WAND:
							hadEffect = true;
							theItem->charges = 0;
							theItem->flags |= ITEM_MAX_CHARGES_KNOWN;
							break;
						case RING:
							hadEffect = true;
							theItem->enchant1 = 0;
							theItem->flags |= ITEM_IDENTIFIED; // Reveal that it is (now) +0, but not necessarily which kind of ring it is.
							updateIdentifiableItems();
						default:
							break;
					}
				}
			}
			break;
		case SCROLL_SHATTERING:
			messageWithColor("the scroll emits a wave of turquoise light that pierces the nearby walls!", &itemMessageColor, false);
			crystalize(9);
			break;
	}
}

void drinkPotion(item *theItem) {
	item *tempItem;
	creature *monst;
	boolean hadEffect = false;
	boolean hadEffect2 = false;
	
	switch (theItem->kind) {
		case POTION_HEALING:
			if (player.status[STATUS_HALLUCINATING] > 1) {
				player.status[STATUS_HALLUCINATING] = 1;
			}
			if (player.status[STATUS_CONFUSED] > 1) {
				player.status[STATUS_CONFUSED] = 1;
			}
			if (player.status[STATUS_NAUSEOUS] > 1) {
				player.status[STATUS_NAUSEOUS] = 1;
			}
			if (player.status[STATUS_SLOWED] > 1) {
				player.status[STATUS_SLOWED] = 1;
			}
			if (player.status[STATUS_WEAKENED] > 1) {
				player.status[STATUS_WEAKENED] = 1;
			}
			if (player.status[STATUS_POISONED]) {
				player.status[STATUS_POISONED] = 0;
			}
			if (player.status[STATUS_DARKNESS] > 0) {
				player.status[STATUS_DARKNESS] = 0;
				updateMinersLightRadius();
				updateVision(true);
			}
			if (player.currentHP < player.info.maxHP) {
				player.currentHP = player.info.maxHP;
				message("your wounds heal completely.", false);
			} else {
				message("you feel extremely healthy.", false);
			}
			break;
		case POTION_HALLUCINATION:
			player.status[STATUS_HALLUCINATING] = player.maxStatus[STATUS_HALLUCINATING] = 300;
			message("colors are everywhere! The walls are singing!", false);
			break;
		case POTION_INCINERATION:
			lightFlash(&darkOrange, 0, IN_FIELD_OF_VIEW, 4, 4, player.xLoc, player.yLoc);
			message("as you uncork the flask, it explodes in flame!", false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_FLAMETHROWER], true, false);
			exposeCreatureToFire(&player);
			break;
		case POTION_DARKNESS:
			player.status[STATUS_DARKNESS] = max(400, player.status[STATUS_DARKNESS]);
			player.maxStatus[STATUS_DARKNESS] = max(400, player.maxStatus[STATUS_DARKNESS]);
			updateMinersLightRadius();
			updateVision(true);
			message("your vision flickers as a cloak of darkness settles around you!", false);
			break;
		case POTION_DESCENT:
			lightFlash(&darkBlue, 0, IN_FIELD_OF_VIEW, 3, 3, player.xLoc, player.yLoc);
			message("vapor pours out of the flask and causes the floor to disappear!", false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_HOLE_POTION], true, false);
			break;
		case POTION_GAIN_LEVEL:
			addExperience(levelPoints[rogue.experienceLevel - 1] - rogue.experience);
			break;
		case POTION_GAIN_STRENGTH:
			rogue.strength++;
			if (player.status[STATUS_WEAKENED]) {
				player.status[STATUS_WEAKENED] = 1;
			}
			updateEncumbrance();
			messageWithColor("newfound strength surges through your body.", &advancementMessageColor, false);
			break;
		case POTION_POISON:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_POISON_GAS_CLOUD_POTION], true, false);
			message("poisonous gas billows out of the open flask!", false);
			break;
		case POTION_PARALYSIS:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_PARALYSIS_GAS_CLOUD_POTION], true, false);
			message("your muscles stiffen as a cloud of pink gas bursts from the open flask!", false);
			break;
		case POTION_TELEPATHY:
			player.status[STATUS_TELEPATHIC] = player.maxStatus[STATUS_TELEPATHIC] = 300;
			for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
			if (monsters->nextCreature == NULL) {
				message("you can somehow tell that you are alone on this level at the moment.", false);
			} else {
				message("you can somehow feel the presence of other creatures' minds!", false);
			}
			break;
		case POTION_LEVITATION:
			player.status[STATUS_LEVITATING] = player.maxStatus[STATUS_LEVITATING] = 100;
			player.bookkeepingFlags &= ~MONST_SEIZED; // break free of holding monsters
			message("you float into the air!", false);
			break;
		case POTION_CONFUSION:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_CONFUSION_GAS_CLOUD_POTION], true, false);
			message("a shimmering cloud of rainbow-colored gas billows out of the open flask!", false);
			break;
		case POTION_LICHEN:
			message("a handful of tiny spores burst out of the open flask!", false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_LICHEN_PLANTED], true, false);
			break;
		case POTION_DETECT_MAGIC:
			hadEffect = false;
			hadEffect2 = false;
			for (tempItem = floorItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->category & CAN_BE_DETECTED) {
					tempItem->flags |= ITEM_MAGIC_DETECTED;
					if (itemMagicChar(tempItem)) {
						pmap[tempItem->xLoc][tempItem->yLoc].flags |= ITEM_DETECTED;
						hadEffect = true;
						refreshDungeonCell(tempItem->xLoc, tempItem->yLoc);
					}
				}
			}
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				if (monst->carriedItem && (monst->carriedItem->category & CAN_BE_DETECTED)) {
					monst->carriedItem->flags |= ITEM_MAGIC_DETECTED;
					if (itemMagicChar(monst->carriedItem)) {
						hadEffect = true;
						refreshDungeonCell(monst->xLoc, monst->yLoc);
					}
				}
			}
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->category & CAN_BE_DETECTED) {
					tempItem->flags |= ITEM_MAGIC_DETECTED;
					if (itemMagicChar(tempItem)) {
						if (tempItem->flags & ITEM_MAGIC_DETECTED) {
							hadEffect2 = true;
						}
					}
				}
			}
			if (hadEffect || hadEffect2) {
				if (hadEffect && hadEffect2) {
					message("you can somehow feel the presence of magic on the level and in your pack.", false);
				} else if (hadEffect) {
					message("you can somehow feel the presence of magic on the level.", false);
				} else {
					message("you can somehow feel the presence of magic in your pack.", false);
				}
			} else {
				message("you can somehow feel the absence of magic on the level and in your pack.", false);
			}
			break;
		case POTION_HASTE_SELF:
			haste(&player, 25);
			break;
		case POTION_FIRE_IMMUNITY:
			player.status[STATUS_IMMUNE_TO_FIRE] = player.maxStatus[STATUS_IMMUNE_TO_FIRE] = 150;
			//player.info.flags |= MONST_IMMUNE_TO_FIRE;
			if (player.status[STATUS_BURNING]) {
				extinguishFireOnCreature(&player);
			}
			message("you no longer fear fire.", false);
			break;
		default:
			message("you feel very strange, as though your body doesn't know how to react!", true);
	}
}

// Used for the Discoveries screen. Returns a number: 1 == good, -1 == bad, 0 == could go either way.
short magicCharDiscoverySuffix(short category, short kind) {
	short result = 0;
	
	switch (category) {
		case SCROLL:
			switch (kind) {
				case SCROLL_AGGRAVATE_MONSTER:
				case SCROLL_SUMMON_MONSTER:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case POTION:
			switch (kind) {
				case POTION_HALLUCINATION:
				case POTION_INCINERATION:
				case POTION_DESCENT:
				case POTION_POISON:
				case POTION_PARALYSIS:
				case POTION_CONFUSION:
				case POTION_LICHEN:
				case POTION_DARKNESS:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case WAND:
			switch (kind) {
				case WAND_PLENTY:
				case WAND_INVISIBILITY:
				case WAND_BECKONING:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case STAFF:
			switch (kind) {
				case STAFF_HEALING:
				case STAFF_HASTE:
				case STAFF_PROTECTION:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case RING:
			result = 0;
			break;
	}
	return result;
}

uchar itemMagicChar(item *theItem) {
	switch (theItem->category) {
		case WEAPON:
		case ARMOR:
			if ((theItem->flags & ITEM_CURSED) || theItem->enchant1 < 0) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			}
			return 0;
			break;
		case SCROLL:
			switch (theItem->kind) {
				case SCROLL_AGGRAVATE_MONSTER:
				case SCROLL_SUMMON_MONSTER:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case POTION:
			switch (theItem->kind) {
				case POTION_HALLUCINATION:
				case POTION_INCINERATION:
				case POTION_DESCENT:
				case POTION_POISON:
				case POTION_PARALYSIS:
				case POTION_CONFUSION:
				case POTION_LICHEN:
				case POTION_DARKNESS:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case WAND:
			if (theItem->charges == 0) {
				return 0;
			}
			switch (theItem->kind) {
				case WAND_PLENTY:
				case WAND_INVISIBILITY:
				case WAND_BECKONING:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case STAFF:
			switch (theItem->kind) {
				case STAFF_HEALING:
				case STAFF_HASTE:
				case STAFF_PROTECTION:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case RING:
			if (theItem->flags & ITEM_CURSED || theItem->enchant1 < 0) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			} else {
				return 0;
			}
		case AMULET:
			return AMULET_CHAR;
	}
	return 0;
}

void unequip(item *theItem) {
	char buf[COLS], buf2[COLS];
	unsigned char command[3];
	
	command[0] = UNEQUIP_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType(ALL_ITEMS, ITEM_EQUIPPED, 0,
									  "Remove (unequip) what? (a-z or <esc> to cancel)", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	command[1] = theItem->inventoryLetter;
	command[2] = '\0';
	
	if (!(theItem->flags & ITEM_EQUIPPED)) {
		itemName(theItem, buf2, false, false, NULL);
		sprintf(buf, "your %s was not equipped.", buf2);
		confirmMessages();
		messageWithColor(buf, &itemMessageColor, false);
		return;
	} else if (theItem->flags & ITEM_CURSED) { // this is where the item gets unequipped
		itemName(theItem, buf2, false, false, NULL);
		sprintf(buf, "you can't; your %s appears to be cursed.", buf2);
		confirmMessages();
		messageWithColor(buf, &itemMessageColor, false);
		return;
	} else {
		recordKeystrokeSequence(command);
		unequipItem(theItem, false);
		if (theItem->category & RING) {
			updateRingBonuses();
		}
		itemName(theItem, buf2, true, true, NULL);
		if (strLenWithoutEscapes(buf2) > 52) {
			itemName(theItem, buf2, false, true, NULL);
		}
		confirmMessages();
		updateEncumbrance();
		sprintf(buf, "you are no longer %s %s.", (theItem->category & WEAPON ? "wielding" : "wearing"), buf2);
		messageWithColor(buf, &itemMessageColor, false);
	}
	playerTurnEnded();
}

boolean canDrop() {
	item *theItem;
	
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_ITEMS)) {
		return false;
	}
	
	theItem = itemAtLoc(player.xLoc, player.yLoc);
	if (theItem && (theItem->flags & ITEM_NO_PICKUP)) {
		return false;
	}
	return true;
}

void drop(item *theItem) {
	char buf[COLS], buf2[COLS];
	unsigned char command[3];
	
	command[0] = DROP_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType(ALL_ITEMS, 0, 0,
									  "Drop what? (a-z, shift for more info; or <esc> to cancel)", true);
	}
	if (theItem == NULL) {
		return;
	}
	command[1] = theItem->inventoryLetter;
	command[2] = '\0';
	
	if ((theItem->flags & ITEM_EQUIPPED) && (theItem->flags & ITEM_CURSED)) {
		itemName(theItem, buf2, false, false, NULL);
		sprintf(buf, "you can't; your %s appears to be cursed.", buf2);
		confirmMessages();
		messageWithColor(buf, &itemMessageColor, false);
	} else if (canDrop()) {
		recordKeystrokeSequence(command);
		if (theItem->flags & ITEM_EQUIPPED) {
			unequipItem(theItem, false);
		}
		theItem = dropItem(theItem); // This is where it gets dropped.
		theItem->flags |= ITEM_PLAYER_AVOIDS; // Try not to pick up stuff you've already dropped.
		itemName(theItem, buf2, true, true, NULL);
		sprintf(buf, "You dropped %s.", buf2);
		messageWithColor(buf, &itemMessageColor, false);
		playerTurnEnded();
	} else {
		confirmMessages();
		message("There is already something there.", false);
	}
}

item *promptForItemOfType(unsigned short category,
						  unsigned long requiredFlags,
						  unsigned long forbiddenFlags,
						  char *prompt,
						  boolean allowInventoryActions) {
	char keystroke;
	item *theItem;
	
	if (!numberOfMatchingPackItems(ALL_ITEMS, requiredFlags, forbiddenFlags, true)) {
		return NULL;
	}
	
	temporaryMessage(prompt, false);
	
	keystroke = displayInventory(category, requiredFlags, forbiddenFlags, false, allowInventoryActions);
	
	if (!keystroke) {
		// This can happen if the player does an action with an item directly from the inventory screen via a button.
		return NULL;
	}
	
	if (keystroke < 'a' || keystroke > 'z') {
		confirmMessages();
		if (keystroke != ESCAPE_KEY && keystroke != ACKNOWLEDGE_KEY) {
			message("Invalid entry.", false);
		}
		return NULL;
	}
	
	theItem = itemOfPackLetter(keystroke);
	if (theItem == NULL) {
		confirmMessages();
		message("No such item.", false);
		return NULL;
	}
	
	return theItem;
}

item *itemOfPackLetter(char letter) {
	item *theItem;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->inventoryLetter == letter) {
			return theItem;
		}
	}
	return NULL;
}

item *itemAtLoc(short x, short y) {
	item *theItem;
	
	if (!(pmap[x][y].flags & HAS_ITEM)) {
		return NULL; // easy optimization
	}
	for (theItem = floorItems->nextItem; theItem != NULL && (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
	if (theItem == NULL) {
		pmap[x][y].flags &= ~HAS_ITEM;
		hiliteCell(x, y, &white, 75, true);
		rogue.automationActive = false;
		message("ERROR: An item was supposed to be here, but I couldn't find it.", true);
		refreshDungeonCell(x, y);
	}
	return theItem;
}

item *dropItem(item *theItem) {
	item *itemFromTopOfStack, *itemOnFloor;
	
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_ITEMS)) {
		return NULL;
	}
	
	itemOnFloor = itemAtLoc(player.xLoc, player.yLoc);
	if (itemOnFloor && (itemOnFloor->flags & ITEM_NO_PICKUP)) {
		return NULL;
	}
	
	if (theItem->quantity > 1 && !(theItem->category & WEAPON)) { // peel off the top item and drop it
		itemFromTopOfStack = generateItem(ALL_ITEMS, -1);
		*itemFromTopOfStack = *theItem; // clone the item
		theItem->quantity--;
		itemFromTopOfStack->quantity = 1;
		if (itemOnFloor) {
			itemOnFloor->inventoryLetter = theItem->inventoryLetter; // just in case all letters are taken
			pickUpItemAt(player.xLoc, player.yLoc);
		}
		placeItem(itemFromTopOfStack, player.xLoc, player.yLoc);
		return itemFromTopOfStack;
	} else { // drop the entire item
		removeItemFromChain(theItem, packItems);
		if (itemOnFloor) {
			itemOnFloor->inventoryLetter = theItem->inventoryLetter;
			pickUpItemAt(player.xLoc, player.yLoc);
		}
		placeItem(theItem, player.xLoc, player.yLoc);
		return theItem;
	}
}

void recalculateEquipmentBonuses() {
	float enchant;
	item *theItem;
	
	if (rogue.weapon) {
		theItem = rogue.weapon;
		enchant = netEnchant(theItem);
		player.info.damage = theItem->damage;
		player.info.damage.lowerBound *= pow(WEAPON_ENCHANT_FACTOR, enchant);
		player.info.damage.upperBound *= pow(WEAPON_ENCHANT_FACTOR, enchant);
		if (player.info.damage.lowerBound < 1) {
			player.info.damage.lowerBound = 1;
		}
		if (player.info.damage.upperBound < 1) {
			player.info.damage.upperBound = 1;
		}
	}
	
	if (rogue.armor) {
		theItem = rogue.armor;
		enchant = netEnchant(theItem);
		player.info.defense = theItem->armor + enchant * 10;
		if (player.info.defense < 0) {
			player.info.defense = 0;
		}
	}
}

void equipItem(item *theItem, boolean force) {
	item *previouslyEquippedItem = NULL;
	
	if ((theItem->category & RING) && (theItem->flags & ITEM_EQUIPPED)) {
		return;
	}
	
	if (theItem->category & WEAPON) {
		previouslyEquippedItem = rogue.weapon;
	} else if (theItem->category & ARMOR) {
		previouslyEquippedItem = rogue.armor;
	}
	if (previouslyEquippedItem) {
		if (!force && (previouslyEquippedItem->flags & ITEM_CURSED)) {
			return; // already using a cursed item
		} else {
			unequipItem(previouslyEquippedItem, force);
		}
	}
	if (theItem->category & WEAPON) {
		rogue.weapon = theItem;
		recalculateEquipmentBonuses();
	} else if (theItem->category & ARMOR) {
		rogue.armor = theItem;
		recalculateEquipmentBonuses();
	} else if (theItem->category & RING) {
		if (rogue.ringLeft && rogue.ringRight) {
			return;
		}
		if (rogue.ringLeft) {
			rogue.ringRight = theItem;
		} else {
			rogue.ringLeft = theItem;
		}
		updateRingBonuses();
		if (theItem->kind == RING_CLAIRVOYANCE) {
			updateClairvoyance();
			displayLevel();
			ringTable[RING_CLAIRVOYANCE].identified = true;
		} else if (theItem->kind == RING_LIGHT) {
			ringTable[RING_LIGHT].identified = true;
		}
	}
	theItem->flags |= ITEM_EQUIPPED;
	return;
}

void unequipItem(item *theItem, boolean force) {
	
	if (theItem == NULL || !(theItem->flags & ITEM_EQUIPPED)) {
		return;
	}
	if ((theItem->flags & ITEM_CURSED) && !force) {
		return;
	}
	theItem->flags &= ~ITEM_EQUIPPED;
	if (theItem->category & WEAPON) {
		player.info.damage.lowerBound = 1;
		player.info.damage.upperBound = 2;
		player.info.damage.clumpFactor = 1;
		rogue.weapon = NULL;
	}
	if (theItem->category & ARMOR) {
		player.info.defense = 0;
		rogue.armor = NULL;
	}
	if (theItem->category & RING) {
		if (rogue.ringLeft == theItem) {
			rogue.ringLeft = NULL;
		} else if (rogue.ringRight == theItem) {
			rogue.ringRight = NULL;
		}
		updateRingBonuses();
		if (theItem->kind == RING_CLAIRVOYANCE) {
			updateClairvoyance();
			updateClairvoyance(); // Yes, we have to call this twice.
			displayLevel();
		}
	}
	updateEncumbrance();
	return;
}

void updateRingBonuses() {
	short i;
	item *rings[2] = {rogue.ringLeft, rogue.ringRight};
	
	rogue.clairvoyance = rogue.aggravating = rogue.stealthBonus = rogue.transference
	= rogue.awarenessBonus = rogue.regenerationBonus = rogue.wisdomBonus = 0;
	rogue.lightMultiplier = 1;
	
	for (i=0; i<= 1; i++) {
		if (rings[i]) {
			switch (rings[i]->kind) {
				case RING_CLAIRVOYANCE:
					rogue.clairvoyance += rings[i]->enchant1;
					break;
				case RING_STEALTH:
					rogue.stealthBonus += rings[i]->enchant1;
					break;
				case RING_REGENERATION:
					rogue.regenerationBonus += rings[i]->enchant1;
					break;
				case RING_TRANSFERENCE:
					rogue.transference += rings[i]->enchant1;
					break;
				case RING_LIGHT:
					rogue.lightMultiplier += rings[i]->enchant1;
					break;
				case RING_AWARENESS:
					rogue.awarenessBonus += 20 * rings[i]->enchant1;
					break;
				case RING_WISDOM:
					rogue.wisdomBonus += rings[i]->enchant1;
					break;
			}
		}
	}
	
	if (rogue.lightMultiplier <= 0) {
		rogue.lightMultiplier--; // because it starts at positive 1 instead of 0
	}
	
	updateMinersLightRadius();
	updatePlayerRegenerationDelay();
	
	if (rogue.stealthBonus < 0) {
		rogue.stealthBonus *= 4;
		rogue.aggravating = true;
	}
	
	if (rogue.aggravating) {
		aggravateMonsters();
	}
}

void updatePlayerRegenerationDelay() {
	short maxHP;
	long turnsForFull; // In thousandths of a turn.
	maxHP = player.info.maxHP;
	turnsForFull = turnsForFullRegen(rogue.regenerationBonus);
	
	player.regenPerTurn = 0;
	while (maxHP > turnsForFull / 1000) {
		player.regenPerTurn++;
		maxHP -= turnsForFull / 1000;
	}
	
	player.info.turnsBetweenRegen = (turnsForFull / maxHP);
	// DEBUG printf("\nTurnsForFull: %i; regenPerTurn: %i; (thousandths of) turnsBetweenRegen: %i", turnsForFull, player.regenPerTurn, player.info.turnsBetweenRegen);
}

boolean removeItemFromChain(item *theItem, item *theChain) {
	item *previousItem;
	
	for (previousItem = theChain;
		 previousItem->nextItem;
		 previousItem = previousItem->nextItem) {
		if (previousItem->nextItem == theItem) {
			previousItem->nextItem = theItem->nextItem;
			return true;
		}
	}
	return false;
}

void deleteItem(item *theItem) {
	free(theItem);
}

void resetItemTableEntry(itemTable *theEntry) {
	theEntry->identified = false;
	theEntry->called = false;
	theEntry->callTitle[0] = '\0';
}

void shuffleFlavors() {
	short i, j, randIndex, randNumber;
	char buf[COLS];
	
	//	for (i=0; i<NUMBER_FOOD_KINDS; i++) {
	//		resetItemTableEntry(foodTable + i);
	//	}
	for (i=0; i<NUMBER_POTION_KINDS; i++) {
		resetItemTableEntry(potionTable + i);
	}
	//	for (i=0; i<NUMBER_WEAPON_KINDS; i++) {
	//		resetItemTableEntry(weaponTable + i);
	//	}
	//	for (i=0; i<NUMBER_ARMOR_KINDS; i++) {
	//		resetItemTableEntry(armorTable + i);
	//	}
	for (i=0; i<NUMBER_STAFF_KINDS; i++) {
		resetItemTableEntry(staffTable+ i);
	}
	for (i=0; i<NUMBER_WAND_KINDS; i++) {
		resetItemTableEntry(wandTable + i);
	}
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		resetItemTableEntry(scrollTable + i);
	}
	for (i=0; i<NUMBER_RING_KINDS; i++) {
		resetItemTableEntry(ringTable + i);
	}
	
	for (i=0; i<NUMBER_ITEM_COLORS; i++) {
		strcpy(itemColors[i], itemColorsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_COLORS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_COLORS - 1);
		strcpy(buf, itemColors[i]);
		strcpy(itemColors[i], itemColors[randIndex]);
		strcpy(itemColors[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		strcpy(itemWoods[i], itemWoodsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_WOODS - 1);
		strcpy(buf, itemWoods[i]);
		strcpy(itemWoods[i], itemWoods[randIndex]);
		strcpy(itemWoods[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		strcpy(itemGems[i], itemGemsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_GEMS - 1);
		strcpy(buf, itemGems[i]);
		strcpy(itemGems[i], itemGems[randIndex]);
		strcpy(itemGems[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_METALS; i++) {
		strcpy(itemMetals[i], itemMetalsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_METALS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_METALS - 1);
		strcpy(buf, itemMetals[i]);
		strcpy(itemMetals[i], itemMetals[randIndex]);
		strcpy(itemMetals[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		itemTitles[i][0] = '\0';
		randNumber = rand_range(3, 4);
		for (j=0; j<randNumber; j++) {
			randIndex = rand_range(0, NUMBER_TITLE_PHONEMES - 1);
			strcpy(buf, itemTitles[i]);
			sprintf(itemTitles[i], "%s%s%s", buf, ((rand_percent(50) && j>0) ? " " : ""), titlePhonemes[randIndex]);
		}
	}
}

unsigned long itemValue(item *theItem) {
	const short weaponRunicIntercepts[] = {
		500,	//W_SPEED,
		1000,	//W_QUIETUS,
		900,	//W_PARALYSIS,
		1000,	//W_MULTIPLICITY,
		700,	//W_SLOWING,
		750,	//W_CONFUSION,
		500,	//W_SLAYING,
		-1000,	//W_MERCY,
		-1000,	//W_PLENTY,
	};
	const short armorRunicIntercepts[] = {
		900,	//A_MULTIPLICITY,
		700,	//A_MUTUALITY,
		900,	//A_ABSORPTION,
		900,	//A_REPRISAL,
		500,	//A_IMMUNITY,
		900,	//A_REFLECTION,
		-1000,	//A_BURDEN,
		-1000,	//A_VULNERABILITY,
        -1000,  //A_IMMOLATION,
	};
	
	signed long value;
	
	switch (theItem->category) {
		case FOOD:
			return foodTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case WEAPON:
			value = (signed long) (weaponTable[theItem->kind].marketValue * theItem->quantity
								   * (1 + 0.15 * (theItem->enchant1 + (theItem->flags & ITEM_PROTECTED ? 1 : 0))));
			if (theItem->flags & ITEM_RUNIC) {
				value += weaponRunicIntercepts[theItem->enchant2];
				if (value < 0) {
					value = 0;
				}
			}
			return (unsigned long) value;
			break;
		case ARMOR:
			value = (signed long) (armorTable[theItem->kind].marketValue * theItem->quantity
								   * (1 + 0.15 * (theItem->enchant1 + ((theItem->flags & ITEM_PROTECTED) ? 1 : 0))));
			if (theItem->flags & ITEM_RUNIC) {
				value += armorRunicIntercepts[theItem->enchant2];
				if (value < 0) {
					value = 0;
				}
			}
			return (unsigned long) value;
			break;
		case SCROLL:
			return scrollTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case POTION:
			return potionTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case STAFF:
			return staffTable[theItem->kind].marketValue * theItem->quantity
			* (unsigned long) (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case WAND:
			return wandTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case RING:
			return ringTable[theItem->kind].marketValue * theItem->quantity
			* (unsigned long) (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case AMULET:
			return 10000;
			break;
		case GEM:
			return 5000 * theItem->quantity;
			break;
		case KEY:
			return 0;
			break;
		default:
			return 0;
			break;
	}
}
