/*
 *  RogueMain.c
 *  Brogue
 *
 *  Created by Brian Walker on 12/26/08.
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
#include <time.h>

void rogueMain() {
	previousGameSeed = 0;
	initializeBrogueSaveLocation();
	mainBrogueJunction();
}

void executeEvent(rogueEvent *theEvent) {
	rogue.playbackBetweenTurns = false;
	if (theEvent->eventType == KEYSTROKE) {
		executeKeystroke(theEvent->param1, theEvent->controlKey, theEvent->shiftKey);
	} else if (theEvent->eventType == MOUSE_UP
			   || theEvent->eventType == RIGHT_MOUSE_UP) {
		executeMouseClick(theEvent);
	}
}

boolean fileExists(const char *pathname) {
	FILE *openedFile;
	openedFile = fopen(pathname, "rb");
	if (openedFile) {
		fclose(openedFile);
		return true;
	} else {
		return false;
	}
}

// Player specifies a file; if all goes well, put it into path and return true.
// Otherwise, return false.
boolean chooseFile(char *path, char *prompt, char *defaultName, char *suffix) {
	
	if (getInputTextString(path,
						   prompt,
						   min(DCOLS-25, BROGUE_FILENAME_MAX - strlen(suffix)),
						   defaultName,
						   suffix,
						   TEXT_INPUT_NORMAL,
						   false)
		&& path[0] != '\0') {
		
		strcat(path, suffix);
		return true;
	} else {
		return false;
	}
}

// If the file exists, copy it into currentFilePath. (Otherwise return false.)
// Then, strip off the suffix, replace it with ANNOTATION_SUFFIX,
// and if that file exists, copy that into annotationPathname. Return true.
boolean openFile(const char *path) {
	short i;
	char buf[BROGUE_FILENAME_MAX];
	boolean retval;
	
	if (fileExists(path)) {
		
		strcpy(currentFilePath, path);
		annotationPathname[0] = '\0';
		
		// Clip off the suffix.
		strcpy(buf, path);
		for (i = strlen(path); buf[i] != '.' && i > 0; i--) continue;
		if (buf[i] == '.'
			&& i + strlen(ANNOTATION_SUFFIX) < BROGUE_FILENAME_MAX) {
			
			buf[i] = '\0'; // Snip!
			strcat(buf, ANNOTATION_SUFFIX);
			strcpy(annotationPathname, buf); // Load the annotations file too.
		}
		retval = true;
	} else {
		retval = false;
	}
	
	return retval;
}

// Deprecated!
boolean selectFile(char *prompt, char *defaultName, char *suffix) {
	boolean retval;
	char newFilePath[BROGUE_FILENAME_MAX];
	
	retval = false;

	if (chooseFile(newFilePath, prompt, defaultName, suffix)) {
		if (openFile(newFilePath)) {
			retval = true;
		} else {
			confirmMessages();
			message("File not found.", false);
			retval = false;
		}
	}
	return retval;
}

void welcome() {
    char buf[DCOLS*3];
	message("Hello and welcome, adventurer, to the Dungeons of Doom!", false);
    strcpy(buf, "Retrieve the ");
    encodeMessageColor(buf, strlen(buf), &itemMessageColor);
    strcat(buf, "Amulet of Yendor");
    encodeMessageColor(buf, strlen(buf), &white);
    strcat(buf, " from the 26th floor and escape with it!");
	message(buf, false);
	messageWithColor("Press <?> for help at any time.", &backgroundMessageColor, false);
	flavorMessage("The doors to the dungeon slam shut behind you.");
}

// Seed is used as the dungeon seed unless it's zero, in which case generate a new one.
// Either way, previousGameSeed is set to the seed we use.
// None of this seed stuff is applicable if we're playing a recording.
void initializeRogue(unsigned long seed) {
	short i, j;
	item *theItem;
	uchar k;
	boolean playingback, playbackFF, playbackPaused;
	
	// generate libtcod font bitmap
	// add any new unicode characters here to include them
#ifdef GENERATE_FONT_FILES
	uchar c8[16] = {
		FLOOR_CHAR,
		CHASM_CHAR,
		TRAP_CHAR,
		FIRE_CHAR,
		FOLIAGE_CHAR,
		AMULET_CHAR,
		SCROLL_CHAR,
		RING_CHAR,
		WEAPON_CHAR,
		GEM_CHAR,
		TOTEM_CHAR,
		TURRET_CHAR,
		BAD_MAGIC_CHAR,
		GOOD_MAGIC_CHAR,
		' ',
		' ',
	};
	uchar c9[16] = {
		UP_ARROW_CHAR,
		DOWN_ARROW_CHAR,
		LEFT_ARROW_CHAR,
		RIGHT_ARROW_CHAR,
		UP_TRIANGLE_CHAR,
		DOWN_TRIANGLE_CHAR,
		OMEGA_CHAR,
		THETA_CHAR,
		LAMDA_CHAR,
		KOPPA_CHAR,
		LOZENGE_CHAR,
		CROSS_PRODUCT_CHAR,
		' ',
		' ',
		' ',
		' ',
	};
	
	for (i=0; i<COLS; i++) {
		for(j=0; j<ROWS; j++ ) {
			plotCharWithColor(' ', i, j, white, white);
		}
	}
	i = j = 0;
	for (k=0; k<256; k++) {
		i = k % 16;
		j = k / 16;
		if (j >= ROWS) {
			break;
		}
		if (j == 8) {
			plotCharWithColor(c8[i], i, j+5, white, black);
		} else if (j == 9) {
			plotCharWithColor(c9[i], i, j+5, white, black);
		} else {
			plotCharWithColor(k, i, j+5, white, black);
		}
	}
	for (;;) {
		waitForAcknowledgment();
	}
#endif
	
	playingback = rogue.playbackMode; // the only three animals that need to go on the ark
	playbackPaused = rogue.playbackPaused;
	playbackFF = rogue.playbackFastForward;
	memset((void *) &rogue, 0, sizeof( playerCharacter )); // the flood
	rogue.playbackMode = playingback;
	rogue.playbackPaused = playbackPaused;
	rogue.playbackFastForward = playbackFF;
	
	rogue.gameHasEnded = false;
	rogue.highScoreSaved = false;
	rogue.cautiousMode = false;
	rogue.milliseconds = 0;
	
	rogue.RNG = RNG_SUBSTANTIVE;
	if (!rogue.playbackMode) {
		rogue.seed = seedRandomGenerator(seed);
		previousGameSeed = rogue.seed;
	}
	initRecording();
	
	levels[0].upStairsLoc[0] = (DCOLS - 1) / 2 - 1;
	levels[0].upStairsLoc[1] = DROWS - 2;
	
	// reset enchant and gain strength frequencies
	rogue.strengthPotionFrequency = 40;
	rogue.enchantScrollFrequency = 60;
	
	// all DF messages are eligible for display
	resetDFMessageEligibility();
	
	// initialize the levels list
	for (i=0; i<101; i++) {
		levels[i].levelSeed = (unsigned long) rand_range(0, 9999) + 10000 * rand_range(0, 9999);
		levels[i].monsters = NULL;
		levels[i].dormantMonsters = NULL;
		levels[i].items = NULL;
		levels[i].roomStorage = NULL;
		levels[i].visited = false;
		levels[i].numberOfRooms = 0;
		levels[i].playerExitedVia[0] = 0;
		levels[i].playerExitedVia[1] = 0;
		do {
			levels[i].downStairsLoc[0] = rand_range(1, DCOLS - 2);
			levels[i].downStairsLoc[1] = rand_range(1, DROWS - 2);
		} while (distanceBetween(levels[i].upStairsLoc[0], levels[i].upStairsLoc[1],
								 levels[i].downStairsLoc[0], levels[i].downStairsLoc[1]) < DCOLS / 3);
		if (i < 100) {
			levels[i+1].upStairsLoc[0] = levels[i].downStairsLoc[0];
			levels[i+1].upStairsLoc[1] = levels[i].downStairsLoc[1];
		}
	}
	
	rogue.rewardRoomsGenerated = 0;
	
	// pre-shuffle the random terrain colors
	assureCosmeticRNG;
	for (i=0; i<DCOLS; i++) {
		for( j=0; j<DROWS; j++ ) {
			for (k=0; k<8; k++) {
				terrainRandomValues[i][j][k] = rand_range(0, 1000);
			}
		}
	}
	restoreRNG;
	
	zeroOutGrid(displayDetail);
	
	for (i=0; i<NUMBER_MONSTER_KINDS; i++) {
		monsterCatalog[i].monsterID = i;
	}
	
	shuffleFlavors();
	
	deleteMessages();
	for (i = 0; i < MESSAGE_ARCHIVE_LINES; i++) { // Clear the message archive.
		messageArchive[i][0] = '\0';
	}
	messageArchivePosition = 0;
	
	// Seed the stacks.
	floorItems = (item *) malloc(sizeof(item));
	memset(floorItems, '\0', sizeof(item));
	floorItems->nextItem = NULL;
	packItems = (item *) malloc(sizeof(item));
	memset(packItems, '\0', sizeof(item));
	packItems->nextItem = NULL;
	monsters = (creature *) malloc(sizeof(creature));
	memset(monsters, '\0', sizeof(creature));
	dormantMonsters = (creature *) malloc(sizeof(creature));
	memset(dormantMonsters, '\0', sizeof(creature));
	monsters->nextCreature = NULL;
	graveyard = (creature *) malloc(sizeof(creature));
	memset(graveyard, '\0', sizeof(creature));
	graveyard->nextCreature = NULL;
	rooms = (room *) malloc(sizeof(room));
	memset(rooms, '\0', sizeof(room));
	rooms->nextRoom = NULL;
	
	scentMap			= allocDynamicGrid();
	safetyMap			= allocDynamicGrid();
	allySafetyMap		= allocDynamicGrid();
	chokeMap			= allocDynamicGrid();
	playerPathingMap	= allocDynamicGrid();
	
	rogue.mapToSafeTerrain = allocDynamicGrid();
	
	// Zero out the dynamic grids, as an essential safeguard against OOSes:
	fillDynamicGrid(scentMap, 0);
	fillDynamicGrid(safetyMap, 0);
	fillDynamicGrid(allySafetyMap, 0);
	fillDynamicGrid(chokeMap, 0);
	fillDynamicGrid(playerPathingMap, 0);
	fillDynamicGrid(rogue.mapToSafeTerrain, 0);
	
	// initialize the player
	
	memset(&player, '\0', sizeof(creature));
	player.info = monsterCatalog[0];
	initializeGender(&player);
	player.movementSpeed = player.info.movementSpeed;
	player.attackSpeed = player.info.attackSpeed;
	clearStatus(&player);
	player.carriedItem = NULL;
	player.status[STATUS_NUTRITION] = player.maxStatus[STATUS_NUTRITION] = STOMACH_SIZE;
	player.currentHP = player.info.maxHP;
	rogue.previousHealthPercent = 100;
	player.creatureState = MONSTER_ALLY;
	player.ticksUntilTurn = 0;
	
	rogue.depthLevel = 1;
    rogue.deepestLevel = 1;
	rogue.scentTurnNumber = 1000;
	rogue.turnNumber = 0;
	rogue.foodSpawned = 0;
	rogue.gold = 0;
	rogue.goldGenerated = 0;
	rogue.disturbed = false;
	rogue.autoPlayingLevel = false;
	rogue.automationActive = false;
	rogue.justRested = false;
	rogue.easyMode = false;
	rogue.inWater = false;
	rogue.creaturesWillFlashThisTurn = false;
	rogue.updatedSafetyMapThisTurn = false;
	rogue.updatedAllySafetyMapThisTurn = false;
	rogue.updatedMapToSafeTerrainThisTurn = false;
	rogue.updatedMapToShoreThisTurn = false;
	rogue.experienceLevel = 1;
	rogue.experience = 0;
	rogue.strength = 12;
	rogue.weapon = NULL;
	rogue.armor = NULL;
	rogue.ringLeft = NULL;
	rogue.ringRight = NULL;
	rogue.monsterSpawnFuse = rand_range(125, 175);
	rogue.ticksTillUpdateEnvironment = 100;
	rogue.mapToShore = NULL;
	rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
	rogue.xpxpThisTurn = 0;
	
	rogue.minersLight = lightCatalog[MINERS_LIGHT];
	
	rogue.clairvoyance = rogue.aggravating = rogue.regenerationBonus
	= rogue.stealthBonus = rogue.transference = rogue.wisdomBonus = 0;
	rogue.lightMultiplier = 1;
	
	theItem = generateItem(FOOD, RATION);
	theItem = addItemToPack(theItem);
	
	theItem = generateItem(WEAPON, DAGGER);
	theItem->enchant1 = theItem->enchant2 = 0;
	theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC);
	identify(theItem);
	theItem = addItemToPack(theItem);
	equipItem(theItem, false);
	
	theItem = generateItem(WEAPON, DART);
	theItem->enchant1 = theItem->enchant2 = 0;
	theItem->quantity = 15;
	theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC);
	identify(theItem);
	theItem = addItemToPack(theItem);
	
	theItem = generateItem(ARMOR, LEATHER_ARMOR);
	theItem->enchant1 = 0;
	theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC);
	identify(theItem);
	theItem = addItemToPack(theItem);
	equipItem(theItem, false);
	
	DEBUG {
		theItem = generateItem(RING, RING_CLAIRVOYANCE);
		theItem->enchant1 = max(DROWS, DCOLS);
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_FIRE);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_LIGHTNING);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_BLINKING);
		theItem->enchant1 = theItem->charges = 10;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_TUNNELING);
		theItem->enchant1 = 10;
		theItem->charges = 3000;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_OBSTRUCTION);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_BECKONING);
		theItem->charges = 3000;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_DISCORD);
		theItem->enchant1 = theItem->charges = 10;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_ENTRANCEMENT);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_POISON);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_CONJURATION);
		theItem->enchant1 = 7;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_HEALING);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_PROTECTION);
		theItem->enchant1 = 30;
		theItem->charges = 300;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_DOMINATION);
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_PLENTY);
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WEAPON, DAGGER);
		theItem->enchant1 = 100;
		theItem->enchant2 = W_SLAYING;
		theItem->vorpalEnemy = MK_REVENANT;
		theItem->flags &= ~(ITEM_CURSED);
		theItem->flags |= (ITEM_PROTECTED | ITEM_RUNIC | ITEM_RUNIC_HINTED);
		theItem->damage.lowerBound = theItem->damage.upperBound = 3;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(ARMOR, LEATHER_ARMOR);
		theItem->enchant1 = 50;
		theItem->enchant2 = A_REFLECTION;
		theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC_HINTED);
		theItem->flags |= (ITEM_PROTECTED | ITEM_RUNIC);
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(RING, RING_AWARENESS);
		theItem->enchant1 = 30;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(POTION, POTION_LEVITATION);
        theItem->quantity = 2;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
//		short i;
//		for (i=0; i < NUMBER_SCROLL_KINDS; i++) {
//			theItem = generateItem(SCROLL, i);
//			theItem = addItemToPack(theItem);
//		}
	}
	blackOutScreen();
	welcome();
}

// call this once per level to set all the dynamic colors as a function of depth
void updateColors() {
	short i;
	
	for (i=0; i<NUMBER_DYNAMIC_COLORS; i++) {
		*(dynamicColors[i][0]) = *(dynamicColors[i][1]);
		applyColorAverage(dynamicColors[i][0], dynamicColors[i][2], min(100, max(0, rogue.depthLevel * 100 / 26)));
	}
}

void startLevel(short oldLevelNumber, short stairDirection) {
	unsigned long oldSeed;
	item *theItem;
	short loc[2], i, j, x, y, px, py, flying, dir;
	boolean isAlreadyAmulet = false, placedPlayer;
	creature *monst;
	enum dungeonLayers layer;
	unsigned long timeAway;
	short **mapToStairs;
	short **mapToPit;
	boolean connectingStairsDiscovered;
	
	rogue.cursorLoc[0] = -1;
	rogue.cursorLoc[1] = -1;
	rogue.lastTarget = NULL;
	
	if (stairDirection == 0) { // fallen
		connectingStairsDiscovered = (pmap[rogue.downLoc[0]][rogue.downLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED) ? true : false);
		levels[oldLevelNumber-1].playerExitedVia[0] = player.xLoc;
		levels[oldLevelNumber-1].playerExitedVia[1] = player.yLoc;
	}
	
	if (oldLevelNumber != rogue.depthLevel) {
		px = player.xLoc;
		py = player.yLoc;
		if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_AUTO_DESCENT)) {
			for (i=0; i<8; i++) {
				if (!cellHasTerrainFlag(player.xLoc+nbDirs[i][0], player.yLoc+nbDirs[i][1], (T_PATHING_BLOCKER))) {
					px = player.xLoc+nbDirs[i][0];
					py = player.yLoc+nbDirs[i][1];
					break;
				}
			}
		}
		mapToStairs = allocDynamicGrid();
		fillDynamicGrid(mapToStairs, 0);
		for (flying = 0; flying <= 1; flying++) {
			fillDynamicGrid(mapToStairs, 0);
			calculateDistances(mapToStairs, px, py, (flying ? T_OBSTRUCTS_PASSABILITY : T_PATHING_BLOCKER), NULL, true, true);
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				x = monst->xLoc;
				y = monst->yLoc;
				if (((monst->creatureState == MONSTER_TRACKING_SCENT && (stairDirection != 0 || monst->status[STATUS_LEVITATING]))
					 || monst->creatureState == MONSTER_ALLY)
					&& (stairDirection != 0 || monst->currentHP > 10 || monst->status[STATUS_LEVITATING])
					&& ((flying != 0) == ((monst->status[STATUS_LEVITATING] != 0)
										  || cellHasTerrainFlag(x, y, T_PATHING_BLOCKER)
										  || cellHasTerrainFlag(px, py, T_AUTO_DESCENT)))
					&& !(monst->bookkeepingFlags & MONST_CAPTIVE)
					&& !(monst->info.flags & (MONST_WILL_NOT_USE_STAIRS | MONST_RESTRICTED_TO_LIQUID))
					&& !(cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY))
					&& !monst->status[STATUS_ENTRANCED]
					&& !monst->status[STATUS_PARALYZED]
					&& mapToStairs[monst->xLoc][monst->yLoc] < 30000) {
					
					monst->status[STATUS_ENTERS_LEVEL_IN] = max(1, mapToStairs[monst->xLoc][monst->yLoc] * monst->movementSpeed / 100);
					switch (stairDirection) {
						case 1:
							monst->bookkeepingFlags |= MONST_APPROACHING_DOWNSTAIRS;
							break;
						case -1:
							monst->bookkeepingFlags |= MONST_APPROACHING_UPSTAIRS;
							break;
						case 0:
							monst->bookkeepingFlags |= MONST_APPROACHING_PIT;
							break;
						default:
							break;
					}
				}
			}
		}
		freeDynamicGrid(mapToStairs);
	}
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->mapToMe) {
			freeDynamicGrid(monst->mapToMe);
			monst->mapToMe = NULL;
		}
		if (monst->safetyMap) {
			freeDynamicGrid(monst->safetyMap);
			monst->safetyMap = NULL;
		}
	}
	levels[oldLevelNumber-1].monsters = monsters->nextCreature;
	levels[oldLevelNumber-1].dormantMonsters = dormantMonsters->nextCreature;
	levels[oldLevelNumber-1].items = floorItems->nextItem;
	
	levels[oldLevelNumber - 1].numberOfRooms = numberOfRooms;
	levels[oldLevelNumber - 1].roomStorage = rooms->nextRoom;
	rooms->nextRoom = NULL;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				levels[oldLevelNumber - 1].mapStorage[i][j].layers[layer] = pmap[i][j].layers[layer];
			}
			levels[oldLevelNumber - 1].mapStorage[i][j].volume = pmap[i][j].volume;
			levels[oldLevelNumber - 1].mapStorage[i][j].flags = (pmap[i][j].flags & PERMANENT_TILE_FLAGS);
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedAppearance = pmap[i][j].rememberedAppearance;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedTerrain = pmap[i][j].rememberedTerrain;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedItemCategory = pmap[i][j].rememberedItemCategory;
			levels[oldLevelNumber - 1].mapStorage[i][j].machineNumber = pmap[i][j].machineNumber;
		}
	}
	
	levels[oldLevelNumber - 1].awaySince = rogue.turnNumber;
	
	//	Prepare the new level
	
	rogue.minersLightRadius = 2.25 + (DCOLS - 1) * (float) pow(0.85, rogue.depthLevel);
	updateColors();
	updateRingBonuses(); // also updates miner's light
	
	if (!levels[rogue.depthLevel - 1].visited) { // level has not already been visited
		// generate new level
		oldSeed = (unsigned long) rand_range(0, 9999) + 10000 * rand_range(0, 9999);
		seedRandomGenerator(levels[rogue.depthLevel - 1].levelSeed);
		thisLevelProfile = &(levelProfileCatalog[rand_range(0, NUMBER_LEVEL_PROFILES - 1)]);
		
		// Load up next level's monsters and items, since one might have fallen from above.
		monsters->nextCreature			= levels[rogue.depthLevel-1].monsters;
		dormantMonsters->nextCreature	= levels[rogue.depthLevel-1].dormantMonsters;
		floorItems->nextItem			= levels[rogue.depthLevel-1].items;
		
		levels[rogue.depthLevel-1].monsters = NULL;
		levels[rogue.depthLevel-1].dormantMonsters = NULL;
		levels[rogue.depthLevel-1].items = NULL;
		
		digDungeon();
		setUpWaypoints();
		initializeLevel();
		
		shuffleTerrainColors(100, false);
		
		if (rogue.depthLevel >= AMULET_LEVEL && !numberOfMatchingPackItems(AMULET, 0, 0, false)
			&& levels[rogue.depthLevel-1].visited == false) {
			for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
				if (theItem->category & AMULET) {
					isAlreadyAmulet = true;
					break;
				}
			}
			if (!isAlreadyAmulet) {
				placeItem(generateItem(AMULET, 0), 0, 0);
			}
		}
		seedRandomGenerator(oldSeed);
		
		//logLevel();
		
		// Simulate 50 turns so the level is broken in (swamp gas accumulating, brimstone percolating, etc.).
		timeAway = 50;
		
	} else { // level has already been visited
		
		// restore level
		timeAway = max(0, rogue.turnNumber - levels[rogue.depthLevel - 1].awaySince - 1);
		numberOfRooms = levels[rogue.depthLevel - 1].numberOfRooms;
		rooms->nextRoom = levels[rogue.depthLevel - 1].roomStorage;
        levels[rogue.depthLevel - 1].roomStorage = NULL;
		
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
					pmap[i][j].layers[layer] = levels[rogue.depthLevel - 1].mapStorage[i][j].layers[layer];
				}
				pmap[i][j].volume = levels[rogue.depthLevel - 1].mapStorage[i][j].volume;
				pmap[i][j].flags = (levels[rogue.depthLevel - 1].mapStorage[i][j].flags & PERMANENT_TILE_FLAGS);
				pmap[i][j].rememberedAppearance = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedAppearance;
				pmap[i][j].rememberedTerrain = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedTerrain;
				pmap[i][j].rememberedItemCategory = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedItemCategory;
				pmap[i][j].machineNumber = levels[rogue.depthLevel - 1].mapStorage[i][j].machineNumber;
			}
		}
		
		setUpWaypoints();
		
		rogue.downLoc[0]	= levels[rogue.depthLevel - 1].downStairsLoc[0];
		rogue.downLoc[1]	= levels[rogue.depthLevel - 1].downStairsLoc[1];
		rogue.upLoc[0]		= levels[rogue.depthLevel - 1].upStairsLoc[0];
		rogue.upLoc[1]		= levels[rogue.depthLevel - 1].upStairsLoc[1];
		
		monsters->nextCreature = levels[rogue.depthLevel - 1].monsters;
		dormantMonsters->nextCreature = levels[rogue.depthLevel - 1].dormantMonsters;
		floorItems->nextItem = levels[rogue.depthLevel - 1].items;
		
		levels[rogue.depthLevel-1].monsters = NULL;
		levels[rogue.depthLevel-1].dormantMonsters = NULL;
		levels[rogue.depthLevel-1].items = NULL;
		
		if (numberOfMatchingPackItems(AMULET, 0, 0, false)) {
			isAlreadyAmulet = true;
		}
		
		for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
			if (theItem->category & AMULET) {
				if (isAlreadyAmulet) {
					
					pmap[theItem->xLoc][theItem->yLoc].flags &= ~(HAS_ITEM | ITEM_DETECTED);
					
					removeItemFromChain(theItem, floorItems);
					deleteItem(theItem);
					
					continue;
				}
				isAlreadyAmulet = true;
			}
			restoreItem(theItem);
		}
		
		mapToStairs = allocDynamicGrid();
		mapToPit = allocDynamicGrid();
		fillDynamicGrid(mapToStairs, 0);
		fillDynamicGrid(mapToPit, 0);
		calculateDistances(mapToStairs, player.xLoc, player.yLoc, T_PATHING_BLOCKER, NULL, true, true);
		calculateDistances(mapToPit, levels[rogue.depthLevel-1].playerExitedVia[0],
						   levels[rogue.depthLevel-1].playerExitedVia[0], T_PATHING_BLOCKER, NULL, true, true);
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			restoreMonster(monst, mapToStairs, mapToPit);
		}
		freeDynamicGrid(mapToStairs);
		freeDynamicGrid(mapToPit);
	}
	
	// Simulate the environment!
	// First bury the player in limbo while we run the simulation,
	// so that any harmful terrain doesn't affect her during the process.
	px = player.xLoc;
	py = player.yLoc;
	player.xLoc = player.yLoc = 0;
	for (i = 0; i < timeAway && i < 100; i++) {
		updateEnvironment();
	}
	player.xLoc = px;
	player.yLoc = py;
	
    if (!levels[rogue.depthLevel-1].visited) {
        levels[rogue.depthLevel-1].visited = true;
        if (rogue.depthLevel == 26) {
            messageWithColor("An alien energy permeates the area. The Amulet of Yendor must be nearby!", &itemMessageColor, false);
        }
    }
	
	// Position the player.
	if (stairDirection == 0) { // fell into the level
		
		getQualifyingLocNear(loc, player.xLoc, player.yLoc, true, 0,
							 (T_PATHING_BLOCKER),
							 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS | IS_IN_MACHINE), false, false);
	} else {
		if (stairDirection == 1) { // heading downward
			player.xLoc = rogue.upLoc[0];
			player.yLoc = rogue.upLoc[1];
		} else if (stairDirection == -1) { // heading upward
			player.xLoc = rogue.downLoc[0];
			player.yLoc = rogue.downLoc[1];
		}
        
        placedPlayer = false;
        for (dir=0; dir<4 && !placedPlayer; dir++) {
            loc[0] = player.xLoc + nbDirs[dir][0];
            loc[1] = player.yLoc + nbDirs[dir][1];
            if (!cellHasTerrainFlag(loc[0], loc[1], T_PATHING_BLOCKER)
                && !(pmap[loc[0]][loc[1]].flags & (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS | IS_IN_MACHINE))) {
                placedPlayer = true;
            }
        }
		if (!placedPlayer) {
            getQualifyingLocNear(loc, player.xLoc, player.yLoc, true, 0,
                                 (T_PATHING_BLOCKER),
                                 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS | IS_IN_MACHINE), false, true);
        }
	}
	player.xLoc = loc[0];
	player.yLoc = loc[1];
		
	pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
	
	if (connectingStairsDiscovered) {
        for (i = rogue.upLoc[0]-1; i <= rogue.upLoc[0] + 1; i++) {
            for (j = rogue.upLoc[1]-1; j <= rogue.upLoc[1] + 1; j++) {
                if (coordinatesAreInMap(i, j)) {
                    pmap[i][j].flags |= DISCOVERED;
                    rogue.xpxpThisTurn++;
                }
            }
        }
	}
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_IS_DEEP_WATER) && !player.status[STATUS_LEVITATING]
		&& !cellHasTerrainFlag(player.xLoc, player.yLoc, T_ENTANGLES)
		&& !cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_PASSABILITY)) {
		rogue.inWater = true;
	}
	
	updateMapToShore();
	
	updateVision(true);
	
	// update monster states so none are hunting if they can't see the player
	for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		updateMonsterState(monst);
	}
	
	rogue.playbackBetweenTurns = true;
	displayLevel();
	refreshSideBar(-1, -1, false);
	
	if (rogue.turnNumber) {
		rogue.turnNumber++;
	}
	RNGCheck();
	flushBufferToFile();
}

void freeGlobalDynamicGrid(short ***grid) {
	if (*grid) {
		freeDynamicGrid(*grid);
		*grid = NULL;
	}
}

void freeCreature(creature *monst) {
	freeGlobalDynamicGrid(&(monst->mapToMe));
	freeGlobalDynamicGrid(&(monst->safetyMap));
	if (monst->carriedItem) {
		free(monst->carriedItem);
		monst->carriedItem = NULL;
	}
	if (monst->carriedMonster) {
		freeCreature(monst->carriedMonster);
		monst->carriedMonster = NULL;
	}
	free(monst);
}

void emptyGraveyard() {
	creature *monst, *monst2;
	for (monst = graveyard->nextCreature; monst != NULL; monst = monst2) {
		monst2 = monst->nextCreature;
		freeCreature(monst);
	}
	graveyard->nextCreature = NULL;
}

void freeEverything() {
	short i;
	creature *monst, *monst2;
	item *theItem, *theItem2;
    room *room, *room2;
	
#ifdef AUDIT_RNG
	fclose(RNGLogFile);
#endif
    
	freeGlobalDynamicGrid(&scentMap);
	freeGlobalDynamicGrid(&safetyMap);
	freeGlobalDynamicGrid(&allySafetyMap);
	freeGlobalDynamicGrid(&chokeMap);
	freeGlobalDynamicGrid(&playerPathingMap);
	freeGlobalDynamicGrid(&rogue.mapToShore);
	freeGlobalDynamicGrid(&rogue.mapToSafeTerrain);
	
	for (i=0; i<101; i++) {
		for (monst = levels[i].monsters; monst != NULL; monst = monst2) {
			monst2 = monst->nextCreature;
			freeCreature(monst);
		}
		levels[i].monsters = NULL;
		for (monst = levels[i].dormantMonsters; monst != NULL; monst = monst2) {
			monst2 = monst->nextCreature;
			freeCreature(monst);
		}
		levels[i].dormantMonsters = NULL;
		for (theItem = levels[i].items; theItem != NULL; theItem = theItem2) {
			theItem2 = theItem->nextItem;
			deleteItem(theItem);
		}
		levels[i].items = NULL;
        for (room = levels[i].roomStorage; room != NULL; room = room2) {
            room2 = room->nextRoom;
            free(room);
        }
        levels[i].roomStorage = NULL;
	}
    for (monst = monsters; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    monsters = NULL;
    for (monst = dormantMonsters; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    dormantMonsters = NULL;
    for (monst = graveyard; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    graveyard = NULL;
    for (theItem = floorItems; theItem != NULL; theItem = theItem2) {
        theItem2 = theItem->nextItem;
        deleteItem(theItem);
    }
    floorItems = NULL;
    for (theItem = packItems; theItem != NULL; theItem = theItem2) {
        theItem2 = theItem->nextItem;
        deleteItem(theItem);
    }
    packItems = NULL;
    for (room = rooms; room != NULL; room = room2) {
        room2 = room->nextRoom;
        free(room);
    }
    rooms = NULL;
}

void gameOver(char *killedBy, boolean useCustomPhrasing) {
	char buf[COLS];
	rogueHighScoresEntry theEntry;
	cellDisplayBuffer dbuf[COLS][ROWS];
	boolean playback;
	
	rogue.autoPlayingLevel = false;
	
	flushBufferToFile();
	
	if (rogue.quit) {
		if (rogue.playbackMode) {
			playback = rogue.playbackMode;
			rogue.playbackMode = false;
			message("(The player quit at this point.)", true);
			rogue.playbackMode = playback;
		}
	} else {
		playback = rogue.playbackMode;
		if (!D_IMMORTAL) {
			rogue.playbackMode = false;
		}
		messageWithColor("You die...", &badMessageColor, true);
		rogue.playbackMode = playback;
	}
    
    rogue.creaturesWillFlashThisTurn = false;
	
	if (D_IMMORTAL && !rogue.quit) {
		message("...but then you get better.", false);
		player.currentHP = player.info.maxHP;
		if (player.status[STATUS_NUTRITION] < 10) {
			player.status[STATUS_NUTRITION] = STOMACH_SIZE;
		}
		player.bookkeepingFlags &= ~MONST_IS_DYING;
		return;
	}
	
	if (rogue.highScoreSaved) {
		return;
	}
	rogue.highScoreSaved = true;
	
	if (rogue.quit) {
		blackOutScreen();
	} else {
		copyDisplayBuffer(dbuf, displayBuffer);
		funkyFade(dbuf, &black, 0, 30, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), false);
	}
	
	if (useCustomPhrasing) {
		sprintf(buf, "%s on level %i%s.", killedBy, rogue.depthLevel,
				(numberOfMatchingPackItems(AMULET, 0, 0, false) > 0 ? ", amulet in hand" : ""));
	} else {
		sprintf(buf, "Killed by a%s %s on level %i%s.", (isVowel(killedBy) ? "n" : ""), killedBy,
				rogue.depthLevel, (numberOfMatchingPackItems(AMULET, 0, 0, false) > 0 ? ", amulet in hand" : ""));
	}
	
	theEntry.score = rogue.gold;
	if (rogue.easyMode) {
		theEntry.score /= 10;
	}
	strcpy(theEntry.description, buf);
	
	printString(buf, ((COLS - strLenWithoutEscapes(buf)) / 2), (ROWS / 2), &gray, &black, 0);
	if (!rogue.quit) {
		displayMoreSign();
	}
	
	if (!rogue.playbackMode) {
		if (saveHighScore(theEntry)) {
			printHighScores(true);
		}
		blackOutScreen();
		saveRecording();
	}
	
	rogue.gameHasEnded = true;
}

void victory() {
	char buf[DCOLS];
	item *theItem;
	short i, gemCount = 0;
	unsigned long totalValue = 0;
	rogueHighScoresEntry theEntry;
	boolean qualified, isPlayback;
	cellDisplayBuffer dbuf[COLS][ROWS];
	
	flushBufferToFile();
	
	deleteMessages();
	message("You are bathed in sunlight as you throw open the heavy doors.", false);
	
	copyDisplayBuffer(dbuf, displayBuffer);
	funkyFade(dbuf, &white, 0, 100, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), false);
	displayMoreSign();
	printString("Congratulations; you have escaped from the Dungeons of Doom!     ", mapToWindowX(0), mapToWindowY(-1), &black, &white, 0);
	displayMoreSign();
	
	clearDisplayBuffer(dbuf);
	
	deleteMessages();
	strcpy(displayedMessage[0], "You sell your treasures and live out your days in fame and glory.");
	
	printString(displayedMessage[0], mapToWindowX(0), mapToWindowY(-1), &white, &black, dbuf);
	
	printString("Gold", mapToWindowX(2), 2, &white, &black, dbuf);
	sprintf(buf, "%li", rogue.gold);
	printString(buf, mapToWindowX(60), 2, &yellow, &black, dbuf);
	totalValue += rogue.gold;
	
	for (i = 3, theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem, i++) {
		if (theItem->category & GEM) {
			gemCount += theItem->quantity;
		}
		identify(theItem);
		itemName(theItem, buf, true, true, &white);
		upperCase(buf);
		printString(buf, mapToWindowX(2), min(ROWS-1, i + 1), &white, &black, dbuf);
		sprintf(buf, "%li", max(0, itemValue(theItem)));
		printString(buf, mapToWindowX(60), min(ROWS-1, i + 1), &yellow, &black, dbuf);
		totalValue += max(0, itemValue(theItem));
	}
	i++;
	printString("TOTAL:", mapToWindowX(2), min(ROWS-1, i + 1), &teal, &black, dbuf);
	sprintf(buf, "%li", totalValue);
	printString(buf, mapToWindowX(60), min(ROWS-1, i + 1), &teal, &black, dbuf);
	
	funkyFade(dbuf, &white, 0, 15, COLS/2, ROWS/2, true);	
	
	if (gemCount == 0) {
		strcpy(theEntry.description, "Escaped the Dungeons of Doom!");
	} else if (gemCount == 1) {
		strcpy(theEntry.description, "Escaped the Dungeons of Doom with a lumenstone!");
	} else {
		sprintf(theEntry.description, "Escaped the Dungeons of Doom with %i lumenstones!", gemCount);
	}
	
	theEntry.score = totalValue;
	
	if (rogue.easyMode) {
		theEntry.score /= 10;
	}
	
	if (!DEBUGGING && !rogue.playbackMode) {
		qualified = saveHighScore(theEntry);
	} else {
		qualified = false;
	}
	
	isPlayback = rogue.playbackMode;
	rogue.playbackMode = false;
	displayMoreSign();
	rogue.playbackMode = isPlayback;
	
	saveRecording();
	
	printHighScores(qualified);
	
	rogue.gameHasEnded = true;
}

void enableEasyMode() {
	if (rogue.easyMode) {
		message("Alas, all hope of salvation is lost. You shed scalding tears at your plight.", false);
		return;
	}
	message("A dark presence surrounds you, whispering promises of stolen power.", true);
	if (confirm("Succumb to demonic temptation (i.e. enable Easy Mode)?", false)) {
		recordKeystroke(EASY_MODE_KEY, false, true);
		message("An ancient and terrible evil burrows into your willing flesh!", true);
		player.info.displayChar = '&';
		rogue.easyMode = true;
		refreshDungeonCell(player.xLoc, player.yLoc);
		refreshSideBar(-1, -1, false);
		message("Wracked by spasms, your body contorts into an ALL-POWERFUL AMPERSAND!!!", false);
		message("You have a feeling that you will take 20% as much damage from now on.", false);
		message("But great power comes at a great price -- specifically, a 90% income tax rate.", false);
	} else {
		message("The evil dissipates, hissing, from the air around you.", false);
	}
}

// takes a flag of the form Fl(n) and returns n
short unflag(unsigned long flag) {
	short i;
	for (i=0; i<32; i++) {
		if (flag >> i == 1) {
			return i;
		}
	}
	return -1;
}
