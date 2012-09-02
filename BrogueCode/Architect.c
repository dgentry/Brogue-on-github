/*
 *  Architect.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/10/09.
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

short topBlobMinX, topBlobMinY, blobWidth, blobHeight;
levelSpecProfile levelSpec;

unsigned long wallNum;

#ifdef BROGUE_ASSERTS // otherwise handled as a macro in rogue.h
boolean cellHasTerrainFlag(short x, short y, unsigned long flagMask) {
	assert(coordinatesAreInMap(x, y));
	return ((flagMask) & terrainFlags((x), (y)) ? true : false);
}
#endif

boolean checkLoopiness(short x, short y) {
	boolean inString;
	short newX, newY, dir, sdir;
	short numStrings, maxStringLength, currentStringLength;
	const short cDirs[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
	
	if (!(pmap[x][y].flags & IN_LOOP)) {
		return false;
	}
	
	// find an unloopy neighbor to start on
	for (sdir = 0; sdir < 8; sdir++) {
		newX = x + cDirs[sdir][0];
		newY = y + cDirs[sdir][1];
		if (!coordinatesAreInMap(newX, newY)
			|| !(pmap[newX][newY].flags & IN_LOOP)) {
			break;
		}
	}
	if (sdir == 8) { // no unloopy neighbors
		return false; // leave cell loopy
	}
	
	// starting on this unloopy neighbor, work clockwise and count up (a) the number of strings
	// of loopy neighbors, and (b) the length of the longest such string.
	numStrings = maxStringLength = currentStringLength = 0;
	inString = false;
	for (dir = sdir; dir < sdir + 8; dir++) {
		newX = x + cDirs[dir % 8][0];
		newY = y + cDirs[dir % 8][1];
		if (coordinatesAreInMap(newX, newY) && (pmap[newX][newY].flags & IN_LOOP)) {
			currentStringLength++;
			if (!inString) {
				if (numStrings > 0) {
					return false; // more than one string here; leave loopy
				}
				numStrings++;
				inString = true;
			}
		} else if (inString) {
			if (currentStringLength > maxStringLength) {
				maxStringLength = currentStringLength;
			}
			currentStringLength = 0;
			inString = false;
		}
	}
	if (inString && currentStringLength > maxStringLength) {
		maxStringLength = currentStringLength;
	}
	if (numStrings == 1 && maxStringLength <= 4) {
		pmap[x][y].flags &= ~IN_LOOP;
		
		for (dir = 0; dir < 8; dir++) {
			newX = x + cDirs[dir][0];
			newY = y + cDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)) {
				checkLoopiness(newX, newY);
			}
		}
		return true;
	} else {
		return false;
	}
}

void auditLoop(short x, short y, char grid[DCOLS][DROWS]) {
	short dir, newX, newY;
	if (coordinatesAreInMap(x, y)
		&& !grid[x][y]
		&& !(pmap[x][y].flags & IN_LOOP)) {
		
		grid[x][y] = true;
		for (dir = 0; dir < 8; dir++) {
			newX = x + nbDirs[dir][0];
			newY = y + nbDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)) {
				auditLoop(newX, newY, grid);
			}
		}
	}
}

// Assumes it is called with respect to a passable (startX, startY), and that the same is not already included in results.
// Returns 10000 if the area included an area machine.
short floodFillCount(char results[DCOLS][DROWS], char passMap[DCOLS][DROWS], short startX, short startY) {
	short dir, newX, newY, count;
	
	count = (passMap[startX][startY] == 2 ? 5000 : 1);
	
	if (pmap[startX][startY].flags & IS_IN_AREA_MACHINE) {
		count = 10000;
	}
	
	results[startX][startY] = true;
	
	for(dir=0; dir<4; dir++) {
		newX = startX + nbDirs[dir][0];
		newY = startY + nbDirs[dir][1];
		if (coordinatesAreInMap(newX, newY)
			&& passMap[newX][newY]
			&& !results[newX][newY]) {
			
			count += floodFillCount(results, passMap, newX, newY);
		}
	}
	return min(count, 10000);
}

// Rotates around the cell, counting up the number of distinct strings of passable neighbors in a single revolution.
//		Zero means there are no impassable tiles adjacent.
//		One means it is adjacent to a wall.
//		Two means it is in a hallway or something similar.
//		Three means it is the center of a T-intersection, or something analogous.
//		Four means it is in the intersection of two hallways.
//		Five or more means there is a bug.
short passableArcCount(short x, short y) {
	short arcCount, dir, oldX, oldY, newX, newY;
	const short cDirs[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
	
#ifdef BROGUE_ASSERTS
	assert(coordinatesAreInMap(x, y));
#endif
	
	arcCount = 0;
	for (dir = 0; dir < 8; dir++) {
		oldX = x + cDirs[(dir + 7) % 8][0];
		oldY = y + cDirs[(dir + 7) % 8][1];
		newX = x + cDirs[dir][0];
		newY = y + cDirs[dir][1];
		// Counts every transition from passable to impassable or vice-versa on the way around the cell:
		if ((coordinatesAreInMap(newX, newY) && cellIsPassableOrDoor(newX, newY))
			!= (coordinatesAreInMap(oldX, oldY) && cellIsPassableOrDoor(oldX, oldY))) {
			arcCount++;
		}
	}
	return arcCount / 2; // Since we added one when we entered a wall and another when we left.
}

// locates all loops and chokepoints
void analyzeMap(boolean calculateChokeMap) {
	short i, j, i2, j2, dir, newX, newY, oldX, oldY, passableArcCount, cellCount;
	char grid[DCOLS][DROWS], passMap[DCOLS][DROWS];
	boolean designationSurvives;
	const short cDirs[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
	
	// first find all of the loops
	rogue.staleLoopMap = false;
	
	for(i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, T_PATHING_BLOCKER)
				&& !cellHasTerrainFlag(i, j, T_IS_SECRET)) {
				pmap[i][j].flags &= ~IN_LOOP;
				passMap[i][j] = false;
			} else {
				pmap[i][j].flags |= IN_LOOP;
				passMap[i][j] = true;
			}
		}
	}
	
	for(i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {
			checkLoopiness(i, j);
		}
	}
	
	// remove extraneous loop markings
	zeroOutGrid(grid);
	auditLoop(0, 0, grid);
	
	for(i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {
			if (pmap[i][j].flags & IN_LOOP) {
				designationSurvives = false;
				for (dir = 0; dir < 8; dir++) {
					newX = i + nbDirs[dir][0];
					newY = j + nbDirs[dir][1];
					if (coordinatesAreInMap(newX, newY)
						&& !grid[newX][newY]
						&& !(pmap[newX][newY].flags & IN_LOOP)) {
						designationSurvives = true;
						break;
					}
				}
				if (!designationSurvives) {
					grid[i][j] = true;
					pmap[i][j].flags &= ~IN_LOOP;
				}
			}
		}
	}
	
	// done finding loops; now flag chokepoints
	for(i=1; i<DCOLS-1; i++) {
		for(j=1; j<DROWS-1; j++) {
			pmap[i][j].flags &= ~IS_CHOKEPOINT;
			if (passMap[i][j] && !(pmap[i][j].flags & IN_LOOP)) {
				passableArcCount = 0;
				for (dir = 0; dir < 8; dir++) {
					oldX = i + cDirs[(dir + 7) % 8][0];
					oldY = j + cDirs[(dir + 7) % 8][1];
					newX = i + cDirs[dir][0];
					newY = j + cDirs[dir][1];
					if ((coordinatesAreInMap(newX, newY) && passMap[newX][newY])
						!= (coordinatesAreInMap(oldX, oldY) && passMap[oldX][oldY])) {
						if (++passableArcCount > 2) {
							if (!passMap[i-1][j] && !passMap[i+1][j] || !passMap[i][j-1] && !passMap[i][j+1]) {
								pmap[i][j].flags |= IS_CHOKEPOINT;
							}
							break;
						}
					}
				}
			}
		}
	}
	
	if (calculateChokeMap) {
		
		// done finding chokepoints; now create a chokepoint map.
		
		// The chokepoint map is a number for each passable tile. If the tile is a chokepoint,
		// then the number indicates the number of tiles that would be rendered unreachable if the
		// chokepoint were blocked. If the tile is not a chokepoint, then the number indicates
		// the number of tiles that would be rendered unreachable if the nearest exit chokepoint
		// were blocked.
		// The cost of all of this is one depth-first flood-fill per open point that is adjacent to a chokepoint.
		
		// Start by setting the chokepoint values really high, and roping off room machines.
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				chokeMap[i][j] = 30000;
				if (pmap[i][j].flags & IS_IN_ROOM_MACHINE) {
					passMap[i][j] = false;
				}
			}
		}
		
		// Scan through and find a chokepoint next to an open point.
		
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (passMap[i][j] && (pmap[i][j].flags & IS_CHOKEPOINT)) {
					for (dir=0; dir<4; dir++) {
						newX = i + nbDirs[dir][0];
						newY = j + nbDirs[dir][1];
						if (coordinatesAreInMap(newX, newY)
							&& passMap[newX][newY]
							&& !(pmap[newX][newY].flags & IS_CHOKEPOINT)) {
							// OK, (newX, newY) is an open point and (i, j) is a chokepoint.
							// Pretend (i, j) is blocked by changing passMap, and run a flood-fill cell count starting on (newX, newY).
							// Keep track of the flooded region in grid[][].
							zeroOutGrid(grid);
							passMap[i][j] = false;
							cellCount = floodFillCount(grid, passMap, newX, newY);
							passMap[i][j] = true;
							
							// CellCount is the size of the region that would be obstructed if the chokepoint were blocked.
							// CellCounts less than 4 are not useful, so we skip those cases.
							
							if (cellCount >= 4) {
								// Now, on the chokemap, all of those flooded cells should take the lesser of their current value or this resultant number.
								for(i2=0; i2<DCOLS; i2++) {
									for(j2=0; j2<DROWS; j2++) {
										if (grid[i2][j2] && cellCount < chokeMap[i2][j2]) {
											chokeMap[i2][j2] = cellCount;
											pmap[i2][j2].flags &= ~IS_GATE_SITE;
										}
									}
								}
								
								// The chokepoint itself should also take the lesser of its current value or the flood count.
								if (cellCount < chokeMap[i][j]) {
									chokeMap[i][j] = cellCount;
									pmap[i][j].flags |= IS_GATE_SITE;
								}
							}
						}
					}
				}
			}
		}
	}
}

// Assumes (startX, startY) is in the machine.
// Returns true if everything went well, and false if we ran into a machine component
// that was already there -- we don't want to build a machine around it.
boolean addTileToMachineInteriorAndIterate(char interior[DCOLS][DROWS], short startX, short startY) {
	short dir, newX, newY;
	boolean goodSoFar = true;
	
	interior[startX][startY] = true;
	
	for (dir = 0; dir < 4 && goodSoFar; dir++) {
		newX = startX + nbDirs[dir][0];
		newY = startY + nbDirs[dir][1];
		if ((pmap[newX][newY].flags & HAS_ITEM)
			|| ((pmap[newX][newY].flags & IS_IN_MACHINE) && !(pmap[newX][newY].flags & IS_GATE_SITE))) {
			// Abort if there's an item in the room.
			// Items haven't been populated yet, so the only way this could happen is if another machine
			// previously placed an item here.
			// Also abort if we're touching another machine at any point other than a gate tile.
			return false;
		}
		if (coordinatesAreInMap(newX, newY)
			&& !interior[newX][newY]
			&& chokeMap[newX][newY] <= chokeMap[startX][startY] // don't have to worry about walls since they're all 30000
			&& !(pmap[newX][newY].flags & IS_IN_MACHINE)) {
			goodSoFar = goodSoFar && addTileToMachineInteriorAndIterate(interior, newX, newY);
		}
	}
	return goodSoFar;
}

void copyMap(pcell from[DCOLS][DROWS], pcell to[DCOLS][DROWS]) {
	short i, j;
	
	for(i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {
			to[i][j] = from[i][j];
		}
	}
}

boolean blueprintQualifies(short i, unsigned long requiredMachineFlags) {
	if (blueprintCatalog[i].depthRange[0] > rogue.depthLevel
		|| blueprintCatalog[i].depthRange[1] < rogue.depthLevel
				// Must have the required flags:
		|| (~(blueprintCatalog[i].flags) & requiredMachineFlags)
				// May NOT have BP_ADOPT_ITEM_MANDATORY unless that flag is required:
		|| (blueprintCatalog[i].flags & BP_ADOPT_ITEM_MANDATORY & ~requiredMachineFlags)) {
		
		return false;
	}
	return true;
}

void abortItemsAndMonsters(item *spawnedItems[MACHINES_BUFFER_LENGTH], creature *spawnedMonsters[MACHINES_BUFFER_LENGTH]) {
	short i, j;
	
	for (i=0; i<MACHINES_BUFFER_LENGTH && spawnedItems[i]; i++) {
		removeItemFromChain(spawnedItems[i], floorItems);
		removeItemFromChain(spawnedItems[i], packItems); // just in case; can't imagine why this would arise.
		for (j=0; j<MACHINES_BUFFER_LENGTH && spawnedMonsters[j]; j++) {
			// Remove the item from spawned monsters, so it doesn't get double-freed when the creature is killed below.
			if (spawnedMonsters[j]->carriedItem == spawnedItems[i]) {
				spawnedMonsters[j]->carriedItem = NULL;
				break;
			}
		}
		deleteItem(spawnedItems[i]);
        spawnedItems[i] = NULL;
	}
	
	for (i=0; i<MACHINES_BUFFER_LENGTH && spawnedMonsters[i]; i++) {
		killCreature(spawnedMonsters[i], true);
        spawnedMonsters[i] = NULL;
	}
}

boolean cellIsFeatureCandidate(short x, short y,
							   short originX, short originY,
							   short distanceBound[2],
							   char interior[DCOLS][DROWS],
							   char occupied[DCOLS][DROWS],
							   char viewMap[DCOLS][DROWS],
							   short **distanceMap,
							   unsigned long featureFlags,
							   unsigned long bpFlags) {
	short newX, newY, dir, distance;
	
	// No building in the hallway if it's prohibited.
	// This check comes before the origin check, so an area machine will fail altogether
	// if its origin is in a hallway and the feature that must be built there does not permit as much.
	if ((featureFlags & MF_NOT_IN_HALLWAY)
		&& passableArcCount(x, y) > 1) {
		return false;
	}
	
	// No building along the perimeter of the level if it's prohibited.
	if ((featureFlags & MF_NOT_ON_LEVEL_PERIMETER)
		&& (x == 0 || x == DCOLS - 1 || y == 0 || y == DROWS - 1)) {
		return false;
	}
	
	// The origin is a candidate if the feature is flagged to be built at the origin.
	// If it's a room, the origin (i.e. doorway) is otherwise NOT a candidate.
	if (featureFlags & MF_BUILD_AT_ORIGIN) {
		return ((x == originX && y == originY) ? true : false);
	} else if ((bpFlags & BP_ROOM) && x == originX && y == originY) {
		return false;
	}
	
	// No building in another feature's personal space!
	if (occupied[x][y]) {
		return false; 
	}
	
	// Must be in the viewmap if the appropriate flag is set.
	if ((featureFlags & (MF_IN_VIEW_OF_ORIGIN | MF_IN_PASSABLE_VIEW_OF_ORIGIN))
		&& !viewMap[x][y]) {
		return false;
	}
	
	// Do a distance check if the feature requests it.
	if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
		distance = 10000;
		for (dir = 0; dir < 4; dir++) {
			newX = x + nbDirs[dir][0];
			newY = y + nbDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)
				&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)
				&& distance > distanceMap[newX][newY] + 1) {
				
				distance = distanceMap[newX][newY] + 1; // to take care of walls
			}
		}
	} else {
		distance = distanceMap[x][y];
	}
	
	if (distance > distanceBound[1]		// distance exceeds max
		|| distance < distanceBound[0]) {	// distance falls short of min
		return false;
	}
	if (featureFlags & MF_BUILD_IN_WALLS) {				// If we're supposed to build in a wall...
		if (!interior[x][y]
			&& cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) { // ...and this location is a wall...
			for (dir=0; dir<4; dir++) {
				newX = x + nbDirs[dir][0];
				newY = y + nbDirs[dir][1];
				if (coordinatesAreInMap(newX, newY)		// ...and it's next to an interior spot or permitted elsewhere and next to passable spot...
					&& ((interior[newX][newY] && !(newX==originX && newY==originY))
						|| ((featureFlags & MF_BUILD_ANYWHERE_ON_LEVEL)
							&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)))) {
					return true;						// ...then we're golden!
				}
			}
		}
		return false;									// Otherwise, no can do.
	} else if (featureFlags & MF_BUILD_ANYWHERE_ON_LEVEL) {
		if ((featureFlags & MF_GENERATE_ITEM)
			&& (cellHasTerrainFlag(x, y, T_OBSTRUCTS_ITEMS | T_PATHING_BLOCKER) || (pmap[x][y].flags & (IS_CHOKEPOINT | IN_LOOP | IS_IN_MACHINE)))) {
			return false;
		} else {
			return true;
		}
	} else if (interior[x][y]) {
		return true;
	}
	return false;
}


void addLocationToKey(item *theItem, short x, short y, boolean disposableHere) {
	short i;
	
	for (i=0; i < KEY_ID_MAXIMUM && (theItem->keyLoc[i].x || theItem->keyLoc[i].machine); i++);
	theItem->keyLoc[i].x = x;
	theItem->keyLoc[i].y = y;
	theItem->keyLoc[i].disposableHere = disposableHere;
}

void addMachineNumberToKey(item *theItem, short machineNumber, boolean disposableHere) {
	short i;
	
	for (i=0; i < KEY_ID_MAXIMUM && (theItem->keyLoc[i].x || theItem->keyLoc[i].machine); i++);
	theItem->keyLoc[i].machine = machineNumber;
	theItem->keyLoc[i].disposableHere = disposableHere;
}

void expandMachineInterior(char interior[DCOLS][DROWS], short minimumInteriorNeighbors) {
    boolean madeChange;
    short nbcount, newX, newY, i, j, dir, layer;
    
    do {
        madeChange = false;
        for(i=1; i<DCOLS-1; i++) {
            for(j=1; j < DROWS-1; j++) {
                if (cellHasTerrainFlag(i, j, T_PATHING_BLOCKER) ) {
                    //&& (!pmap[i][j].flags & IS_IN_MACHINE)) {
                    
                    // Count up the number of interior open neighbors out of eight:
                    for (nbcount = dir = 0; dir < 8; dir++) {
                        newX = i + nbDirs[dir][0];
                        newY = j + nbDirs[dir][1];
                        if (interior[newX][newY]
                            && !cellHasTerrainFlag(newX, newY, T_PATHING_BLOCKER)) {
                            nbcount++;
                        }
                    }
                    if (nbcount >= minimumInteriorNeighbors) {
                        // Make sure zero exterior open/machine neighbors out of eight:
                        for (nbcount = dir = 0; dir < 8; dir++) {
                            newX = i + nbDirs[dir][0];
                            newY = j + nbDirs[dir][1];
                            if (!interior[newX][newY]
                                && !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
                                nbcount++;
                                break;
                            }
                        }
                        if (!nbcount) {
                            // Eliminate this obstruction; welcome its location into the room.
                            madeChange = true;
                            interior[i][j] = true;
                            for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
                                if (tileCatalog[pmap[i][j].layers[layer]].flags & T_PATHING_BLOCKER) {
                                    pmap[i][j].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
                                }
                            }
                            for (dir = 0; dir < 8; dir++) {
                                newX = i + nbDirs[dir][0];
                                newY = j + nbDirs[dir][1];
                                if (pmap[newX][newY].layers[DUNGEON] == GRANITE) {
                                    pmap[newX][newY].layers[DUNGEON] = TOP_WALL;
                                }
                            }
                        }
                    }
                }
            }
        }
    } while (madeChange);
}

// Returns true if the machine got built; false if it was aborted.
// If empty array parentSpawnedItems or parentSpawnedMonsters is given, will pass those back for deletion if necessary.
boolean buildAMachine(enum machineTypes bp,
					  short originX, short originY,
					  unsigned long requiredMachineFlags,
					  item *adoptiveItem,
					  item *parentSpawnedItems[MACHINES_BUFFER_LENGTH],
					  creature *parentSpawnedMonsters[MACHINES_BUFFER_LENGTH]) {
	
	short i, j, k, feat, randIndex, totalFreq, gateCandidates[50][2],
	layer, dir, newX, newY, instance, instanceCount, qualifyingTileCount,
	featX, featY, itemCount, monsterCount,
	sRows[DROWS], sCols[DCOLS],
	**distanceMap, distance25, distance75, distances[100], distanceBound[2],
	personalSpace, failsafe, locationFailsafe,
	machineNumber;
	const unsigned long alternativeFlags[2] = {MF_ALTERNATIVE, MF_ALTERNATIVE_2};
	
	// Our boolean grids:
	//	Interior:		This is the master grid for the machine. All area inside the machine are set to true.
	//	Occupied:		This keeps track of what is within the personal space of a previously built feature in the same machine.
	//	Candidates:		This is calculated at the start of each feature, and is true where that feature is eligible for building.
	//	BlockingMap:	Used during terrain/DF placement in features that are flagged not to tolerate blocking, to see if they block.
	//	ViewMap:		Used for features with MF_IN_VIEW_OF_ORIGIN, to calculate which cells are in view of the origin.
	char interior[DCOLS][DROWS], occupied[DCOLS][DROWS], candidates[DCOLS][DROWS], blockingMap[DCOLS][DROWS], viewMap[DCOLS][DROWS];
	
	boolean DFSucceeded, terrainSucceeded, generateEverywhere, skipFeature[20], chooseBP, chooseLocation, tryAgain;
	
	pcell levelBackup[DCOLS][DROWS];
	
	creature *monst, *nextMonst, *torchBearer = NULL, *leader = NULL;
	
	item *theItem, *torch = NULL, *spawnedItems[MACHINES_BUFFER_LENGTH] = {0}, *spawnedItemsSub[MACHINES_BUFFER_LENGTH] = {0};
	creature *spawnedMonsters[MACHINES_BUFFER_LENGTH] = {0}, *spawnedMonstersSub[MACHINES_BUFFER_LENGTH] = {0};
	
	const machineFeature *feature;
	
	distanceMap = NULL;
	
	chooseBP = (((signed short) bp) <= 0 ? true : false);
	
	chooseLocation = (originX <= 0 || originY <= 0 ? true : false);
	
	failsafe = 10;
	do {
		if (--failsafe <= 0) {
			if (distanceMap) {
				freeDynamicGrid(distanceMap);
			}
			DEBUG {
				if (chooseBP || chooseLocation) {
					printf("\nDepth %i: Failed to build a machine; gave up after 10 unsuccessful attempts to find a suitable blueprint and/or location.",
						   rogue.depthLevel);
				} else {
					printf("\nDepth %i: Failed to build a machine; requested blueprint and location did not work.",
						   rogue.depthLevel);
				}
			}
			return false;
		}
		
		if (chooseBP) { // If no blueprint is given, then pick one:
			
			// First, choose the blueprint. We choose from among blueprints
			// that have the required blueprint flags and that satisfy the depth requirements.
			totalFreq = 0;
			for (i=1; i<NUMBER_BLUEPRINTS; i++) {
				if (blueprintQualifies(i, requiredMachineFlags)) {
					totalFreq += blueprintCatalog[i].frequency;
				}
			}
			
			if (!totalFreq) { // If no suitable blueprints are in the library, fail.
				if (distanceMap) {
					freeDynamicGrid(distanceMap);
				}
				DEBUG printf("\nDepth %i: Failed to build a machine because no suitable blueprints were available.",
							 rogue.depthLevel);
				return false;
			}
			
			// Pick from among the suitable blueprints.
			randIndex = rand_range(1, totalFreq);
			for (i=1; i<NUMBER_BLUEPRINTS; i++) {
				if (blueprintQualifies(i, requiredMachineFlags)) {
					if (randIndex <= blueprintCatalog[i].frequency) {
						bp = i;
						break;
					} else {
						randIndex -= blueprintCatalog[i].frequency;
					}
				}
			}
			
			// If we don't have a blueprint yet, something went wrong.
#ifdef BROGUE_ASSERTS
			assert(bp>0);
#endif
		}
		
		// Find a location and map out the machine interior.
		if (blueprintCatalog[bp].flags & BP_ROOM) {
			// If it's a room machine, count up the gates of appropriate
			// choke size and remember where they are. The origin of the room will be the gate location.
			zeroOutGrid(interior);
			
			if (chooseLocation) {
                analyzeMap(true); // Make sure the chokeMap is up to date.
				totalFreq = 0;
				for(i=0; i<DCOLS; i++) {
					for(j=0; j<DROWS && totalFreq < 50; j++) {
						if ((pmap[i][j].flags & IS_GATE_SITE)
							&& !(pmap[i][j].flags & IS_IN_MACHINE)
							&& chokeMap[i][j] >= blueprintCatalog[bp].roomSize[0]
							&& chokeMap[i][j] <= blueprintCatalog[bp].roomSize[1]) {
							
							gateCandidates[totalFreq][0] = i;
							gateCandidates[totalFreq][1] = j;
							totalFreq++;
						}
					}
				}
				
				if (totalFreq) {
					// Choose the gate.
					randIndex = rand_range(0, totalFreq - 1);
					originX = gateCandidates[randIndex][0];
					originY = gateCandidates[randIndex][1];
				} else {
					// If no suitable sites, abort.
					if (distanceMap) {
						freeDynamicGrid(distanceMap);
					}
					DEBUG printf("\nDepth %i: Failed to build a machine; there was no eligible door candidate for the chosen room machine from blueprint %i.",
								 rogue.depthLevel,
								 bp);
					return false;
				}
			}
			
			// Now map out the interior into interior[][].
			// Start at the gate location and do a depth-first floodfill to grab all adjoining tiles with the
			// same or lower choke value, ignoring any tiles that are already part of a machine.
			// If we get false from this, try again. If we've tried too many times already, abort.
			tryAgain = !addTileToMachineInteriorAndIterate(interior, originX, originY);
		} else {
			// Find a location and map out the interior for a non-room machine.
			// The strategy here is simply to pick a random location on the map,
			// expand it along a pathing map by one space in all directions until the size reaches
			// the chosen size, and then make sure the resulting space qualifies.
			// If not, try again. If we've tried too many times already, abort.
			
			locationFailsafe = 10;
			do {
				zeroOutGrid(interior);
				tryAgain = false;
				
				if (chooseLocation) {
					// Pick a random origin location.
					randomMatchingLocation(&originX, &originY, FLOOR, NOTHING, -1);
				}
				
				if (!distanceMap) {
					distanceMap = allocDynamicGrid();
				}
				fillDynamicGrid(distanceMap, 0);
				calculateDistances(distanceMap, originX, originY, T_PATHING_BLOCKER, NULL, true, false);
				qualifyingTileCount = 0; // Keeps track of how many interior cells we've added.
				totalFreq = rand_range(blueprintCatalog[bp].roomSize[0], blueprintCatalog[bp].roomSize[1]); // Keeps track of the goal size.
				
				for (i=0; i<DCOLS; i++) {
					sCols[i] = i;
				}
				shuffleList(sCols, DCOLS);
				for (i=0; i<DROWS; i++) {
					sRows[i] = i;
				}
				shuffleList(sRows, DROWS);
				
				for (k=0; k<1000 && qualifyingTileCount < totalFreq; k++) {
					for(i=0; i<DCOLS && qualifyingTileCount < totalFreq; i++) {
						for(j=0; j<DROWS && qualifyingTileCount < totalFreq; j++) {
							if (distanceMap[sCols[i]][sRows[j]] == k) {
								interior[sCols[i]][sRows[j]] = true;
								qualifyingTileCount++;
								
								if (pmap[sCols[i]][sRows[j]].flags & (HAS_ITEM | IS_IN_MACHINE)) {
									// Abort if we've entered another machine or engulfed another machine's item.
									tryAgain = true;
									qualifyingTileCount = totalFreq; // This is a hack to drop out of these three for-loops.
								}
							}
						}
					}
				}
				
				// Now make sure the interior map satisfies the machine's qualifications.
				if ((blueprintCatalog[bp].flags & BP_TREAT_AS_BLOCKING)
					&& levelIsDisconnectedWithBlockingMap(interior, false)) {
					tryAgain = true;
				} else if ((blueprintCatalog[bp].flags & BP_REQUIRE_BLOCKING)
						   && levelIsDisconnectedWithBlockingMap(interior, true) < 100) {
					tryAgain = true; // BP_REQUIRE_BLOCKING needs some work to make sure the disconnect is interesting.
				}
				// If locationFailsafe runs out, tryAgain will still be true, and we'll try a different machine.
				// If we're not choosing the blueprint, then don't bother with the locationFailsafe; just use the higher-level failsafe.
			} while (chooseBP && tryAgain && --locationFailsafe);
		}
		
		// If something went wrong, but we haven't been charged with choosing blueprint OR location,
		// then there is nothing to try again, so just fail.
		if (tryAgain && !chooseBP && !chooseLocation) {
			if (distanceMap) {
				freeDynamicGrid(distanceMap);
			}
			return false;
		}
		
		// Now loop if necessary.
	} while (tryAgain);
	
	// This is the point of no return. Back up the level so it can be restored if we have to abort this machine after this point.
	copyMap(pmap, levelBackup);
	
	// If requested, clear and expand the room as far as possible until either it's convex or it bumps into surrounding rooms
	if (blueprintCatalog[bp].flags & BP_MAXIMIZE_INTERIOR) {
		expandMachineInterior(interior, 1);
	} else if (blueprintCatalog[bp].flags & BP_OPEN_INTERIOR) {
		expandMachineInterior(interior, 4);
	}
	
	// If requested, cleanse the interior -- no interesting terrain allowed.
	if (blueprintCatalog[bp].flags & BP_PURGE_INTERIOR) {
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (interior[i][j]) {
					for (layer=0; layer<NUMBER_TERRAIN_LAYERS; layer++) {
						pmap[i][j].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
					}
				}
			}
		}
	}
	
	// If requested, purge pathing blockers -- no traps allowed.
	if (blueprintCatalog[bp].flags & BP_PURGE_PATHING_BLOCKERS) {
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (interior[i][j]) {
					for (layer=0; layer<NUMBER_TERRAIN_LAYERS; layer++) {
						if (tileCatalog[pmap[i][j].layers[layer]].flags & T_PATHING_BLOCKER) {
							pmap[i][j].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
						}
					}
				}
			}
		}
	}
	
	// If requested, purge the liquid layer in the interior -- no liquids allowed.
	if (blueprintCatalog[bp].flags & BP_PURGE_LIQUIDS) {
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (interior[i][j]) {
					pmap[i][j].layers[LIQUID] = NOTHING;
				}
			}
		}
	}
	
	// Surround with walls if requested.
	if (blueprintCatalog[bp].flags & BP_SURROUND_WITH_WALLS) {
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (interior[i][j] && !(pmap[i][j].flags & IS_GATE_SITE)) {
					for (dir=0; dir<8; dir++) {
						newX = i + nbDirs[dir][0];
						newY = j + nbDirs[dir][1];
						if (coordinatesAreInMap(newX, newY)
							&& !interior[newX][newY]
							&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)
							&& !(pmap[newX][newY].flags & IS_GATE_SITE)
							&& !pmap[newX][newY].machineNumber) {
							for (layer=0; layer<NUMBER_TERRAIN_LAYERS; layer++) {
								pmap[newX][newY].layers[layer] = (layer == DUNGEON ? TOP_WALL : 0);
							}
						}
					}
				}
			}
		}
	}
	
	// Reinforce surrounding tiles and interior tiles if requested to prevent tunneling in or through.
	if (blueprintCatalog[bp].flags & BP_IMPREGNABLE) {
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (interior[i][j]) {
					pmap[i][j].flags |= IMPREGNABLE;
					if (!(pmap[i][j].flags & IS_GATE_SITE)) {
						for (dir=0; dir<8; dir++) {
							newX = i + nbDirs[dir][0];
							newY = j + nbDirs[dir][1];
							if (coordinatesAreInMap(newX, newY)
								&& !interior[newX][newY]) {
								pmap[newX][newY].flags |= IMPREGNABLE;
							}
						}
					}
				}
			}
		}
	}
	
	// If necessary, label the interior as IS_IN_AREA_MACHINE or IS_IN_ROOM_MACHINE and mark down the number.
	machineNumber = ++rogue.machineNumber; // Reserve this machine number, starting with 1.
	if (!(blueprintCatalog[bp].flags & BP_NO_INTERIOR_FLAG)) {
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (interior[i][j]) {
					pmap[i][j].flags |= ((blueprintCatalog[bp].flags & BP_ROOM) ? IS_IN_ROOM_MACHINE : IS_IN_AREA_MACHINE);
					pmap[i][j].machineNumber = machineNumber;
					// also clear any secret doors, since they screw up distance mapping and aren't fun inside machines
					if (pmap[i][j].layers[DUNGEON] == SECRET_DOOR) {
						pmap[i][j].layers[DUNGEON] = DOOR;
					}
				}
			}
		}
	}
	
//	DEBUG printf("\n\nWorking on blueprint %i, with origin at (%i, %i). Here's the initial interior map:", bp, originX, originY);
//	DEBUG logBuffer(interior);
	
	// Calculate the distance map (so that features that want to be close to or far from the door can be placed accordingly)
	// and figure out the 33rd and 67th percentiles for features that want to be near or far from the origin.
	if (!distanceMap) {
		distanceMap = allocDynamicGrid();
	}
	fillDynamicGrid(distanceMap, 0);
	calculateDistances(distanceMap, originX, originY, T_PATHING_BLOCKER, NULL, true, true);
	qualifyingTileCount = 0;
	for (i=0; i<100; i++) {
		distances[i] = 0;
	}
	for(i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {
			if (interior[i][j]
				&& distanceMap[i][j] < 100) {
				distances[distanceMap[i][j]]++; // create a histogram of distances -- poor man's sort function
				qualifyingTileCount++;
			}
		}
	}
	distance25 = (int) (qualifyingTileCount / 4);
	distance75 = (int) (3 * qualifyingTileCount / 4);
	for (i=0; i<100; i++) {
		if (distance25 <= distances[i]) {
			distance25 = i;
			break;
		} else {
			distance25 -= distances[i];
		}
	}
	for (i=0; i<100; i++) {
		if (distance75 <= distances[i]) {
			distance75 = i;
			break;
		} else {
			distance75 -= distances[i];
		}
	}
	//DEBUG printf("\nDistances calculated: 33rd percentile of distance is %i, and 67th is %i.", distance25, distance75);
	
	// Now decide which features will be skipped -- of the features marked MF_ALTERNATIVE, skip all but one, chosen randomly.
	// Then repeat and do the same with respect to MF_ALTERNATIVE_2, to provide up to two independent sets of alternative features per machine.
	
	for (i=0; i<blueprintCatalog[bp].featureCount; i++) {
		skipFeature[i] = false;
	}
	for (j = 0; j <= 1; j++) {
		totalFreq = 0;
		for (i=0; i<blueprintCatalog[bp].featureCount; i++) {
			if (blueprintCatalog[bp].feature[i].flags & alternativeFlags[j]) {
				skipFeature[i] = true;
				totalFreq++;
			}
		}
		if (totalFreq > 0) {
			randIndex = rand_range(1, totalFreq);
			for (i=0; i<blueprintCatalog[bp].featureCount; i++) {
				if (blueprintCatalog[bp].feature[i].flags & alternativeFlags[j]) {
					if (randIndex == 1) {
						skipFeature[i] = false; // This is the alternative that gets built. The rest do not.
						break;
					} else {
						randIndex--;
					}
				}
			}
		}
	}
	
	// Keep track of all monsters and items that we spawn -- if we abort, we have to go back and delete them all.
	itemCount = monsterCount = 0;
	
	// Zero out occupied[][], and use it to keep track of the personal space around each feature that gets placed.
	zeroOutGrid(occupied);
	
	// Now tick through the features and build them.
	for (feat = 0; feat < blueprintCatalog[bp].featureCount; feat++) {
		
		if (skipFeature[feat]) {
			continue; // Skip the alternative features that were not selected for building.
		}
		
		feature = &(blueprintCatalog[bp].feature[feat]);
		
		// Figure out the distance bounds.
		distanceBound[0] = 0;
		distanceBound[1] = 10000;
		if (feature->flags & MF_NEAR_ORIGIN) {
			distanceBound[1] = distance25;
		}
		if (feature->flags & MF_FAR_FROM_ORIGIN) {
			distanceBound[0] = distance75;
		}
		
		if (feature->flags & (MF_IN_VIEW_OF_ORIGIN | MF_IN_PASSABLE_VIEW_OF_ORIGIN)) {
			zeroOutGrid(viewMap);
            if (feature->flags & MF_IN_PASSABLE_VIEW_OF_ORIGIN) {
                getFOVMask(viewMap, originX, originY, max(DCOLS, DROWS), T_PATHING_BLOCKER, 0, false);
            } else {
                getFOVMask(viewMap, originX, originY, max(DCOLS, DROWS), (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION), 0, false);
            }
			viewMap[originX][originY] = true;
			
			if (D_INSPECT_MACHINES) {
				dumpLevelToScreen();
				hiliteGrid(viewMap, &omniscienceColor, 75);
				temporaryMessage("Showing visibility.", true);
			}
		}
		
		do { // If the MF_REPEAT_UNTIL_NO_PROGRESS flag is set, repeat until we fail to build the required number of instances.
			
			// Make a master map of candidate locations for this feature.
			qualifyingTileCount = 0;
			for(i=0; i<DCOLS; i++) {
				for(j=0; j<DROWS; j++) {
					if (cellIsFeatureCandidate(i, j, originX, originY, distanceBound, interior, occupied, viewMap, distanceMap, feature->flags, blueprintCatalog[bp].flags)) {
						qualifyingTileCount++;
						candidates[i][j] = true;
					} else {
						candidates[i][j] = false;
					}
				}
			}
			
			if (D_INSPECT_MACHINES) {
				dumpLevelToScreen();
				hiliteGrid(occupied, &red, 75);
				hiliteGrid(candidates, &green, 75);
				hiliteGrid(interior, &blue, 75);
				temporaryMessage("Indicating: Occupied (red); Candidates (green); Interior (blue).", true);
			}
			
			if (feature->flags & MF_EVERYWHERE & ~MF_BUILD_AT_ORIGIN) {
				// Generate everywhere that qualifies -- instead of randomly picking tiles, keep spawning until we run out of eligible tiles.
				generateEverywhere = true;
			} else {
				// build as many instances as required
				generateEverywhere = false;
				instanceCount = rand_range(feature->instanceCountRange[0], feature->instanceCountRange[1]);
			}
			
			// Cache the personal space constant.
			personalSpace = feature->personalSpace;
			//		if (generateEverywhere && personalSpace == 0) {
			//			personalSpace = 1; // Otherwise we might keep generating it on the same tiles forever.
			//		}
			
			for (instance = 0; (generateEverywhere || instance < instanceCount) && qualifyingTileCount > 0;) {
				
				// Find a location for the feature.
				if (feature->flags & MF_BUILD_AT_ORIGIN) {
					// Does the feature want to be at the origin? If so, put it there. (Just an optimization.)
					featX = originX;
					featY = originY;
				} else {
					// Pick our candidate location randomly, and also strike it from
					// the candidates map so that subsequent instances of this same feature can't choose it.
					featX = -1;
					randIndex = rand_range(1, qualifyingTileCount);
					for(i=0; i<DCOLS && featX < 0; i++) {
						for(j=0; j<DROWS && featX < 0; j++) {
							if (candidates[i][j]) {
								if (randIndex == 1) {
									// This is the place!
									featX = i;
									featY = j;
									i = DCOLS;	// break out of the loops
									j = DROWS;
								} else {
									randIndex--;
								}
							}
						}
					}
				}
				// Don't waste time trying the same place again whether or not this attempt succeeds.
				candidates[featX][featY] = false;
				qualifyingTileCount--;
				
				DFSucceeded = terrainSucceeded = true;
				
				// Try to build the DF first, if any, since we don't want it to be disrupted by subsequently placed terrain.
				if (feature->featureDF) {
					DFSucceeded = spawnDungeonFeature(featX, featY, &dungeonFeatureCatalog[feature->featureDF], false,
													  !(feature->flags & MF_PERMIT_BLOCKING));
				}
				
				// Now try to place the terrain tile, if any.
				if (DFSucceeded && feature->terrain) {
					// Must we check for blocking?
					if (!(feature->flags & MF_PERMIT_BLOCKING)
						&& ((tileCatalog[feature->terrain].flags & T_PATHING_BLOCKER) || (feature->flags & MF_TREAT_AS_BLOCKING))) {
						// Yes, check for blocking.
						
						zeroOutGrid(blockingMap);
						blockingMap[featX][featY] = true;
						terrainSucceeded = !levelIsDisconnectedWithBlockingMap(blockingMap, false);
					}
					if (terrainSucceeded) {
						pmap[featX][featY].layers[feature->layer] = feature->terrain;
					}
				}
				
				// OK, if placement was successful, clear some personal space around the feature so subsequent features can't be generated too close.
				// Personal space of 0 means nothing gets cleared, 1 means that only the tile itself gets cleared, and 2 means the 3x3 grid centered on it.
				
				if (DFSucceeded && terrainSucceeded) {
					for (i = featX - personalSpace + 1;
						 i <= featX + personalSpace - 1;
						 i++) {
						for (j = featY - personalSpace + 1;
							 j <= featY + personalSpace - 1;
							 j++) {
							if (coordinatesAreInMap(i, j)) {
								if (candidates[i][j]) {
#ifdef BROGUE_ASSERTS
									assert(!occupied[i][j] || (i == originX && j == originY)); // Candidates[][] should never be true where occupied[][] is true.
#endif
									candidates[i][j] = false;
									qualifyingTileCount--;
								}
								occupied[i][j] = true;
							}
						}
					}
					instance++; // we've placed an instance
					//DEBUG printf("\nPlaced instance #%i of feature %i at (%i, %i).", instance, feat, featX, featY);
				}
				
				if (DFSucceeded && terrainSucceeded) { // Proceed only if the terrain stuff for this instance succeeded.
					
					// Mark the feature location as part of the machine, in case it is not already inside of it.
					if (!(blueprintCatalog[bp].flags & BP_NO_INTERIOR_FLAG)) {
						pmap[featX][featY].flags |= ((blueprintCatalog[bp].flags & BP_ROOM) ? IS_IN_ROOM_MACHINE : IS_IN_AREA_MACHINE);
						pmap[featX][featY].machineNumber = machineNumber;
					}
					
					// Mark the feature location as impregnable if requested.
					if (feature->flags & MF_IMPREGNABLE) {
						pmap[featX][featY].flags |= IMPREGNABLE;
					}
					
					// Generate an item as necessary.
					if ((feature->flags & MF_GENERATE_ITEM)
						|| (adoptiveItem && (feature->flags & MF_ADOPT_ITEM) && (blueprintCatalog[bp].flags & BP_ADOPT_ITEM_MANDATORY))) {
						// Are we adopting an item instead of generating one?
						if (adoptiveItem && (feature->flags & MF_ADOPT_ITEM) && (blueprintCatalog[bp].flags & BP_ADOPT_ITEM_MANDATORY)) {
							theItem = adoptiveItem;
							adoptiveItem = NULL; // can be adopted only once
						} else {
							// Have to create an item ourselves.
							theItem = generateItem(feature->itemCategory, feature->itemKind);
							failsafe = 1000;
							while ((feature->itemValueMinimum > 0 && itemValue(theItem) < feature->itemValueMinimum) // must be at least as expensive as requested
								   || ((feature->flags & MF_REQUIRE_GOOD_RUNIC) && (!(theItem->flags & ITEM_RUNIC) || (theItem->flags & ITEM_CURSED))) // runic and uncursed if requested
								   || ((feature->flags & MF_NO_THROWING_WEAPONS) && theItem->category == WEAPON && theItem->quantity > 1)) { // no throwing weapons if prohibited
								deleteItem(theItem);
								theItem = generateItem(feature->itemCategory, feature->itemKind);
								if (failsafe <= 0) {
									break;
								}
								failsafe--;
							}
							spawnedItems[itemCount] = theItem; // Keep a list of generated items so that we can delete them all if construction fails.
							if (parentSpawnedItems) {
								parentSpawnedItems[itemCount] = theItem;
							}
							itemCount++;
							theItem->flags |= feature->itemFlags;
						}
						
						addLocationToKey(theItem, featX, featY, (feature->flags & MF_KEY_DISPOSABLE) ? true : false);
						theItem->keyZ = rogue.depthLevel;
						if (feature->flags & MF_SKELETON_KEY) {
							addMachineNumberToKey(theItem, machineNumber, (feature->flags & MF_KEY_DISPOSABLE) ? true : false);
						}
						
						if (feature->flags & MF_OUTSOURCE_ITEM_TO_MACHINE) {
							// Put this item up for adoption.
							// Try to create a sub-machine that will accept our item.
							// If we fail 10 times, abort the entire machine (including any sub-machines already built).
							// Also, if we build a sub-machine, and it succeeds, but this (its parent machine) fails,
							// we pass the monsters and items that it spawned back to the parent,
							// so that if the parent fails, they can all be freed.
							for (i=10; i > 0; i--) {
								// First make sure our adopted item, if any, is not on the floor or in the pack already.
								// Otherwise, a previous attempt to place it may have put it on the floor in a different
								// machine, only to have that machine fail and be deleted, leaving the item remaining on
								// the floor where placed.
								if (theItem) {
									removeItemFromChain(theItem, floorItems);
									removeItemFromChain(theItem, packItems);
									theItem->nextItem = NULL;
								}
								
								// Now put the item up for adoption.
								if (buildAMachine(-1, -1, -1, BP_ADOPT_ITEM_MANDATORY, theItem, spawnedItemsSub, spawnedMonstersSub)) {
									// Success! Now we have to add that machine's items and monsters to our own list, so they
									// all get deleted if this machine or its parent fails.
									for (j=0; j<MACHINES_BUFFER_LENGTH && spawnedItemsSub[j]; j++) {
                                        spawnedItems[itemCount] = spawnedItemsSub[j];
                                        if (parentSpawnedItems) {
                                            parentSpawnedItems[itemCount] = spawnedItemsSub[j];
                                        }
                                        itemCount++;
                                        spawnedItemsSub[j] = NULL;
									}
									for (j=0; j<MACHINES_BUFFER_LENGTH && spawnedMonstersSub[j]; j++) {
                                        spawnedMonsters[monsterCount] = spawnedMonstersSub[j];
                                        if (parentSpawnedMonsters) {
                                            parentSpawnedMonsters[monsterCount] = spawnedMonstersSub[j];
                                        }
                                        monsterCount++;
                                        spawnedMonstersSub[j] = NULL;
									}
									break;
								}
							}
							
							if (!i) {
								DEBUG printf("\nDepth %i: Failed to place blueprint %i because it requires an adoptive machine and we couldn't place one.", rogue.depthLevel, bp);
								// failure! abort!
								copyMap(levelBackup, pmap);
								abortItemsAndMonsters(spawnedItems, spawnedMonsters);
								freeDynamicGrid(distanceMap);
								return false;
							}
							theItem = NULL;
						} else if (!(feature->flags & MF_MONSTER_TAKE_ITEM)) {
							// Place the item at the feature location.
							placeItem(theItem, featX, featY);
						}
					}
					
					// Generate a horde as necessary.
					if (feature->flags & (MF_GENERATE_HORDE | MF_GENERATE_MONSTER)) {
						if (feature->flags & MF_GENERATE_HORDE) {
							monst = spawnHorde(0,
											   featX,
											   featY,
											   ((HORDE_IS_SUMMONED | HORDE_LEADER_CAPTIVE) & ~(feature->hordeFlags)),
											   feature->hordeFlags);
							if (monst) {
								monst->bookkeepingFlags |= MONST_JUST_SUMMONED;
							}
						}
						
						if (feature->flags & MF_GENERATE_MONSTER) {
							monst = generateMonster(feature->monsterID, true);
							if (monst) {
								monst->xLoc = featX;
								monst->yLoc = featY;
								pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
								monst->bookkeepingFlags |= MONST_JUST_SUMMONED;
							}
						}
						
						if (monst) {
							if (!leader) {
								leader = monst;
							}
							
							// Give our item to the monster leader if appropriate.
							// Actually just remember that we have to give it to this monster; the actual
							// hand-off happens after we're sure that the machine will succeed.
							if (theItem && (feature->flags & MF_MONSTER_TAKE_ITEM)) {
								torchBearer = monst;
								torch = theItem;
							}
						}
						
						for (monst = monsters->nextCreature; monst; monst = nextMonst) {
							// Have to cache the next monster, as the chain can get disrupted by making a monster dormant below.
							nextMonst = monst->nextCreature;
							if (monst->bookkeepingFlags & MONST_JUST_SUMMONED) {
								
								// All monsters spawned by a machine are tribemates.
								// Assign leader/follower roles if they are not yet assigned.
								if (!(monst->bookkeepingFlags & (MONST_LEADER | MONST_FOLLOWER))) {
									if (leader && leader != monst) {
										monst->leader = leader;
										monst->bookkeepingFlags &= ~MONST_LEADER;
										monst->bookkeepingFlags |= MONST_FOLLOWER;
										leader->bookkeepingFlags |= MONST_LEADER;
									} else {
										leader = monst;
									}
								}
								
								monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
								spawnedMonsters[monsterCount] = monst;
								if (parentSpawnedMonsters) {
									parentSpawnedMonsters[monsterCount] = monst;
								}
								monsterCount++;
								if (feature->flags & MF_MONSTER_SLEEPING) {
									monst->creatureState = MONSTER_SLEEPING;
								}
								if (feature->flags & MF_MONSTERS_DORMANT) {
									toggleMonsterDormancy(monst);
									if (!(feature->flags & MF_MONSTER_SLEEPING) && monst->creatureState != MONSTER_ALLY) {
										monst->creatureState = MONSTER_TRACKING_SCENT;
									}
								}
                                monst->machineHome = machineNumber; // Monster remembers the machine that spawned it.
							}
						}
					}
				}
				theItem = NULL;
				
				// Finished with this instance!
			}
		} while ((feature->flags & MF_REPEAT_UNTIL_NO_PROGRESS) && instance >= feature->minimumInstanceCount);
		
		//DEBUG printf("\nFinished feature %i. Here's the candidates map:", feat);
		//DEBUG logBuffer(candidates);
		
		if (instance < feature->minimumInstanceCount && !(feature->flags & MF_REPEAT_UNTIL_NO_PROGRESS)) {
			// failure! abort!
			
			DEBUG printf("\nDepth %i: Failed to place blueprint %i because of feature %i; needed %i instances but got only %i.",
						 rogue.depthLevel, bp, feat, feature->minimumInstanceCount, instance);
			
			// Restore the map to how it was before we touched it.
			copyMap(levelBackup, pmap);
			abortItemsAndMonsters(spawnedItems, spawnedMonsters);
			freeDynamicGrid(distanceMap);
			return false;
		}
	}
	
	if (torchBearer && torch) {
		if (torchBearer->carriedItem) {
			deleteItem(torchBearer->carriedItem);
		}
		removeItemFromChain(torch, floorItems);
		torchBearer->carriedItem = torch;
	}
	
	freeDynamicGrid(distanceMap);
	DEBUG printf("\nDepth %i: Built a machine from blueprint %i with an origin at (%i, %i).", rogue.depthLevel, bp, originX, originY);
	return true;
}

// add machines to the dungeon.
void addMachines() {
	short machineCount, failsafe = 50;
    
	analyzeMap(true);
	
	// Add reward rooms first, if any:
	
	machineCount = 0;
	
	while (rogue.depthLevel <= AMULET_LEVEL
		&& ((rogue.rewardRoomsGenerated + machineCount) * 2 + 1) * 2 < rogue.depthLevel * MACHINES_FACTOR) {
		// try to build at least one every four levels on average
		machineCount++;
	}
    while (rand_percent(15 * MACHINES_FACTOR)) {
        machineCount++;
    }
	
	for (failsafe = 50; machineCount && failsafe; failsafe--) {
		if (buildAMachine(-1, -1, -1, BP_REWARD, NULL, NULL, NULL)) {
			machineCount--;
			rogue.rewardRoomsGenerated++;
		}
	}
}

// Add terrain, DFs and machines. Includes traps, torches, funguses, flavor machines, etc.
void runAutogenerators() {
	short AG, count, x, y, i;
	const autoGenerator *gen;
	char grid[DCOLS][DROWS];
	
	// Cycle through the autoGenerators.
	for (AG=1; AG<NUMBER_AUTOGENERATORS; AG++) {
		
		// Shortcut:
		gen = &(autoGeneratorCatalog[AG]);
		
		// Enforce depth constraints.
		if (rogue.depthLevel < gen->minDepth || rogue.depthLevel > gen->maxDepth) {
			continue;
		}
		
		// Decide how many of this AG to build.
		count = min((gen->minNumberIntercept + rogue.depthLevel * gen->minNumberSlope) / 100, gen->maxNumber);
		while (rand_percent(gen->frequency) && count < gen->maxNumber) {
			count++;
		}
		
		// Build that many instances.
		for (i = 0; i < count; i++) {
			
			// Find a location for DFs and terrain generations.
			if (randomMatchingLocation(&x, &y, gen->requiredDungeonFoundationType, NOTHING, -1)) {
				
				// Spawn the DF.
				if (gen->DFType) {
					spawnDungeonFeature(x, y, &(dungeonFeatureCatalog[gen->DFType]), false, true);
					
					if (D_INSPECT_LEVELGEN) {
						dumpLevelToScreen();
						hiliteCell(x, y, &yellow, 50, true);
						message("Dungeon feature added.", true);
					}
				}
				
				// Spawn the terrain if it's got the priority to spawn there and won't disrupt connectivity.
				if (gen->terrain
					&& tileCatalog[pmap[x][y].layers[gen->layer]].drawPriority >= tileCatalog[gen->terrain].drawPriority) {
					
					// Check connectivity.
					zeroOutGrid(grid);
					grid[x][y] = true;
					if (!(tileCatalog[gen->terrain].flags & T_PATHING_BLOCKER)
						|| !levelIsDisconnectedWithBlockingMap(grid, false)) {
						
						// Build!
						pmap[x][y].layers[gen->layer] = gen->terrain;
						
						if (D_INSPECT_LEVELGEN) {
							dumpLevelToScreen();
							hiliteCell(x, y, &yellow, 50, true);
							message("Terrain added.", true);
						}
					}
				}
			}
			
			// Attempt to build the machine if requested.
			// Machines will find their own locations, so it will not be at the same place as terrain and DF.
			if (gen->machine > 0) {
				buildAMachine(gen->machine, -1, -1, 0, NULL, NULL, NULL);
			}
		}
	}
}

// Knock down the boundaries between similar lakes where possible.
void cleanUpLakeBoundaries() {
	short i, j, x, y, layer;
	boolean reverse, madeChange;
	
	reverse = true;
	
	do {
		madeChange = false;
		reverse = !reverse;
		
		for (i = (reverse ? DCOLS - 2 : 1);
			 (reverse ? i > 0 : i < DCOLS - 1);
			 (reverse ? i-- : i++)) {
			
			for (j = (reverse ? DROWS - 2 : 1);
				 (reverse ? j > 0 : j < DROWS - 1);
				 (reverse ? j-- : j++)) {
				
				//assert(i >= 1 && i <= DCOLS - 2 && j >= 1 && j <= DROWS - 2);
				
				if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)
                    && !cellHasTerrainFlag(i, j, T_IS_SECRET)
					&& !(pmap[i][j].flags & IMPREGNABLE)) {
					
					x = y = 0;
					if ((terrainFlags(i - 1, j) & T_LAKE_PATHING_BLOCKER)
						&& (terrainFlags(i - 1, j) & T_LAKE_PATHING_BLOCKER) == (terrainFlags(i + 1, j) & T_LAKE_PATHING_BLOCKER)) {
						x = i + 1;
						y = j;
					} else if ((terrainFlags(i, j - 1) & T_LAKE_PATHING_BLOCKER)
							   && (terrainFlags(i, j - 1) & T_LAKE_PATHING_BLOCKER) == (terrainFlags(i, j + 1) & T_LAKE_PATHING_BLOCKER)) {
						x = i;
						y = j + 1;
					}
					if (x) {
						madeChange = true;
						for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
							pmap[i][j].layers[layer] = pmap[x][y].layers[layer];
							//pmap[i][j].layers[DUNGEON] = CRYSTAL_WALL;
						}
					}
				}
			}
		}
	} while (madeChange);
}

// This is the master function for digging out a dungeon level. Finishing touches -- items, monsters, staircases, etc. -- are handled elsewhere.
void digDungeon() {
	short i, j, k;
	short x1, x2, y1, y2;
	short roomWidth, roomHeight, roomWidth2, roomHeight2, roomX, roomY, roomX2, roomY2;
	short randIndex, doorCandidateX, doorCandidateY;
	short dir;
	short numLoops;
	short lakeMaxWidth, lakeMaxHeight;
	short depthRoomSizeScaleFactor;
	short deepLiquid, shallowLiquid, shallowLiquidWidth;
	char wreathMap[DCOLS][DROWS], unfilledLakeMap[DCOLS][DROWS], connectedMap[DCOLS][DROWS];
	boolean isCorridor = false;
	boolean isCross = false;
	boolean roomWasPlaced = true;
	boolean diagonalCornerRemoved;
	boolean foundExposure;
	room *builtRoom = NULL, *fromRoom = NULL;
	
	rogue.machineNumber = 0;
	
	topBlobMinX = topBlobMinY = blobWidth = blobHeight = 0;
	
	rogue.scentTurnNumber = 10000;
	
	for (i=0; i<4; i++) {
		numberOfWalls[i] = -1;
	}
	for (i=0; i<4; i++) {
		for (j=0; j<DCOLS*DROWS; j++) {
			listOfWallsX[i][j] = 0;
			listOfWallsY[i][j] = 0;
		}
	}
	
#ifdef AUDIT_RNG
	char RNGMessage[100];
	sprintf(RNGMessage, "\n\n\nDigging dungeon level %i:\n", rogue.depthLevel);
	RNGLog(RNGMessage);
#endif
	
	// Clear level and fill with granite
	for( i=0; i<DCOLS; i++ ) {
		for( j=0; j<DROWS; j++ ) {	
			pmap[i][j].layers[DUNGEON] = GRANITE;
			pmap[i][j].layers[LIQUID] = NOTHING;
			pmap[i][j].layers[GAS] = NOTHING;
			pmap[i][j].layers[SURFACE] = NOTHING;
			pmap[i][j].machineNumber = 0;
			pmap[i][j].rememberedTerrain = NOTHING;
			pmap[i][j].rememberedItemCategory = 0;
			pmap[i][j].flags = 0;
			scentMap[i][j] = 0;
			pmap[i][j].volume = 0;
		}
	}
	
	//depthRoomSizeScaleFactor = 1000 - 500 * (min(rogue.depthLevel - 1, 25)) / 25;
	depthRoomSizeScaleFactor = 1000;
	levelSpec.roomMinWidth = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MIN_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.roomMaxWidth = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MAX_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.roomMinHeight = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MIN_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.roomMaxHeight = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MAX_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.horCorrMinLength = max(MIN_SCALED_ROOM_DIMENSION, HORIZONTAL_CORRIDOR_MIN_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.horCorrMaxLength = max(MIN_SCALED_ROOM_DIMENSION, HORIZONTAL_CORRIDOR_MAX_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.vertCorrMinLength = max(MIN_SCALED_ROOM_DIMENSION, VERTICAL_CORRIDOR_MIN_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.vertCorrMaxLength = max(MIN_SCALED_ROOM_DIMENSION, VERTICAL_CORRIDOR_MAX_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMinWidth = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MIN_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMaxWidth = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MAX_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMinHeight = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MIN_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMaxHeight = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MAX_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.secretDoorChance = max(0, min(67, (rogue.depthLevel - 1) * 67 / 25));
	levelSpec.numberOfTraps = rand_range((rogue.depthLevel - 1) / 4, (rogue.depthLevel - 1) / 2);
	
	// Build a rectangular seed room or cave at a random place
	if (rogue.depthLevel == 1) {
		roomWidth = levelSpec.crossRoomMaxWidth - 3;
		roomHeight = levelSpec.roomMaxHeight + 3;
		roomWidth2 = levelSpec.roomMaxWidth;
		roomHeight2 = levelSpec.crossRoomMaxHeight - 1;
		roomX = DCOLS/2 - roomWidth/2 - 1;
		roomY = DROWS - roomHeight - 2;
		roomX2 = DCOLS/2 - roomWidth2/2 - 1;
		roomY2 = DROWS - roomHeight2 - 2;
		
		carveRectangle(roomX, roomY, roomWidth, roomHeight);
		carveRectangle(roomX2, roomY2, roomWidth2, roomHeight2);
		markRectangle(roomX, roomY, roomWidth, roomHeight);
		markRectangle(roomX2, roomY2, roomWidth2, roomHeight2);
		allocateRoom(roomX, roomY, roomWidth, roomHeight, roomX2, roomY2, roomWidth2, roomHeight2);
	} else if (rand_percent(thisLevelProfile->caveLevelChance)) {
		generateCave();
	} else {		
		roomWidth = rand_range(4, 25);
		roomHeight = rand_range(2, 7);
		roomX = rand_range(1, DCOLS - roomWidth - 1);
		roomY = rand_range(1, DROWS - roomHeight - 1);
		carveRectangle(roomX, roomY, roomWidth, roomHeight);
		markRectangle(roomX, roomY, roomWidth, roomHeight);
		allocateRoom(roomX, roomY, roomWidth, roomHeight, 0, 0, 0, 0);
	}
	
	numberOfRooms = 1;
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("First room placed.", true);
	}
	
	// Now start placing random rooms branching off of preexisting rooms; make 600 attempts
	
	for(k=0; k<600 && numberOfRooms <= thisLevelProfile->maxNumberOfRooms; k++) {
		
		dir = rand_range(0,3); // Are we going to try to branch up, down, left, or right? Pick randomly.
		
		// Is it a corridor? Harder to place corridors, since you need space for a room at the end,
		// so the last 225 attempts are hard-wired not to be corridors so that they can fill in any
		// big gaps left by the corridor requirement.
		isCorridor = (( k > 375) ? false : (rand_percent(thisLevelProfile->corridorChance))); // boolean: is it a corridor?
		
		if (roomWasPlaced) { // We only randomize the crossRoom status if the previous room succeeded
			if (!isCorridor) {
				isCross = (rand_percent(thisLevelProfile->crossRoomChance));
			}
		}
		
		// Now choose a random wall cell of the given direction
		randIndex = rand_range(0, numberOfWalls[dir]);
		doorCandidateX = listOfWallsX[dir][randIndex];
		doorCandidateY = listOfWallsY[dir][randIndex];
		
		builtRoom = attemptRoom(doorCandidateX, doorCandidateY, dir, isCorridor, isCross, (isCorridor ? 10 : 5));
		// note: if it is a corridor, this call will also build a
		// non-corridor room at the end of the corridor, or it will fail.
		
		roomWasPlaced = (builtRoom != 0);
		
		if (builtRoom) {
			fromRoom = roomContainingCell(doorCandidateX + nbDirs[oppositeDirection(dir)][0],
										  doorCandidateY + nbDirs[oppositeDirection(dir)][1]);
			connectRooms(fromRoom, builtRoom, doorCandidateX, doorCandidateY, dir);
		}
		if (D_INSPECT_LEVELGEN && builtRoom) {
			dumpLevelToScreen();
			message("Room placed", true);
		}
	}
	//DEBUG printf("%i non-corridor rooms placed.", numberOfRooms);
	
	// now we add some loops to the otherwise simply connected network of rooms
	numLoops = 0;
	for (k=0; k<500 && numLoops <= thisLevelProfile->maxNumberOfLoops; k++) {
		
		dir = rand_range(0,3); // Are we going to try to loop up, down, left, or right? Pick randomly.
		
		// Now choose a random wall cell of the given direction
		randIndex = rand_range(0, numberOfWalls[dir]);
		doorCandidateX = listOfWallsX[dir][randIndex];
		doorCandidateY = listOfWallsY[dir][randIndex];
		
		x1 = doorCandidateX + nbDirs[dir][0];
		x2 = doorCandidateX + nbDirs[oppositeDirection(dir)][0];
		y1 = doorCandidateY + nbDirs[dir][1];
		y2 = doorCandidateY + nbDirs[oppositeDirection(dir)][1];
		
		if (coordinatesAreInMap(x1, y1) && coordinatesAreInMap(x2, y2)
			&& pmap[x1][y1].layers[DUNGEON] == FLOOR && pmap[x2][y2].layers[DUNGEON] == FLOOR &&
			distanceBetweenRooms(roomContainingCell(x1, y1), roomContainingCell(x2, y2)) > 2) {
			pmap[doorCandidateX][doorCandidateY].layers[DUNGEON] = (rand_percent(thisLevelProfile->doorChance) ? DOOR : FLOOR);
			pmap[doorCandidateX][doorCandidateY].flags |= DOORWAY;
			removeWallFromList(dir, doorCandidateX, doorCandidateY);
			connectRooms(roomContainingCell(x2, y2), roomContainingCell(x1, y1), doorCandidateX, doorCandidateY, dir);
			numLoops++;
		}
	}
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("Loops added.", true);
	}
	//DEBUG printf("\n%i loops created.", numLoops);
	//DEBUG logLevel();
	
	// Time to add lakes and chasms. Strategy is to generate a series of blob lakes of decreasing size. For each lake,
	// propose a position, and then check via a flood fill that the level would remain connected with that placement (i.e. that
	// each passable tile can still be reached). If not, make 9 more placement attempts before abandoning that lake
	// and proceeding to generate the next smaller one.
	// Canvas sizes start at 30x15 and decrease by 2x1 at a time down to a minimum of 20x10. Min generated size is always 4x4.
	
	// DEBUG logLevel();
	
	zeroOutGrid(unfilledLakeMap);
	for (lakeMaxHeight = 15, lakeMaxWidth = 30; lakeMaxHeight >=10; lakeMaxHeight--, lakeMaxWidth -= 2) { // lake generations
		
		cellularAutomata(4, 4, lakeMaxWidth, lakeMaxHeight, 55, "ffffftttt", "ffffttttt");
		
		for (k=0; k<20; k++) { // placement attempts
			
			// propose a position
			roomX = rand_range(1, DCOLS - blobWidth - 1);
			roomY = rand_range(1, DROWS - blobHeight - 1);
			
			if (checkLakePassability(roomX, roomY, connectedMap)) { // level with lake is completely connected
				//printf("Placed a lake!");
				
				// copy in lake
				for (i = roomX; i < roomX + blobWidth; i++) {
					for (j = roomY; j < roomY + blobHeight; j++) {
						if (connectedMap[i][j] == 101) { // It's in the lake.
							unfilledLakeMap[i][j] = true;
							pmap[i][j].layers[LIQUID] = FLOOR; // Just need a non-NOTHING value so createWreath() works.
							pmap[i][j].layers[DUNGEON] = FLOOR;
							if (pmap[i][j].flags & DOORWAY) {
								pmap[i][j].flags &= ~DOORWAY; // so that waypoint selection is not blocked by former doorways
							}
//							for (l=0; l<8; l++) {
//								if (coordinatesAreInMap(i+nbDirs[l][0], j+nbDirs[l][1]) &&
//									pmap[i+nbDirs[l][0]][j+nbDirs[l][1]].layers[DUNGEON] == GRANITE) {
//									pmap[i+nbDirs[l][0]][j+nbDirs[l][1]].layers[DUNGEON] = PERM_WALL;
//								}
//							}
						}
					}
				}
				
				if (D_INSPECT_LEVELGEN) {
					dumpLevelToScreen();
					hiliteGrid(unfilledLakeMap, &white, 75);
					message("Added a lake location.", true);
				}
				
				// DEBUG logLevel();
				break;
			}
		}
	}
	
	// Now fill all the lakes with various liquids
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (unfilledLakeMap[i][j]) {
				liquidType(&deepLiquid, &shallowLiquid, &shallowLiquidWidth);
				zeroOutGrid(wreathMap);
				fillLake(i, j, deepLiquid, 4, wreathMap, unfilledLakeMap);
				createWreath(shallowLiquid, shallowLiquidWidth, wreathMap);
				
				if (D_INSPECT_LEVELGEN) {
					dumpLevelToScreen();
					hiliteGrid(unfilledLakeMap, &white, 75);
					message("Lake filled.", true);
				}
			}
		}
	}
	
	// Now replace all left, right and bottom walls with TOP_WALLS for homogeneity
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[DUNGEON] == LEFT_WALL
				|| pmap[i][j].layers[DUNGEON] == RIGHT_WALL
				|| pmap[i][j].layers[DUNGEON] == BOTTOM_WALL) {
				pmap[i][j].layers[DUNGEON] = TOP_WALL;
			}
		}
	}
	
	// Now run the autoGenerators.
	runAutogenerators();
	
	// Now remove diagonal openings.
	do {
		diagonalCornerRemoved = false;
		for (i=0; i<DCOLS-1; i++) {
			for (j=0; j<DROWS-1; j++) {
				for (k=0; k<=1; k++) {
					if (!(tileCatalog[pmap[i + k][j].layers[DUNGEON]].flags & T_OBSTRUCTS_PASSABILITY)
						&& (tileCatalog[pmap[i + (1-k)][j].layers[DUNGEON]].flags & T_OBSTRUCTS_PASSABILITY)
						&& (tileCatalog[pmap[i + k][j+1].layers[DUNGEON]].flags & T_OBSTRUCTS_PASSABILITY)
						&& !(tileCatalog[pmap[i + (1-k)][j+1].layers[DUNGEON]].flags & T_OBSTRUCTS_PASSABILITY)) {
						
						diagonalCornerRemoved = true;
						if (rand_percent(50)) {
							x1 = i + (1-k);
							x2 = i + k;
							y1 = j;
						} else {
							x1 = i + k;
							x2 = i + (1-k);
							y1 = j + 1;
						}
						if (!(pmap[x1][y1].flags & HAS_MONSTER)) {
							if ((tileCatalog[pmap[x1][y1].layers[DUNGEON]].flags & T_OBSTRUCTS_PASSABILITY)
								&& (tileCatalog[pmap[x2][y1].layers[LIQUID]].flags & T_PATHING_BLOCKER)) {
								
								pmap[x1][y1].layers[LIQUID] = pmap[x2][y1].layers[LIQUID];
							}
							pmap[x1][y1].layers[DUNGEON] = FLOOR;
						}
					}
				}
			}
		}
	} while (diagonalCornerRemoved == true);
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("Diagonal openings removed.", true);
	}
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("Bridges added.", true);
	}
	
	// Now add some treasure machines.
	addMachines();
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("Machines added.", true);
	}
	
	// Now knock down the boundaries between similar lakes where possible.
	cleanUpLakeBoundaries();
	
	// Now add some bridges.
	while (buildABridge());
	
	// Now remove orphaned doors and upgrade some doors to secret doors
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (pmap[i][j].layers[DUNGEON] == DOOR) {
				if ((!cellHasTerrainFlag(i+1, j, T_OBSTRUCTS_PASSABILITY) || !cellHasTerrainFlag(i-1, j, T_OBSTRUCTS_PASSABILITY))
					&& (!cellHasTerrainFlag(i, j+1, T_OBSTRUCTS_PASSABILITY) || !cellHasTerrainFlag(i, j-1, T_OBSTRUCTS_PASSABILITY))) {
					// If there's passable terrain to the left or right, and there's passable terrain
					// above or below, then the door is orphaned and must be removed.
					pmap[i][j].layers[DUNGEON] = FLOOR;
				} else if ((cellHasTerrainFlag(i+1, j, T_PATHING_BLOCKER) ? 1 : 0)
						   + (cellHasTerrainFlag(i-1, j, T_PATHING_BLOCKER) ? 1 : 0)
						   + (cellHasTerrainFlag(i, j+1, T_PATHING_BLOCKER) ? 1 : 0)
						   + (cellHasTerrainFlag(i, j-1, T_PATHING_BLOCKER) ? 1 : 0) >= 3) {
					// If the door has three or more pathing blocker neighbors in the four cardinal directions,
					// then the door is orphaned and must be removed.
					pmap[i][j].layers[DUNGEON] = FLOOR;
				} else if (rand_percent(levelSpec.secretDoorChance)) {
					pmap[i][j].layers[DUNGEON] = SECRET_DOOR;
				}
			}
		}
	}
	
	// Now finish any exposed granite with walls and revert any unexposed walls to granite
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[DUNGEON] == GRANITE) {
				foundExposure = false;
				for (dir = 0; dir < 8 && !foundExposure; dir++) {
					x1 = i + nbDirs[dir][0];
					y1 = j + nbDirs[dir][1];
					if (coordinatesAreInMap(x1, y1)
						&& (!cellHasTerrainFlag(x1, y1, T_OBSTRUCTS_VISION) || !cellHasTerrainFlag(x1, y1, T_OBSTRUCTS_PASSABILITY))) {
                        
						pmap[i][j].layers[DUNGEON] = PERM_WALL;
						foundExposure = true;
					}
				}
			} else if (pmap[i][j].layers[DUNGEON] == TOP_WALL || pmap[i][j].layers[DUNGEON] == PERM_WALL) {
				foundExposure = false;
				for (dir = 0; dir < 8 && !foundExposure; dir++) {
					x1 = i + nbDirs[dir][0];
					y1 = j + nbDirs[dir][1];
					if (coordinatesAreInMap(x1, y1)
						&& (!cellHasTerrainFlag(x1, y1, T_OBSTRUCTS_VISION) || !cellHasTerrainFlag(x1, y1, T_OBSTRUCTS_PASSABILITY))) {
                        
						foundExposure = true;
					}
				}
				if (foundExposure == false) {
					pmap[i][j].layers[DUNGEON] = GRANITE;
				}
			}
		}
	}
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		message("Finishing touches added. Level has been generated.", true);
	}
}

// Scans the map in random order looking for a good place to build a bridge.
// If it finds one, it builds a bridge there, halts and returns true.
boolean buildABridge() {
	short i, j, k, l, i2, j2, nCols[DCOLS], nRows[DROWS];
	short bridgeRatioX, bridgeRatioY;
	boolean foundExposure;
	
	bridgeRatioX = (short) (100 + (100 + 100 * rogue.depthLevel / 9) * rand_range(10, 20) / 10);
	bridgeRatioY = (short) (100 + (400 + 100 * rogue.depthLevel / 18) * rand_range(10, 20) / 10);
	
	for (i=0; i<DCOLS; i++) {
		nCols[i] = i;
	}
	for (i=0; i<DROWS; i++) {
		nRows[i] = i;
	}
	shuffleList(nCols, DCOLS);
	shuffleList(nRows, DROWS);
	
	for (i2=1; i2<DCOLS-1; i2++) {
		i = nCols[i2];
		for (j2=1; j2<DROWS-1; j2++) {
			j = nRows[j2];
			if (!cellHasTerrainFlag(i, j, (T_CAN_BE_BRIDGED | T_PATHING_BLOCKER))
				&& !pmap[i][j].machineNumber) {
				
				// try a horizontal bridge
				foundExposure = false;
				for (k = i + 1;
					 k < DCOLS // Iterate across the prospective length of the bridge.
					 && !pmap[k][j].machineNumber // No bridges in machines.
					 && cellHasTerrainFlag(k, j, T_CAN_BE_BRIDGED)	// Candidate tile must be chasm.
					 && !cellHasTerrainFlag(k, j, T_OBSTRUCTS_PASSABILITY)	// Candidate tile cannot be a wall.
					 && cellHasTerrainFlag(k, j-1, (T_CAN_BE_BRIDGED | T_OBSTRUCTS_PASSABILITY))	// Only chasms or walls are permitted next to the length of the bridge.
					 && cellHasTerrainFlag(k, j+1, (T_CAN_BE_BRIDGED | T_OBSTRUCTS_PASSABILITY));
					 k++) {
					
					if (!cellHasTerrainFlag(k, j-1, T_OBSTRUCTS_PASSABILITY) // Can't run against a wall the whole way.
						&& !cellHasTerrainFlag(k, j+1, T_OBSTRUCTS_PASSABILITY)) {
						foundExposure = true;
					}
				}
				if (k < DCOLS
					&& (k - i > 3) // Can't have bridges shorter than 3 meters.
					&& foundExposure
					&& !cellHasTerrainFlag(k, j, T_PATHING_BLOCKER | T_CAN_BE_BRIDGED) // Must end on an unobstructed land tile.
					&& !pmap[k][j].machineNumber // Cannot end in a machine.
					&& 100 * pathingDistance(i, j, k, j, T_PATHING_BLOCKER) / (k - i) > bridgeRatioX) { // Must shorten the pathing distance enough.
					
					for (l=i+1; l < k; l++) {
						pmap[l][j].layers[LIQUID] = BRIDGE;
					}
					pmap[i][j].layers[SURFACE] = BRIDGE_EDGE;
					pmap[k][j].layers[SURFACE] = BRIDGE_EDGE;
					return true;
				}
				
				// try a vertical bridge
				foundExposure = false;
				for (k = j + 1;
					 k < DROWS
					 && !pmap[i][k].machineNumber
					 && cellHasTerrainFlag(i, k, T_CAN_BE_BRIDGED)
					 && !cellHasTerrainFlag(i, k, T_OBSTRUCTS_PASSABILITY)
					 && cellHasTerrainFlag(i-1, k, (T_CAN_BE_BRIDGED | T_OBSTRUCTS_PASSABILITY))
					 && cellHasTerrainFlag(i+1, k, (T_CAN_BE_BRIDGED | T_OBSTRUCTS_PASSABILITY));
					 k++) {
					
					if (!cellHasTerrainFlag(i-1, k, T_OBSTRUCTS_PASSABILITY)
						&& !cellHasTerrainFlag(i+1, k, T_OBSTRUCTS_PASSABILITY)) {
						foundExposure = true;
					}
				}
				if (k < DROWS
					&& (k - j > 3)
					&& foundExposure
					&& !cellHasTerrainFlag(i, k, T_OBSTRUCTS_PASSABILITY | T_CAN_BE_BRIDGED)
					&& !pmap[i][k].machineNumber // Cannot end in a machine.
					&& 100 * pathingDistance(i, j, i, k, T_PATHING_BLOCKER) / (k - j) > bridgeRatioY) {
					
					for (l=j+1; l < k; l++) {
						pmap[i][l].layers[LIQUID] = BRIDGE;
					}
					pmap[i][j].layers[SURFACE] = BRIDGE_EDGE;
					pmap[i][k].layers[SURFACE] = BRIDGE_EDGE;
					return true;
				}
			}
		}
	}
	return false;
}

void updateMapToShore() {
	short i, j;
	short **costMap;
	
	rogue.updatedMapToShoreThisTurn = true;
	
	costMap = allocDynamicGrid();
	
	// Calculate the map to shore for this level
	if (!rogue.mapToShore) {
		rogue.mapToShore = allocDynamicGrid();
		fillDynamicGrid(rogue.mapToShore, 0);
	}
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)) {
				costMap[i][j] = PDS_OBSTRUCTION;
				rogue.mapToShore[i][j] = 30000;
			} else {
				costMap[i][j] = 1;
				rogue.mapToShore[i][j] = (cellHasTerrainFlag(i, j, T_LAVA_INSTA_DEATH | T_IS_DEEP_WATER | T_AUTO_DESCENT)
										  && !cellHasTerrainFlag(i, j, T_IS_SECRET)) ? 30000 : 0;
			}
		}
	}
	dijkstraScan(rogue.mapToShore, costMap, true);
	freeDynamicGrid(costMap);
}

void augmentAccessMapWithWaypoint(char accessMap[DCOLS][DROWS], boolean WPactive[MAX_WAYPOINTS], short x, short y) {
	short i, j;
	char grid[DCOLS][DROWS];
	
	zeroOutGrid(grid);
	getFOVMask(grid, x, y, WAYPOINT_SIGHT_RADIUS, T_WAYPOINT_BLOCKER, DOORWAY, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j] && accessMap[i][j] != 3) { // i.e. if it's in the FOV mask and is not a wall/trap/lava/water
				accessMap[i][j] = 1;
			}
		}
	}
	accessMap[x][y] = 1;
	
	for (i = 0; i < numberOfWaypoints; i++) {
		if (!WPactive[i] && (accessMap[waypoints[i].x][waypoints[i].y] == 1 || accessMap[waypoints[i].x][waypoints[i].y] == 2)) {
			WPactive[i] = true;
			if (waypoints[i].pointsTo[0] && waypoints[i].pointsTo[1]) {
				accessMap[waypoints[i].pointsTo[0]][waypoints[i].pointsTo[1]] = 1;
			}
			augmentAccessMapWithWaypoint(accessMap, WPactive, waypoints[i].x, waypoints[i].y); // recurse!
		}
	}
}

short findEdges(char accessMap[DCOLS][DROWS], char centralityMap[DCOLS][DROWS]) {
	short i, j, count = 0;
	enum directions dir;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (accessMap[i][j] == 2) {
				count += centralityMap[i][j];
			} else if (accessMap[i][j] == 1) {
				for (dir = 0; dir < 4; dir++) {
					if (accessMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] == 0) {
						accessMap[i][j] = 2;
						count += centralityMap[i][j];
						break;
					}
				}
			}
		}
	}
	return count;
}

boolean waypointLinkedTo(short i, short j) {
	short k;
	if (waypoints[i].pointsTo[0] == waypoints[j].x && waypoints[i].pointsTo[1] == waypoints[j].y) {
		return true;
	}
	for (k = 0; k < waypoints[i].connectionCount; k++) {
		if (waypoints[i].connection[k][0] == waypoints[j].x && waypoints[i].connection[k][1] == waypoints[j].y) {
			return true;
		}
	}
	return false;
}

void setUpWaypoints() {
	short i, j, k, x, y, edgesWeightCount, randIndex;
	room *theRoom;
	char accessMap[DCOLS][DROWS], centralityMap[DCOLS][DROWS], grid[DCOLS][DROWS];
	boolean WPactive[MAX_WAYPOINTS];
	
	numberOfWaypoints = 0;
	
	for (i = 0; i < MAX_WAYPOINTS; i++) {
		waypoints[i].connectionCount = 0;
		waypoints[i].pointsTo[0] = 0;
		waypoints[i].pointsTo[1] = 0;
		WPactive[i] = false;
	}
	
	// 1. generate the waypoints.
	
	// initialize the accessMap
	
	// set up access map
	// key: 0 = passable but not accessible
	//		1 = accessible
	//		2 = accessible and adjacent to type 0
	//		3 = wall, pit, lava, deep water, pit trap
	
	zeroOutGrid(accessMap);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, T_WAYPOINT_BLOCKER) || (pmap[i][j].flags & DOORWAY)) {
				accessMap[i][j] = 3;
			}
		}
	}
	
	// DEBUG logBuffer(accessMap);
	
	// Set up centrality map, to be used as probability weights for picking waypoint candidates.
	// The idea here is that we want a weight map of how many cells are visible from a particular cell.
	// Could just do a FOV from each cell and add it all up, but that would be very expensive.
	// Instead, randomly choose N passable points from around the map, compute FOV for them,
	// and add all the FOV maps. So it's really a map of how many of these random points are visible
	// from any particular cell -- a monte carlo approximation which approaches the right proportions
	// as N->infty. I use N=200.
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			centralityMap[i][j] = 1; // starting it at 0 makes the generation prone to failure if unlucky
		}
	}
	for (k=0; k<200; k++) {
		x = rand_range(1, DCOLS - 2);
		y = rand_range(1, DROWS - 2);
		if (accessMap[x][y] == 3) { // i.e. passable
			zeroOutGrid(grid);
			getFOVMask(grid, x, y, WAYPOINT_SIGHT_RADIUS, T_WAYPOINT_BLOCKER, DOORWAY, false);
			for (i=1; i<DCOLS - 1; i++) {
				for (j=1; j<DROWS - 1; j++) {
					if (grid[i][j]) {
						centralityMap[i][j]++;
					}
				}
			}
		}
	}
	
	// DEBUG logBuffer(accessMap);
	
	//	a. place them on either side of doors and make them point to each other.
	
	// cycle through the rooms
	for (theRoom = rooms->nextRoom; theRoom != NULL; theRoom = theRoom->nextRoom) {
		// cycle through the doors of the room
		for (i = 0; i < theRoom->numberOfSiblings; i++) {
			
			// waypoint is immediately inside the door
			waypoints[numberOfWaypoints].x = theRoom->doors[i].x +
			nbDirs[oppositeDirection(theRoom->doors[i].direction)][0];
			waypoints[numberOfWaypoints].y = theRoom->doors[i].y +
			nbDirs[oppositeDirection(theRoom->doors[i].direction)][1];
			
			//augmentAccessMapWithWaypoint(accessMap, WPactive, waypoints[numberOfWaypoints].x, waypoints[numberOfWaypoints].y);
			
			// DEBUG logBuffer(accessMap);
			
			// waypoint points to the cell immediately on the other side of the door
			waypoints[numberOfWaypoints].pointsTo[0] = theRoom->doors[i].x + nbDirs[theRoom->doors[i].direction][0];
			waypoints[numberOfWaypoints].pointsTo[1] = theRoom->doors[i].y + nbDirs[theRoom->doors[i].direction][1];
			
			// link waypoint to the other waypoints of the room, if they're not too far apart
			waypoints[numberOfWaypoints].connectionCount = 0;
			for (j = 0; j < theRoom->numberOfSiblings; j++) {
				if (i != j // don't want to link the waypoint to itself
					&& (distanceBetween(waypoints[numberOfWaypoints].x, waypoints[numberOfWaypoints].y,
											  theRoom->doors[j].x, theRoom->doors[j].y) < 21
						|| specifiedPathBetween(waypoints[numberOfWaypoints].x, waypoints[numberOfWaypoints].y,
												theRoom->doors[j].x, theRoom->doors[j].y, T_WAYPOINT_BLOCKER, DOORWAY))
					&& theRoom->width <= 20) { // not a cave
					waypoints[numberOfWaypoints].connection[waypoints[numberOfWaypoints].connectionCount][0]
					= theRoom->doors[j].x +	nbDirs[oppositeDirection(theRoom->doors[j].direction)][0];
					
					waypoints[numberOfWaypoints].connection[waypoints[numberOfWaypoints].connectionCount][1]
					= theRoom->doors[j].y +	nbDirs[oppositeDirection(theRoom->doors[j].direction)][1];
					
					waypoints[numberOfWaypoints].connectionCount++;
				}
			}
			
			numberOfWaypoints++;
		}
	}
	
	// b. Place them near anywhere that is not within line of sight of a waypoint. This will cover caves and also certain
	// rooms in which there is an alcove not visible from the doors.
	
	// Activate the first waypoint! This process of activation is to ensure that all the WP's are connected.
	WPactive[0] = true;
	accessMap[waypoints[0].pointsTo[0]][waypoints[0].pointsTo[1]] = 1;
	augmentAccessMapWithWaypoint(accessMap, WPactive, waypoints[0].x, waypoints[0].y);
	
	edgesWeightCount = findEdges(accessMap, centralityMap);
	// DEBUG logBuffer(accessMap);
	while (edgesWeightCount) {
		randIndex = rand_range(1, edgesWeightCount);
		
		for (i=0; i<DCOLS; i++)	{
			for (j=0; j<DROWS; j++) {
				if (accessMap[i][j] == 2) {
					randIndex -= centralityMap[i][j];
					if (randIndex <= 0) {
						// this is the edge we've chosen. Add a waypoint.
						augmentAccessMapWithWaypoint(accessMap, WPactive, i, j);
						
						waypoints[numberOfWaypoints].x = i;
						waypoints[numberOfWaypoints].y = j;
						numberOfWaypoints++;
						
						// crappy way to break out of both loops:
						i = DCOLS;
						j = DROWS;
					}
				}
			}
		}
		
		edgesWeightCount = findEdges(accessMap, centralityMap);
		// DEBUG logBuffer(accessMap);
		// DEBUG printf("\n%i waypoints in the above map\n", numberOfWaypoints);
	}
	
	// 2. link the waypoints together.
	
	// i is the waypoint we're connecting
	for (i=0; i < numberOfWaypoints; i++) {
		x = waypoints[i].x;
		y = waypoints[i].y;
		
		zeroOutGrid(grid);
		getFOVMask(grid, x, y, WAYPOINT_SIGHT_RADIUS, T_WAYPOINT_BLOCKER, DOORWAY, false);
		
		// j is the waypoint to which we're contemplating linking it
		for (j=0; j < numberOfWaypoints && waypoints[i].connectionCount <= 10; j++) {
			if (i == j) {
				continue; // don't link to itself
			}
			
			// verify that the waypoint isn't already connected to the target
			if (!waypointLinkedTo(i, j)) {
				// check to make sure the path between is passable
				if (grid[waypoints[j].x][waypoints[j].y]) {
					waypoints[i].connection[waypoints[i].connectionCount][0] = waypoints[j].x;
					waypoints[i].connection[waypoints[i].connectionCount][1] = waypoints[j].y;					
					waypoints[i].connectionCount++;
					
					// create reciprocal link if it doesn't already exist (important to avoid one-way links)
					if (!waypointLinkedTo(j, i)) {
						waypoints[j].connection[waypoints[j].connectionCount][0] = waypoints[i].x;
						waypoints[j].connection[waypoints[j].connectionCount][1] = waypoints[i].y;					
						waypoints[j].connectionCount++;
					}
				}
			}
		}
	}
}

void zeroOutGrid(char grid[DCOLS][DROWS]) {
	short i, j;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			grid[i][j] = 0;
		}
	}
}

void copyGrid(char to[DCOLS][DROWS], char from[DCOLS][DROWS]) {
	short i, j;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			to[i][j] = from[i][j];
		}
	}
}

void createWreath(short shallowLiquid, short wreathWidth, char wreathMap[DCOLS][DROWS]) {
	short i, j, k, l;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (wreathMap[i][j]) {
				for (k = i-wreathWidth; k<= i+wreathWidth; k++) {
					for (l = j-wreathWidth; l <= j+wreathWidth; l++) {
						if (coordinatesAreInMap(k, l) && pmap[k][l].layers[LIQUID] == NOTHING
							&& (i-k)*(i-k) + (j-l)*(j-l) <= wreathWidth*wreathWidth) {
							pmap[k][l].layers[LIQUID] = shallowLiquid;
							if (pmap[k][l].layers[DUNGEON] == DOOR) {
								pmap[k][l].layers[DUNGEON] = FLOOR;
							}
						}
					}
				}
			}
		}
	}
}

short oppositeDirection(short theDir) {
	switch (theDir) {
	case UP:
		return DOWN;
	case DOWN:
		return UP;
	case LEFT:
		return RIGHT;
	case RIGHT:
		return LEFT;
	case UPRIGHT:
		return DOWNLEFT;
	case DOWNLEFT:
		return UPRIGHT;
	case UPLEFT:
		return DOWNRIGHT;
	case DOWNRIGHT:
		return UPLEFT;
	default:
		return -1;
	}
}

void connectRooms(room *fromRoom, room *toRoom, short x, short y, short direction) {
	short f = fromRoom->numberOfSiblings, t = toRoom->numberOfSiblings;
    
	fromRoom->doors[f].x = x;
	toRoom->doors[t].x = x;
	fromRoom->doors[f].y = y;
	toRoom->doors[t].y = y;
	fromRoom->doors[f].direction = direction;
	toRoom->doors[t].direction = oppositeDirection(direction);
	fromRoom->siblingRooms[f] = toRoom;
	toRoom->siblingRooms[t] = fromRoom;
	fromRoom->numberOfSiblings++;
	toRoom->numberOfSiblings++;
}

room *allocateRoom(short roomX, short roomY, short width, short height,
				   short roomX2, short roomY2, short width2, short height2) {
	room *theRoom;
    
	theRoom = (room *) malloc( sizeof(room) );
	memset(theRoom, '\0', sizeof(room) );
	theRoom->nextRoom = rooms->nextRoom;
	rooms->nextRoom = theRoom;
	theRoom->numberOfSiblings = 0;
	theRoom->roomX = roomX;
	theRoom->roomY = roomY;
	theRoom->width = width;
	theRoom->height = height;
	theRoom->roomX2 = roomX2;
	theRoom->roomY2 = roomY2;
	theRoom->width2 = width2;
	theRoom->height2 = height2;
	return theRoom;
}

room *roomContainingCell(short x, short y) {
	room *theRoom;
	short i;
	for (theRoom = rooms->nextRoom; theRoom != NULL; theRoom = theRoom->nextRoom) {
		if ((theRoom->roomX <= x && theRoom->roomX + theRoom->width > x &&
			 theRoom->roomY <= y && theRoom->roomY + theRoom->height > y) ||
			(theRoom->roomX2 <= x && theRoom->roomX2 + theRoom->width2 > x &&
			 theRoom->roomY2 <= y && theRoom->roomY2 + theRoom->height2 > y)) {
			return theRoom;
		}
		for (i=0; i < theRoom->numberOfSiblings; i++) {
			if (theRoom->doors[i].x == x && theRoom->doors[i].y == y) {
				return theRoom;
			}
		}
	}
	return NULL;
}

// Tries to build a room starting at a door with the specified location and direction and room type.
room *attemptRoom(short doorCandidateX, short doorCandidateY,
					short direction, boolean isCorridor, boolean isCross, short numAttempts) {
	
	// roomX2, roomY2, roomWidth2 and roomHeight2 are for cross-shaped rooms
	short i, roomX, roomY, roomWidth, roomHeight, roomX2, roomY2, roomWidth2, roomHeight2, newDoorX, newDoorY;
	room *builtRoom, *nextRoom;
	
	roomX		= 0;
	roomY		= 0;
	roomX2		= 0;
	roomY2		= 0;
	roomWidth	= 0;
	roomWidth2	= 0;
	roomHeight	= 0;
	roomHeight2	= 0;
	newDoorX	= 0;
	newDoorY	= 0;
	
	// Try numAttempts times to place a room adjacent to the previous one with the door candidate joining them.
	for(i=0; i<numAttempts; i++) {
		if (!isCorridor) {
			if (isCross) { // cross room
				if (direction == UP || direction == DOWN) {
					roomWidth = rand_range(levelSpec.crossRoomMinWidth, levelSpec.crossRoomMaxWidth);
					roomHeight = rand_range(levelSpec.roomMinHeight, levelSpec.roomMaxHeight);
					roomWidth2 = rand_range(levelSpec.roomMinWidth, levelSpec.roomMaxWidth);
					roomHeight2 = rand_range(levelSpec.crossRoomMinHeight, levelSpec.crossRoomMaxHeight);
				} else {
					roomWidth = rand_range(levelSpec.roomMinWidth, levelSpec.roomMaxWidth);
					roomHeight = rand_range(levelSpec.crossRoomMinHeight, levelSpec.crossRoomMaxHeight);
					roomWidth2 = rand_range(levelSpec.crossRoomMinWidth, levelSpec.crossRoomMaxWidth);
					roomHeight2 = rand_range(levelSpec.roomMinHeight, levelSpec.crossRoomMaxHeight);
				}
			} else { // rectangular room
				roomWidth = rand_range(levelSpec.roomMinWidth, levelSpec.roomMaxWidth);
				roomHeight = rand_range(levelSpec.roomMinHeight, levelSpec.roomMaxHeight);
			}
		}
		
		switch (direction) {
			case UP:
				if (isCorridor) {
					roomWidth = CORRIDOR_WIDTH;
					roomHeight = rand_range(levelSpec.vertCorrMinLength, levelSpec.vertCorrMaxLength);
					newDoorX = doorCandidateX;
					newDoorY = doorCandidateY - roomHeight - 1;
				}
				roomX = rand_range(max(0, doorCandidateX - (roomWidth - 1)), min(DCOLS, doorCandidateX));
				roomY = (doorCandidateY - roomHeight);
				if (isCross) {
					roomX2 = (roomX + (roomWidth / 2) + rand_range(0, 2) + rand_range(0, 2) - 3) - (roomWidth2 / 2);
					roomY2 = (doorCandidateY - roomHeight2 - (rand_range(0, 2) + rand_range(0, 1)));
				}
				break;
			case DOWN:
				if (isCorridor) {
					roomWidth = CORRIDOR_WIDTH;
					roomHeight = rand_range(levelSpec.vertCorrMinLength, levelSpec.vertCorrMaxLength);
					newDoorX = doorCandidateX;
					newDoorY = doorCandidateY + roomHeight + 1;
				}
				roomX = rand_range(max(0, doorCandidateX - (roomWidth - 1)), min(DCOLS, doorCandidateX));
				roomY = (doorCandidateY + 1);
				if (isCross) {
					roomX2 = (roomX + (roomWidth / 2) + rand_range(0, 2) + rand_range(0, 2) - 3) - (roomWidth2 / 2);
					roomY2 = (doorCandidateY + 1 + (rand_range(0, 2) + rand_range(0, 1)));
				}
				break;
			case LEFT:
				if (isCorridor) {
					roomHeight = CORRIDOR_WIDTH;
					roomWidth = rand_range(levelSpec.horCorrMinLength, levelSpec.horCorrMaxLength);
					newDoorX = doorCandidateX - roomWidth - 1;
					newDoorY = doorCandidateY;
				}
				roomX = (doorCandidateX - roomWidth);
				roomY = rand_range(max(0, doorCandidateY - (roomHeight - 1)), min(DCOLS, doorCandidateY));
				if (isCross) {
					roomX2 = (doorCandidateX - roomWidth2 - (rand_range(0, 2) + rand_range(0, 2)));
					roomY2 = (roomY + (roomHeight / 2) + (rand_range(-1, 1) + rand_range(-1, 1))) - (roomHeight2 / 2);
				}
				break;
			case RIGHT:
				if (isCorridor) {
					roomHeight = CORRIDOR_WIDTH;
					roomWidth = rand_range(levelSpec.horCorrMinLength, levelSpec.horCorrMaxLength);
					newDoorX = doorCandidateX + roomWidth + 1;
					newDoorY = doorCandidateY;
				}
				roomX = (doorCandidateX + 1);
				roomY = rand_range(max(0, doorCandidateY - (roomHeight - 1)), min(DCOLS, doorCandidateY));
				if (isCross) {
					roomX2 = (doorCandidateX + 1 + rand_range(0, 2) + rand_range(0, 2));
					roomY2 = (roomY + (roomHeight / 2) + (rand_range(-1, 1) + rand_range(-1, 1))) - (roomHeight2 / 2);
				}
				break;
		}
		if (checkRoom(roomX, roomY, roomWidth, roomHeight) &&
			(!isCross || checkRoom(roomX2, roomY2, roomWidth2, roomHeight2))) {
			// there is space for the room
			if (isCorridor) {
				// proceed only if there is space to build a non-corridor room at the end
				if ((nextRoom = attemptRoom(newDoorX, newDoorY, direction, false,
										   rand_percent(thisLevelProfile->crossRoomChance), 15))) {
					// success; build corridor (room at end has already been built)
					carveRectangle(roomX, roomY, roomWidth, roomHeight);
					markRectangle(roomX, roomY, roomWidth, roomHeight);
					pmap[doorCandidateX][doorCandidateY].layers[DUNGEON] = (rand_percent(thisLevelProfile->doorChance) ? DOOR : FLOOR);
					pmap[doorCandidateX][doorCandidateY].flags |= DOORWAY;
					removeWallFromList(direction, doorCandidateX, doorCandidateY);
					builtRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, 0, 0, 0, 0);
					connectRooms(builtRoom, nextRoom, newDoorX, newDoorY, direction);
					
					//DEBUG printf("Door candidate (corr): (%i, %i):\n Roomloc: (%i, %i) Width: %i, Height: %i\n",
					//			 doorCandidateX, doorCandidateY, roomX, roomY, roomWidth, roomHeight);
					//DEBUG logLevel();
					
					return builtRoom;
				} else {
					// couldn't build room at the end of the corridor; abort corridor.
					return NULL;
				}
			} else {
				if (isCross) {
					carveRectangle(roomX, roomY, roomWidth, roomHeight);
					carveRectangle(roomX2, roomY2, roomWidth2, roomHeight2);
					markRectangle(roomX, roomY, roomWidth, roomHeight);
					markRectangle(roomX2, roomY2, roomWidth2, roomHeight2);
					builtRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, roomX2, roomY2, roomWidth2, roomHeight2);
				} else {
					carveRectangle(roomX, roomY, roomWidth, roomHeight);
					markRectangle(roomX, roomY, roomWidth, roomHeight);
					builtRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, 0, 0, 0, 0);
				}
				
				pmap[doorCandidateX][doorCandidateY].layers[DUNGEON] = (rand_percent(thisLevelProfile->doorChance) ? DOOR : FLOOR);
				pmap[doorCandidateX][doorCandidateY].flags |= DOORWAY;
				removeWallFromList(direction, doorCandidateX, doorCandidateY);
				
				//DEBUG printf("Door candidate: (%i, %i):\n", doorCandidateX, doorCandidateY);
				//DEBUG logLevel();
				
				numberOfRooms++;
				
				return builtRoom;
				break;
			}
		}
	}
	
	// couldn't find space for the room after numAttempts attempts.
	return NULL;
}

// Checks to see if a given roomX, roomY, roomWidth, and roomHeight describe a valid room.
// This check has two components: first, is the room within the bounds of the dungeon?
// Second, are all of its cells undeveloped (i.e. made of granite)?
boolean checkRoom(short roomX, short roomY, short roomWidth, short roomHeight) {
	int i, j;
	
	if ((roomX < 1) || (roomX + roomWidth >= DCOLS) || (roomY < 1) || (roomY + roomHeight >= DROWS)) {
		return false;
	}
	
	for (i=roomX; i < (roomX + roomWidth); i++) {
		for (j=roomY; j < (roomY + roomHeight); j++) {
			if (pmap[i][j].layers[DUNGEON] != GRANITE) {
				return false;
			}
		}
	}
	return true;
}

// Carves a rectangular hole into the dungeon by marking contained GRANITE tiles as FLOOR
void carveRectangle(short roomX, short roomY, short roomWidth, short roomHeight) {
	short i, j;
	for ( i=roomX; i<(roomX + roomWidth); i++) {
		for ( j=roomY; j<(roomY + roomHeight); j++) {
			if (pmap[i][j].layers[DUNGEON] == GRANITE) {
				pmap[i][j].layers[DUNGEON] = FLOOR;
				//pmap[i][j].layers[LIQUID] = rand_percent(10) ? DEEP_WATER : NOTHING;
			}
		}
	}
}

// Marks the surrounding granite as various WALL tiles, if indeed they are walls
void markRectangle(short roomX, short roomY, short roomWidth, short roomHeight) {
	short i;
	
	// Fill top and bottom walls with WALL tiles
	for ( i=roomX; i<(roomX + roomWidth); i++) {
		if (pmap[i][roomY - 1].layers[DUNGEON] == GRANITE) {
			pmap[i][roomY - 1].layers[DUNGEON] = TOP_WALL;
			addWallToList(UP, i, roomY-1);
		}
		
		if (pmap[i][roomY + roomHeight].layers[DUNGEON] == GRANITE) {
			pmap[i][roomY + roomHeight].layers[DUNGEON] = BOTTOM_WALL;
			addWallToList(DOWN, i, roomY + roomHeight);
		}
	}
	
	// Fill left and right walls with WALL tiles
	for ( i=roomY; i<(roomY + roomHeight); i++) {
		if (pmap[roomX - 1][i].layers[DUNGEON] == GRANITE) {
			pmap[roomX - 1][i].layers[DUNGEON] = LEFT_WALL;
			addWallToList(LEFT, roomX - 1, i);
		}
		
		if (pmap[roomX + roomWidth][i].layers[DUNGEON] == GRANITE) {
			pmap[roomX + roomWidth][i].layers[DUNGEON] = RIGHT_WALL;
			addWallToList(RIGHT, roomX + roomWidth, i);
		}
	}
	
	// Set the corners of the room to PERM_WALL tiles
	if ((pmap[roomX - 1][roomY - 1].layers[DUNGEON]) == GRANITE) {
		pmap[roomX - 1][roomY - 1].layers[DUNGEON] = PERM_WALL;
	}
	if (pmap[roomX + roomWidth][roomY - 1].layers[DUNGEON] == GRANITE) {
		pmap[roomX + roomWidth][roomY - 1].layers[DUNGEON] = PERM_WALL;
	}
	if (pmap[roomX - 1][roomY + roomHeight].layers[DUNGEON] == GRANITE) {
		pmap[roomX - 1][roomY + roomHeight].layers[DUNGEON] = PERM_WALL;
	}
	if (pmap[roomX + roomWidth][roomY + roomHeight].layers[DUNGEON] == GRANITE) {
		pmap[roomX + roomWidth][roomY + roomHeight].layers[DUNGEON] = PERM_WALL;
	}
}

void addWallToList(short direction, short xLoc, short yLoc) {
	wallNum++;
//	if (rogue.depthLevel == 4 && wallNum == 630) {
//		printf("\n%lu Adding a wall: direction %i at (%i, %i).", wallNum++, direction, xLoc, yLoc);
//	}
	numberOfWalls[direction]++;
	listOfWallsX[direction][numberOfWalls[direction]] = xLoc;
	listOfWallsY[direction][numberOfWalls[direction]] = yLoc;
}

// returns the minimum number of rooms one must travel to get from the fromRoom to the toRoom
short distanceBetweenRooms(room *fromRoom, room *toRoom) {
	room *theRoom, *theNeighbor;
	boolean somethingChanged;
	short i;
	
	for (theRoom = rooms->nextRoom; theRoom != NULL; theRoom = theRoom->nextRoom) {
		theRoom->pathNumber = numberOfRooms + 5; // set it high
	}
	toRoom->pathNumber = 0;
	
	do {
		somethingChanged = false;
		for (theRoom=rooms->nextRoom; theRoom != NULL; theRoom=theRoom->nextRoom) {
			for (i=0; i < theRoom->numberOfSiblings; i++) {
				theNeighbor = theRoom->siblingRooms[i];
				if (theNeighbor->pathNumber + 1 < theRoom->pathNumber) {
					theRoom->pathNumber = theNeighbor->pathNumber + 1;
					somethingChanged = true;
				}
			}
		}
	} while (somethingChanged);
	return fromRoom->pathNumber;
}

void removeWallFromList(short direction, short xLoc, short yLoc) {
	int i;
	boolean locatedTheWall = false;
	
	for (i=0; i<numberOfWalls[direction]; i++) {
		if (listOfWallsX[direction][i] == xLoc && listOfWallsY[direction][i] == yLoc) {
			locatedTheWall = true;
			break;
		}
	}
	if (locatedTheWall) {
		for (; i < numberOfWalls[direction] - 1; i++) {
			listOfWallsX[direction][i] = listOfWallsX[direction][i + 1];
			listOfWallsY[direction][i] = listOfWallsY[direction][i + 1];
		}
		
		numberOfWalls[direction]--;
		
	} else {
		//DEBUG printf("Failed to remove wall at %i, %i", xLoc, yLoc);
	}
}

// loads up buffer[][] with a cellular automata. Returns nothing directly, but saves
// its results in globals buffer[][], topBlobMinX, topBlobMinY, blobWidth, and blobHeight.
void cellularAutomata(short minBlobWidth, short minBlobHeight,
					  short maxBlobWidth, short maxBlobHeight, short percentSeeded,
					  char birthParameters[9], char survivalParameters[9]) {
	short i, j, k, l, neighborCount, tempX, tempY;
	short blobNumber, blobSize, topBlobSize, topBlobNumber;
	short topBlobMaxX, topBlobMaxY;
	//short buffer2[maxBlobWidth][maxBlobHeight]; // buffer[][] is already a global short array
	char buffer2[DCOLS][DROWS]; // buffer[][] is already a global short array
	boolean foundACellThisLine;
	
	for( i=0; i<DCOLS; i++ ) {
		for( j=0; j<DROWS; j++ ) {	
			buffer2[i][j] = 0; // prophylactic
		}
	}
	
	// Generate caves until they satisfy the MIN_CAVE_WIDTH and MIN_CAVE_HEIGHT restraints
	do {
		blobNumber = 2;
		
		// These are best-of variables; start them out at worst-case values.
		topBlobSize = 0;
		topBlobNumber = 0;
		topBlobMinX=maxBlobWidth;
		topBlobMaxX=0;
		topBlobMinY=maxBlobHeight;
		topBlobMaxY=0;
		
		// clear buffer
		for( i=0; i<DCOLS; i++ ) {
			for( j=0; j<DROWS; j++ ) {	
				buffer[i][j] = 0;
			}
		}
		
		// Fill relevant portion with 45% wall (0), 55% floor (1)
		for( i=0; i<maxBlobWidth; i++ ) {
			for( j=0; j<maxBlobHeight; j++ ) {	
				buffer[i][j] = (rand_percent(percentSeeded) ? 1 : 0);
			}
		}
		
		//printf("\nRandom starting grid:\n");
		//logBuffer(buffer);
		
		// Five iterations of automata
		for (k=0; k<5; k++) {
			
			// copy buffer into buffer2
			for( i=0; i<maxBlobWidth; i++ ) {
				for( j=0; j<maxBlobHeight; j++ ) {	
					buffer2[i][j] = buffer[i][j];
				}
			}
			
			//logBuffer(buffer2);
			
			for( i=0; i<maxBlobWidth; i++ ) {
				for( j=0; j<maxBlobHeight; j++ ) {
					neighborCount = 0;
					for (l=0; l<8; l++) {
						tempX = i + nbDirs[l][0];
						tempY = j + nbDirs[l][1];
						if (tempX >= 0 && tempX < maxBlobWidth
							&& tempY >= 0 && tempY < maxBlobHeight
							&& buffer2[tempX][tempY] == 1) { // floor
							neighborCount++;
						}
					}
					if (!buffer2[i][j] && birthParameters[neighborCount] == 't') {
						buffer[i][j] = 1;	// birth
					} else if (buffer2[i][j] && survivalParameters[neighborCount] == 't') {
											// survival
					} else {
						buffer[i][j] = 0;	// death
					}
				}
			}
			//logBuffer(buffer);
		}
		
		//logBuffer(buffer);
		
		// Fill each blob with its own number, starting with 2 (since 1 means floor), and keeping track of the biggest:
		for( i=0; i<maxBlobWidth; i++ ) {
			for( j=0; j<maxBlobHeight; j++ ) {
				if (buffer[i][j] == 1) { // an unmarked blob
					
					// Marks all the cells and returns the total size:
					blobSize = markBlobCellAndIterate(i, j, blobNumber);
					
					//DEBUG printf("Blob %i is size %i.\n", blobNumber, blobSize);
					if (blobSize > topBlobSize) { // if this blob is a new record
						topBlobSize = blobSize;
						topBlobNumber = blobNumber;
					}
					blobNumber++;
				}
			}
		}
		
		// DEBUG logBuffer(buffer);
		
		// Figure out the top blob's height and width:
		// First find the max & min x:
		foundACellThisLine = false;
		for( i=0; i<maxBlobWidth; i++ ) {
			for( j=0; j<maxBlobHeight; j++ ) {
				if (buffer[i][j] == topBlobNumber) {
					foundACellThisLine = true;
					break;
				}
			}
			if (foundACellThisLine) {
				if (i < topBlobMinX) {
					topBlobMinX = i;
				}
				if (i > topBlobMaxX) {
					topBlobMaxX = i;
				}
			}
			foundACellThisLine = false;
		}
		
		// then the max & min y:
		for( j=0; j<maxBlobHeight; j++ ) {
			for( i=0; i<maxBlobWidth; i++ ) {
				if (buffer[i][j] == topBlobNumber) {
					foundACellThisLine = true;
					break;
				}
			}
			if (foundACellThisLine) {
				if (j < topBlobMinY) {
					topBlobMinY = j;
				}
				if (j > topBlobMaxY) {
					topBlobMaxY = j;
				}
			}
			foundACellThisLine = false;
		}
		// DEBUG printf("Blob #%i: minX %i, maxX %i, minY %i, maxY %i, size %i.\n", topBlobNumber,
		//			 topBlobMinX, topBlobMaxX, topBlobMinY, topBlobMaxY, topBlobSize);
		
		blobWidth =		(topBlobMaxX - topBlobMinX) + 1;
		blobHeight =	(topBlobMaxY - topBlobMinY) + 1;
		
	} while ((topBlobMaxX - topBlobMinX) <= minBlobWidth ||
			 (topBlobMaxY - topBlobMinY) <= minBlobHeight);
	
	// Replace the winning blob with 1's, and everything else with 0's:
	for( i=0; i<maxBlobWidth; i++ ) {
		for( j=0; j<maxBlobHeight; j++ ) {
			if (buffer[i][j] == topBlobNumber) {
				buffer[i][j] = 1;
			} else {
				buffer[i][j] = 0;
			}
		}
	}
}


// Uses four iterations of cellular automata with 55% floor initial pattern and a B678/S45678 ruleset.
// Chooses the biggest contiguous block after that process and then copies it into the tmap[][] array.
void generateCave() {
	short i, j, k;
	short roomX, roomY;
	short bufferX, bufferY, dungeonX, dungeonY;
	
	bufferX = bufferY = dungeonX = dungeonY = 0; // prophylactic
	
	cellularAutomata(CAVE_MIN_WIDTH, CAVE_MIN_HEIGHT, DCOLS - 2, DROWS - 2, 55, "ffffffttt", "ffffttttt");
	
	// position the new room
	roomX = rand_range(1, DCOLS - blobWidth - 1);
	roomY = rand_range(1, DROWS - blobHeight - 1);
	
	// and copy it to tmap[][]
	
	for( i = 0; i < blobWidth; i++ ) {
		for( j=0; j < blobHeight; j++ ) {
			bufferX = i + topBlobMinX;
			bufferY = j + topBlobMinY;
			dungeonX = i + roomX;
			dungeonY = j + roomY;
			if (buffer[bufferX][bufferY]) {
				pmap[dungeonX][dungeonY].layers[DUNGEON] = FLOOR;
			}
		}
	}
	
	// Mark the edges as walls
	for( i=0; i < blobWidth; i++ ) {
		for( j=0; j < blobHeight; j++ ) {
			bufferX = i + topBlobMinX;
			bufferY = j + topBlobMinY;
			dungeonX = i + roomX;
			dungeonY = j + roomY;
			if (buffer[bufferX][bufferY]) {
				if (dungeonX > 0 && pmap[dungeonX-1][dungeonY].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX-1][dungeonY].layers[DUNGEON] = LEFT_WALL;
					if (dungeonX > ROOM_MIN_WIDTH) {
						addWallToList(LEFT, dungeonX-1, dungeonY);
					}
				}
				//				if (dungeonX < DCOLS && pmap[dungeonX+1][dungeonY].layers[DUNGEON] == GRANITE) {
				//					pmap[dungeonX+1][dungeonY].layers[DUNGEON] = RIGHT_WALL;
				//					addWallToList(RIGHT, dungeonX+1, dungeonY);
				//					if (dungeonX < DCOLS - ROOM_MIN_WIDTH) {
				//						addWallToList(LEFT, dungeonX-1, dungeonY);
				//					}
				//				}
				if (dungeonX < DCOLS && pmap[dungeonX+1][dungeonY].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX+1][dungeonY].layers[DUNGEON] = RIGHT_WALL;
					if (dungeonX < DCOLS - ROOM_MIN_WIDTH) {
						addWallToList(RIGHT, dungeonX+1, dungeonY);
					}
				}
				if (dungeonY > 0 && pmap[dungeonX][dungeonY-1].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX][dungeonY-1].layers[DUNGEON] = TOP_WALL;
					if (dungeonY > ROOM_MIN_HEIGHT) {
						addWallToList(UP, dungeonX, dungeonY-1);
					}
				}
				if (dungeonY < DROWS && pmap[dungeonX][dungeonY+1].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX][dungeonY+1].layers[DUNGEON] = BOTTOM_WALL;
					if (dungeonY < DROWS - ROOM_MIN_HEIGHT) {
						addWallToList(DOWN, dungeonX, dungeonY+1);
					}
				}
			}
		}
	}
	
	// Mark the corners as PERM_WALLs
	for (i = roomX; i < roomX + blobWidth; i++) {
		for (j=roomY; j < roomY + blobHeight; j++) {
			for (k=4; k<8; k++) {
				if ((pmap[i][j].layers[DUNGEON] == FLOOR)
					&& coordinatesAreInMap(i + nbDirs[k][0], j + nbDirs[k][1])
					&& (pmap[i+nbDirs[k][0]][j+nbDirs[k][1]].layers[DUNGEON] == GRANITE)) {
					pmap[i+nbDirs[k][0]][j+nbDirs[k][1]].layers[DUNGEON] = PERM_WALL;
				}
			}
		}
	}
	
	allocateRoom(roomX, roomY, blobWidth, blobHeight, 0, 0, 0, 0);
}

// Marks a cell as being a member of blobNumber, then recursively iterates through the rest of the blob
short markBlobCellAndIterate(short xLoc, short yLoc, short blobNumber) {
	short l, newX, newY, numberOfCells = 1;
	
	buffer[xLoc][yLoc] = blobNumber;
	
	// iterate through the neighbors
	for (l=0; l<4; l++) { // only the four cardinal neighbors; no diagonals!
		newX = xLoc + nbDirs[l][0];
		newY = yLoc + nbDirs[l][1];
		if (!coordinatesAreInMap(newX, newY)) {
			break;
		}
		if (buffer[newX][newY] == 1) { // if the neighbor is an unmarked blob cell
			numberOfCells += markBlobCellAndIterate(newX, newY, blobNumber); // recursive!
		}
	}
	return numberOfCells;
}

// blockingMap is optional.
// Returns the size of the connected zone, and marks visited[][] with the zoneLabel.
short connectCell(short x, short y, short zoneLabel, char blockingMap[DCOLS][DROWS], char zoneMap[DCOLS][DROWS]) {
	enum directions dir;
	short newX, newY, size;
	
	zoneMap[x][y] = zoneLabel;
	size = 1;
	
	for (dir = 0; dir < 4; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		
		if (coordinatesAreInMap(newX, newY)
			&& zoneMap[newX][newY] == 0
			&& (!blockingMap || !blockingMap[newX][newY])
			&& cellIsPassableOrDoor(newX, newY)) {
			
			size += connectCell(newX, newY, zoneLabel, blockingMap, zoneMap);
		}
	}
	return size;
}

// Make a zone map of connected passable regions that include at least one passable
// cell that borders the blockingMap if blockingMap blocks. Keep track of the size of each zone.
// Then pretend that the blockingMap no longer blocks, and grow these zones into the resulting area
// (without changing the stored zone sizes). If two or more zones now touch, then we block.
// At that point, return the size in cells of the smallest of all of the touching regions
// (or just 1, i.e. true, if countRegionSize is false). If no zones touch, then we don't block, and we return zero, i.e. false.
short levelIsDisconnectedWithBlockingMap(char blockingMap[DCOLS][DROWS], boolean countRegionSize) {
	char zoneMap[DCOLS][DROWS];
	short i, j, dir, zoneSizes[200], zoneCount, smallestQualifyingZoneSize, borderingZone;

	zoneCount = 0;
	smallestQualifyingZoneSize = 10000;
	zeroOutGrid(zoneMap);
	
//	dumpLevelToScreen();
//	hiliteGrid(blockingMap, &omniscienceColor, 100);
//	temporaryMessage("Blocking map:", true);
	
	// Map out the zones with the blocking area blocked.
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (cellIsPassableOrDoor(i, j) && zoneMap[i][j] == 0 && !blockingMap[i][j]) {
				for (dir=0; dir<4; dir++) {
					if (blockingMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]]) {
						zoneCount++;
						zoneSizes[zoneCount - 1] = connectCell(i, j, zoneCount, blockingMap, zoneMap);
						break;
					}
				}
			}
		}
	}
	
	// Expand the zones into the blocking area.
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (blockingMap[i][j] && zoneMap[i][j] == 0	&& cellIsPassableOrDoor(i, j)) {
				for (dir=0; dir<4; dir++) {
					borderingZone = zoneMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]];
					if (borderingZone != 0) {
						connectCell(i, j, borderingZone, NULL, zoneMap);
						break;
					}
				}
			}
		}
	}
	
	// Figure out which zones touch.
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (zoneMap[i][j] != 0) {
				for (dir=0; dir<4; dir++) {
					borderingZone = zoneMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]];
					if (zoneMap[i][j] != borderingZone && borderingZone != 0) {
						if (!countRegionSize) {
							return true;
						}
						smallestQualifyingZoneSize = min(smallestQualifyingZoneSize, zoneSizes[zoneMap[i][j] - 1]);
						smallestQualifyingZoneSize = min(smallestQualifyingZoneSize, zoneSizes[borderingZone - 1]);
						break;
					}
				}
			}
		}
	}
	return (smallestQualifyingZoneSize < 10000 ? smallestQualifyingZoneSize : 0);
}

void floodFillLakeCheck(short xLoc, short yLoc, char connectedMap[DCOLS][DROWS]) {
	short l, newX, newY;
	
	//logLevel();
	
//#ifdef BROGUE_ASSERTS
//	assert(connectedMap[xLoc][yLoc] == 0);
//#endif
	
	connectedMap[xLoc][yLoc] = 1;
	
//	hiliteCell(xLoc, yLoc, (rand_percent(50) ? &green : &red), 100, false);
//	pauseBrogue(10);
	
	// iterate through the neighbors
	for (l=0; l<4; l++) { // only the four cardinal neighbors; no diagonals!
		newX = xLoc + nbDirs[l][0];
		newY = yLoc + nbDirs[l][1];
		if (!coordinatesAreInMap(newX, newY)) {
			break;
		}
		if (connectedMap[newX][newY] == 0) { // If there's no proposed media and it's unmarked,
			floodFillLakeCheck(newX, newY, connectedMap); // then recurse.
		}
	}
}

boolean checkLakePassability(short lakeX, short lakeY, char connectedMap[DCOLS][DROWS]) {
	short i, j, startX = -1, startY = -1;
	
	//logLevel();
	
	//zeroOutGrid(connectedMap);
	
	// Get starting location for the fill.
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (i < lakeX + blobWidth && i >= lakeX &&
				j < lakeY + blobHeight && j >= lakeY &&
				buffer[i - lakeX + topBlobMinX][j - lakeY + topBlobMinY] == 1) {
				// If it's in proposed lake, mark 101.
				connectedMap[i][j] = 101;
			} else if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY) || pmap[i][j].layers[LIQUID]) {
				connectedMap[i][j] = 102;
			} else {
				connectedMap[i][j] = 0;
				// dry floor
				startX = i;
				startY = j;
			}
		}
	}
	//dumpLevelToScreen();
	floodFillLakeCheck(startX, startY, connectedMap);
	
	//logLevel();
	
	// If we can find an unmarked dry cell, the map is not connected.
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (connectedMap[i][j] == 0) { // unmarked dry floor
				// printf("\nLake failed at (%i, %i)", i, j);
				return false;
			}
		}
	}
	return true;
}

void liquidType(short *deep, short *shallow, short *shallowWidth) {
	short randMin, randMax, rand;
	
	randMin = (rogue.depthLevel < 4 ? 1 : 0); // no lava before level 4
	randMax = (rogue.depthLevel < 17 ? 2 : 3); // no brimstone before level 18
	rand = rand_range(randMin, randMax);
	
	switch(rand) {
		case 0:
			*deep = LAVA;
			*shallow = NOTHING;
			*shallowWidth = 0;
			break;
		case 1:
			*deep = DEEP_WATER;
			*shallow = SHALLOW_WATER;
			*shallowWidth = 2;
			break;
		case 2:
			*deep = CHASM;
			*shallow = CHASM_EDGE;
			*shallowWidth = 1;
			break;
		case 3:
			*deep = INERT_BRIMSTONE;
			*shallow = OBSIDIAN;
			*shallowWidth = 2;
			break;
	}
}

// Fills a lake marked in unfilledLakeMap with the specified liquid type, scanning outward to reach other lakes within scanWidth.
// Any wreath of shallow liquid must be done elsewhere.
void fillLake(short x, short y, short liquid, short scanWidth, char wreathMap[DCOLS][DROWS], char unfilledLakeMap[DCOLS][DROWS]) {
	short i, j;
	
	for (i = x - scanWidth; i <= x + scanWidth; i++) {
		for (j = y - scanWidth; j <= y + scanWidth; j++) {
			if (coordinatesAreInMap(i, j) && unfilledLakeMap[i][j]) {
				unfilledLakeMap[i][j] = false;
				pmap[i][j].layers[LIQUID] = liquid;
				wreathMap[i][j] = 1;
				fillLake(i, j, liquid, scanWidth, wreathMap, unfilledLakeMap);	// recursive
			}
		}
	}
}

void resetDFMessageEligibility() {
	short i;
	
	for (i=0; i<NUMBER_DUNGEON_FEATURES; i++) {
		dungeonFeatureCatalog[i].messageDisplayed = false;
	}
}

boolean fillSpawnMap(enum dungeonLayers layer,
					 enum tileType surfaceTileType,
					 char spawnMap[DCOLS][DROWS],
					 boolean blockedByOtherLayers,
					 boolean refresh,
					 boolean superpriority) {
	short i, j;
	creature *monst;
	item *theItem;
	boolean accomplishedSomething;
	
	accomplishedSomething = false;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (	// If it's flagged for building in the spawn map,
				spawnMap[i][j]
					// and the new cell doesn't already contain the fill terrain,
				&& pmap[i][j].layers[layer] != surfaceTileType
					// and the terrain in the layer to be overwritten has a higher priority number (unless superpriority),
				&& (superpriority || tileCatalog[pmap[i][j].layers[layer]].drawPriority >= tileCatalog[surfaceTileType].drawPriority)
					// and we won't be painting into the surface layer when that cell forbids it,
				&& !(layer == SURFACE && cellHasTerrainFlag(i, j, T_OBSTRUCTS_SURFACE_EFFECTS))
					// and, if requested, the fill won't violate the priority of the most important terrain in this cell:
				&& (!blockedByOtherLayers || tileCatalog[pmap[i][j].layers[highestPriorityLayer(i, j, true)]].drawPriority >= tileCatalog[surfaceTileType].drawPriority)
				) {
				
				if ((tileCatalog[surfaceTileType].flags & T_IS_FIRE)
					&& !(tileCatalog[pmap[i][j].layers[layer]].flags & T_IS_FIRE)) {
					pmap[i][j].flags |= CAUGHT_FIRE_THIS_TURN;
				}
				
				if ((tileCatalog[pmap[i][j].layers[layer]].flags & T_PATHING_BLOCKER)
					!= (tileCatalog[surfaceTileType].flags & T_PATHING_BLOCKER)) {
					
					rogue.staleLoopMap = true;
				}
				
				pmap[i][j].layers[layer] = surfaceTileType; // Place the terrain!
				accomplishedSomething = true;
				
				if (refresh) {
					refreshDungeonCell(i, j);
					if (player.xLoc == i && player.yLoc == j && !player.status[STATUS_LEVITATING] && refresh) {
						flavorMessage(tileFlavor(player.xLoc, player.yLoc));
					}
					if (pmap[i][j].flags & (HAS_MONSTER)) {
						monst = monsterAtLoc(i, j);
						applyInstantTileEffectsToCreature(monst);
						if (rogue.gameHasEnded) {
							return true;
						}
					}
					if (tileCatalog[surfaceTileType].flags & T_IS_FIRE) {
						if (pmap[i][j].flags & HAS_ITEM) {
							theItem = itemAtLoc(i, j);
							if (theItem->flags & ITEM_FLAMMABLE) {
								burnItem(theItem);
							}
						}
					}
				}
			} else {
				spawnMap[i][j] = false; // so that the spawnmap reflects what actually got built
			}
		}
	}
	return accomplishedSomething;
}

void spawnMapDF(short x, short y,
				enum tileType propagationTerrain,
				boolean requirePropTerrain,
				short startProb,
				short probDec,
				char spawnMap[DCOLS][DROWS]) {
	
	short i, j, dir, t, x2, y2;
	boolean madeChange;
	
	spawnMap[x][y] = t = 1; // incremented before anything else happens
	
	madeChange = true;
	
	while (madeChange && startProb > 0) {
		madeChange = false;
		t++;
		for (i = 0; i < DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (spawnMap[i][j] == t - 1) {
					for (dir = 0; dir < 4; dir++) {
						x2 = i + nbDirs[dir][0];
						y2 = j + nbDirs[dir][1];
						if (coordinatesAreInMap(x2, y2)
							&& (!requirePropTerrain || (propagationTerrain > 0 && cellHasTerrainType(x2, y2, propagationTerrain)))
							&& (!cellHasTerrainFlag(x2, y2, T_OBSTRUCTS_SURFACE_EFFECTS) || (propagationTerrain > 0 && cellHasTerrainType(x2, y2, propagationTerrain)))
							&& rand_percent(startProb)) {
							
							spawnMap[x2][y2] = t;
							madeChange = true;
						}
					}
				}
			}
		}
		startProb -= probDec;
		if (t > 100) {
			for (i = 0; i < DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (spawnMap[i][j] == t) {
						spawnMap[i][j] = 2;
					} else if (spawnMap[i][j] > 0) {
						spawnMap[i][j] = 1;
					}
				}
			}
			t = 2;
		}
	}
}

void evacuateCreatures(char blockingMap[DCOLS][DROWS]) {
	short i, j, newLoc[2];
	creature *monst;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (blockingMap[i][j]
				&& (pmap[i][j].flags & (HAS_MONSTER | HAS_PLAYER))) {
				
				monst = monsterAtLoc(i, j);
				getQualifyingLocNear(newLoc,
									 i, j,
									 true,
									 blockingMap,
									 forbiddenFlagsForMonster(&(monst->info)),
									 (HAS_MONSTER | HAS_PLAYER),
									 false,
									 false);
				monst->xLoc = newLoc[0];
				monst->yLoc = newLoc[1];
				pmap[i][j].flags &= ~(HAS_MONSTER | HAS_PLAYER);
				pmap[newLoc[0]][newLoc[1]].flags |= (monst == &player ? HAS_PLAYER : HAS_MONSTER);
			}
		}
	}
}

// returns whether the feature was successfully generated (false if we aborted because of blocking)
boolean spawnDungeonFeature(short x, short y, dungeonFeature *feat, boolean refreshCell, boolean abortIfBlocking) {
	short i, j, layer;
	char blockingMap[DCOLS][DROWS];
	boolean blocking;
	boolean succeeded = false;
	creature *monst;
    
	zeroOutGrid(blockingMap);
	
	// Blocking keeps track of whether to abort if it turns out that the DF would obstruct the level.
	blocking = ((abortIfBlocking
				 && !(feat->flags & DFF_PERMIT_BLOCKING)
				 && ((tileCatalog[feat->tile].flags & (T_PATHING_BLOCKER))
					 || (feat->flags & DFF_TREAT_AS_BLOCKING))) ? true : false);
	
	if (feat->tile) {
		if (feat->layer == GAS) {
			pmap[x][y].volume += feat->startProbability;
			pmap[x][y].layers[GAS] = feat->tile;
            if (refreshCell) {
                refreshDungeonCell(x, y);
            }
			succeeded = true;
		} else {
			spawnMapDF(x, y,
					   feat->propagationTerrain,
					   (feat->propagationTerrain ? true : false),
					   feat->startProbability,
					   feat->probabilityDecrement,
					   blockingMap);
			if (!blocking || !levelIsDisconnectedWithBlockingMap(blockingMap, false)) {
				if (feat->flags & DFF_EVACUATE_CREATURES_FIRST) { // first, evacuate creatures if necessary, so that they do not re-trigger the tile.
					evacuateCreatures(blockingMap);
				}
				
				//succeeded = fillSpawnMap(feat->layer, feat->tile, blockingMap, (feat->flags & DFF_BLOCKED_BY_OTHER_LAYERS), refreshCell, (feat->flags & DFF_SUPERPRIORITY));
				fillSpawnMap(feat->layer,
							 feat->tile,
							 blockingMap,
							 (feat->flags & DFF_BLOCKED_BY_OTHER_LAYERS),
							 refreshCell,
							 (feat->flags & DFF_SUPERPRIORITY)); // this can tweak the spawn map too
				succeeded = true; // fail ONLY if we blocked the level. We succeed even if, thanks to priority, nothing gets built.
			}		
		}
	} else {
		blockingMap[x][y] = true;
		succeeded = true; // Automatically succeed if there is no terrain to place.
		if (feat->flags & DFF_EVACUATE_CREATURES_FIRST) { // first, evacuate creatures if necessary, so that they do not re-trigger the tile.
			evacuateCreatures(blockingMap);
		}
	}
	
	if (succeeded && (feat->flags & DFF_CLEAR_OTHER_TERRAIN)) {
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (blockingMap[i][j]) {
					for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
						if (layer != feat->layer && layer != GAS) {
							pmap[i][j].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
						}
					}
				}
			}
		}
	}
	
	if (succeeded && refreshCell && feat->flashColor && feat->flashRadius) {
		lightFlash(feat->flashColor, 0, IN_FIELD_OF_VIEW, 4, feat->flashRadius, x, y);
	}
	
	if (refreshCell
		&& (tileCatalog[feat->tile].flags & (T_IS_FIRE | T_AUTO_DESCENT))
		&& cellHasTerrainFlag(player.xLoc, player.yLoc, (T_IS_FIRE | T_AUTO_DESCENT))) {
		
		applyInstantTileEffectsToCreature(&player);
	}
	if (rogue.gameHasEnded) {
		return succeeded;
	}
	if (succeeded && feat->description[0] && !feat->messageDisplayed && playerCanSee(x, y)) {
		feat->messageDisplayed = true;
		message(feat->description, false);
	}
	if (feat->subsequentDF && succeeded) {
		if (feat->flags & DFF_SUBSEQ_EVERYWHERE) {
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (blockingMap[i][j]) {
						spawnDungeonFeature(i, j, &dungeonFeatureCatalog[feat->subsequentDF], refreshCell, abortIfBlocking);
					}
				}
			}
		} else {
			spawnDungeonFeature(x, y, &dungeonFeatureCatalog[feat->subsequentDF], refreshCell, abortIfBlocking);
		}
	}
	if (feat->tile && (tileCatalog[feat->tile].flags & (T_IS_DEEP_WATER | T_LAVA_INSTA_DEATH | T_AUTO_DESCENT))) {
		rogue.updatedMapToShoreThisTurn = false;
	}
	
	// awaken dormant creatures?
	if (feat->flags & DFF_ACTIVATE_DORMANT_MONSTER) {
		for (monst = dormantMonsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (monst->xLoc == x && monst->yLoc == y || blockingMap[monst->xLoc][monst->yLoc]) {
				// found it!
				toggleMonsterDormancy(monst);
				monst = dormantMonsters;
			}
		}
	}
	return succeeded;
}

void restoreMonster(creature *monst, short **mapToStairs, short **mapToPit) {
	short i, *x, *y, loc[2], dir, turnCount;
	creature *leader;
	boolean foundLeader = false;
	short **theMap;
	
	x = &(monst->xLoc);
	y = &(monst->yLoc);
	
	if (monst->status[STATUS_ENTERS_LEVEL_IN] > 0) {
		if (monst->bookkeepingFlags & (MONST_APPROACHING_PIT)) {
			theMap = mapToPit;
		} else {
			theMap = mapToStairs;
		}
		if (theMap) {
			turnCount = ((theMap[monst->xLoc][monst->yLoc] * monst->movementSpeed / 100) - monst->status[STATUS_ENTERS_LEVEL_IN]);
			for (i=0; i < turnCount; i++) {
				if ((dir = nextStep(theMap, monst->xLoc, monst->yLoc, true)) != -1) {
					monst->xLoc += nbDirs[dir][0];
					monst->yLoc += nbDirs[dir][1];	
				}
			}
		}
		monst->bookkeepingFlags |= MONST_PREPLACED;
	}
	
	if ((pmap[*x][*y].flags & (HAS_PLAYER | HAS_UP_STAIRS | HAS_DOWN_STAIRS))
		|| (monst->bookkeepingFlags & MONST_PREPLACED)) {
		
		if (!(monst->bookkeepingFlags & MONST_PREPLACED)) {
			// (If if it's preplaced, it won't have set the HAS_MONSTER flag in the first place,
			// so clearing it might screw up an existing monster.)
			pmap[*x][*y].flags &= ~HAS_MONSTER;
		}
		getQualifyingLocNear(loc, *x, *y, true, 0, T_PATHING_BLOCKER,
							 (HAS_MONSTER | HAS_PLAYER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS | IS_IN_MACHINE), true, true);
		*x = loc[0];
		*y = loc[1];
	}
	pmap[*x][*y].flags |= HAS_MONSTER;
	monst->bookkeepingFlags &= ~(MONST_PREPLACED | MONST_APPROACHING_DOWNSTAIRS | MONST_APPROACHING_UPSTAIRS | MONST_APPROACHING_PIT);
	
	if ((monst->bookkeepingFlags & MONST_SUBMERGED) && !cellHasTerrainFlag(*x, *y, T_ALLOWS_SUBMERGING)) {
		monst->bookkeepingFlags &= ~MONST_SUBMERGED;
	}
	
	if (monst->bookkeepingFlags & MONST_FOLLOWER) {
		// is the leader on the same level?
		for (leader = monsters->nextCreature; leader != NULL; leader = leader->nextCreature) {
			if (leader == monst->leader) {
				foundLeader = true;
				break;
			}
		}
		// if not, it is time to spread your wings and fly solo
		if (!foundLeader) {
			monst->bookkeepingFlags &= ~MONST_FOLLOWER;
			monst->leader = NULL;
		}
	}
}

void restoreItem(item *theItem) {
	short *x, *y, loc[2];
	x = &(theItem->xLoc);
	y = &(theItem->yLoc);

	if (theItem->flags & ITEM_PREPLACED) {
		theItem->flags &= ~ITEM_PREPLACED;
		getQualifyingLocNear(loc, *x, *y, true, 0, (T_OBSTRUCTS_ITEMS | T_AUTO_DESCENT | T_IS_DEEP_WATER | T_LAVA_INSTA_DEATH),
							 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS), true, false);
		*x = loc[0];
		*y = loc[1];
	}
	pmap[*x][*y].flags |= HAS_ITEM;
	if (theItem->flags & ITEM_MAGIC_DETECTED && itemMagicChar(theItem)) {
		pmap[*x][*y].flags |= ITEM_DETECTED;
	}
}

// Returns true iff the location is a plain wall, three of the four cardinal neighbors are walls, the remaining cardinal neighbor
// is not a pathing blocker, the two diagonals between the three cardinal walls are also walls, and none of the eight neighbors are in machines.
boolean validStairLoc(short x, short y) {
	short newX, newY, dir, neighborWallCount;
	
	if (x < 1 || x >= DCOLS - 1 || y < 1 || y >= DROWS - 1 || pmap[x][y].layers[DUNGEON] != TOP_WALL) {
		return false;
	}
    
	for (dir=0; dir<8; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		if (pmap[newX][newY].flags & IS_IN_MACHINE) {
			return false;
		}
    }
	
	neighborWallCount = 0;
	for (dir=0; dir<4; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		
		if (cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
			// neighbor is a wall
			neighborWallCount++;
		} else {
			// neighbor is not a wall
			if (cellHasTerrainFlag(newX, newY, T_PATHING_BLOCKER)
				|| passableArcCount(newX, newY) >= 2) {
				return false;
			}
			// now check the two diagonals between the walls
			
			newX = x - nbDirs[dir][0] + nbDirs[dir][1];
			newY = y - nbDirs[dir][1] + nbDirs[dir][0];
			if (!cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
				return false;
			}
			
			newX = x - nbDirs[dir][0] - nbDirs[dir][1];
			newY = y - nbDirs[dir][1] - nbDirs[dir][0];
			if (!cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
				return false;
			}
		}
	}
	if (neighborWallCount != 3) {
		return false;
	}
	return true;
}

// The walls on either side become torches. Any adjacent granite then becomes top_wall. All wall neighbors are un-tunnelable.
// Grid is zeroed out within 5 spaces in all directions.
void prepareForStairs(short x, short y, char grid[DCOLS][DROWS]) {
	short newX, newY, dir;
	
	// Add torches to either side.
	for (dir=0; dir<4; dir++) {
		if (!cellHasTerrainFlag(x + nbDirs[dir][0], y + nbDirs[dir][1], T_OBSTRUCTS_PASSABILITY)) {
			newX = x - nbDirs[dir][1];
			newY = y - nbDirs[dir][0];
			pmap[newX][newY].layers[DUNGEON] = TORCH_WALL;
			newX = x + nbDirs[dir][1];
			newY = y + nbDirs[dir][0];
			pmap[newX][newY].layers[DUNGEON] = TORCH_WALL;
			break;
		}
	}
	// Expose granite.
	for (dir=0; dir<8; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		if (pmap[newX][newY].layers[DUNGEON] == GRANITE) {
			pmap[newX][newY].layers[DUNGEON] = TOP_WALL;
		}
        if (cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
            pmap[newX][newY].flags |= IMPREGNABLE;
        }
	}
	// Zero out grid in the vicinity.
	for (newX = max(0, x - 5); newX < min(DCOLS, x + 5); newX++) {
		for (newY = max(0, y - 5); newY < min(DROWS, y + 5); newY++) {
			grid[newX][newY] = false;
		}
	}
}

// Places the player, monsters, items and stairs.
void initializeLevel() {
	short i, j, dir;
	short upLoc[2], downLoc[2], **mapToStairs, **mapToPit;
	creature *monst;
	item *theItem, *prevItem;
	boolean amuletOnLevel;
	char grid[DCOLS][DROWS];
	short n = rogue.depthLevel - 1;
	
	// Place the stairs.
	
	for (i=0; i < DCOLS; i++) {
		for (j=0; j < DROWS; j++) {
			grid[i][j] = validStairLoc(i, j);
		}
	}

	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		hiliteGrid(grid, &teal, 100);
		temporaryMessage("Stair location candidates:", true);
	}
	
	if (rogue.depthLevel < 100) {
		if (getQualifyingGridLocNear(downLoc, levels[n].downStairsLoc[0], levels[n].downStairsLoc[1], grid, false)) {
			prepareForStairs(downLoc[0], downLoc[1], grid);
		} else {
			getQualifyingLocNear(downLoc, levels[n].downStairsLoc[0], levels[n].downStairsLoc[1], false, 0,
								 (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_AUTO_DESCENT | T_IS_DEEP_WATER | T_LAVA_INSTA_DEATH | T_ALLOWS_SUBMERGING | T_IS_DF_TRAP),
								 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS | IS_IN_MACHINE), true, false);		
		}
		pmap[downLoc[0]][downLoc[1]].layers[DUNGEON]    = DOWN_STAIRS;
		pmap[downLoc[0]][downLoc[1]].layers[LIQUID]     = NOTHING;
		pmap[downLoc[0]][downLoc[1]].layers[SURFACE]    = NOTHING;
		
		if (!levels[n+1].visited) {
			levels[n+1].upStairsLoc[0] = downLoc[0];
			levels[n+1].upStairsLoc[1] = downLoc[1];
		}
		
		levels[n].downStairsLoc[0] = downLoc[0];
		levels[n].downStairsLoc[1] = downLoc[1];
	}
	
	if (getQualifyingGridLocNear(upLoc, levels[n].upStairsLoc[0], levels[n].upStairsLoc[1], grid, false)) {
		prepareForStairs(upLoc[0], upLoc[1], grid);
	} else {
		getQualifyingLocNear(upLoc, levels[n].upStairsLoc[0], levels[n].upStairsLoc[1], false, 0,
							 (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_AUTO_DESCENT | T_IS_DEEP_WATER | T_LAVA_INSTA_DEATH | T_ALLOWS_SUBMERGING | T_IS_DF_TRAP),
							 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS | IS_IN_MACHINE), true, false);
	}
	
	levels[n].upStairsLoc[0] = upLoc[0];
	levels[n].upStairsLoc[1] = upLoc[1];
	
	if (rogue.depthLevel == 1) {
		pmap[upLoc[0]][upLoc[1]].layers[DUNGEON] = DUNGEON_EXIT;
		pmap[upLoc[0]][upLoc[1]].layers[LIQUID] = NOTHING;
		pmap[upLoc[0]][upLoc[1]].layers[SURFACE] = NOTHING;
	} else {
		pmap[upLoc[0]][upLoc[1]].layers[DUNGEON] = UP_STAIRS;
		pmap[upLoc[0]][upLoc[1]].layers[LIQUID] = NOTHING;
		pmap[upLoc[0]][upLoc[1]].layers[SURFACE] = NOTHING;
	}
	
	rogue.downLoc[0] = downLoc[0];
	rogue.downLoc[1] = downLoc[1];
	pmap[downLoc[0]][downLoc[1]].flags |= HAS_DOWN_STAIRS;
	rogue.upLoc[0] = upLoc[0];
	rogue.upLoc[1] = upLoc[1];
	pmap[upLoc[0]][upLoc[1]].flags |= HAS_UP_STAIRS;
	
	// Only one amulet per level!
	amuletOnLevel = (numberOfMatchingPackItems(AMULET, 0, 0, false) > 0);
	for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->category & AMULET) {
			if (amuletOnLevel) {
				for (prevItem = floorItems; prevItem->nextItem != theItem; prevItem = prevItem->nextItem);
				prevItem->nextItem = theItem->nextItem;
				deleteItem(theItem);
				theItem = prevItem->nextItem;
			} else {
				amuletOnLevel = true;
			}
		}
	}
	if (amuletOnLevel) {
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (monst->carriedItem && monst->carriedItem->category == AMULET) {
				free(monst->carriedItem);
				monst->carriedItem = NULL;
			}
		}
	}
	
	for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		restoreItem(theItem);
	}
	
	mapToStairs = allocDynamicGrid();
	fillDynamicGrid(mapToStairs, 0);
	mapToPit = allocDynamicGrid();
	fillDynamicGrid(mapToPit, 0);
	calculateDistances(mapToStairs, player.xLoc, player.yLoc, T_PATHING_BLOCKER, NULL, true, true);
	calculateDistances(mapToPit,
					   levels[rogue.depthLevel - 1].playerExitedVia[0],
					   levels[rogue.depthLevel - 1].playerExitedVia[1],
					   T_PATHING_BLOCKER,
					   NULL,
					   true,
					   true);
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		restoreMonster(monst, mapToStairs, mapToPit);
	}
	freeDynamicGrid(mapToStairs);
	freeDynamicGrid(mapToPit);
	
	if (!levels[rogue.depthLevel-1].visited) {
		//updateVision();
		
//		for (i=0; i<DCOLS; i++) {
//			for (j=0; j<DROWS; j++) {
//				if (pmap[i][j].flags & VISIBLE) {
//					plotCharWithColor(' ', mapToWindowX(i), mapToWindowY(j), yellow, yellow);
//				}
//			}
//		}
//		displayMoreSign();
		
		// Run a field of view check from up stairs so that monsters do not spawn within sight of it.
        for (dir=0; dir<4; dir++) {
            if (coordinatesAreInMap(upLoc[0] + nbDirs[dir][0], upLoc[1] + nbDirs[dir][1])
                && !cellHasTerrainFlag(upLoc[0] + nbDirs[dir][0], upLoc[1] + nbDirs[dir][1], T_OBSTRUCTS_PASSABILITY)) {
                
                upLoc[0] += nbDirs[dir][0];
                upLoc[1] += nbDirs[dir][1];
                break;
            }
        }
		zeroOutGrid(grid);
		getFOVMask(grid, upLoc[0], upLoc[1], max(DCOLS, DROWS), (T_OBSTRUCTS_VISION), 0, false);
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (grid[i][j]) {
					pmap[i][j].flags |= IN_FIELD_OF_VIEW;
				}
			}
		}
		populateItems(upLoc[0], upLoc[1]);
		populateMonsters();
	}
}

// fills (*x, *y) with the coordinates of a random cell with
// no creatures, items or stairs and with either a matching liquid and dungeon type
// or at least one layer of type terrainType.
// A dungeon, liquid type of -1 will match anything.
boolean randomMatchingLocation(short *x, short *y, short dungeonType, short liquidType, short terrainType) {
	short failsafeCount = 0;
	do {
		failsafeCount++;
		*x = rand_range(0, DCOLS - 1);
		*y = rand_range(0, DROWS - 1);
	} while (failsafeCount < 500 && ((terrainType >= 0 && !cellHasTerrainType(*x, *y, terrainType))
									 || (((dungeonType >= 0 && pmap[*x][*y].layers[DUNGEON] != dungeonType) || (liquidType >= 0 && pmap[*x][*y].layers[LIQUID] != liquidType)) && terrainType < 0)
									 || (pmap[*x][*y].flags & (HAS_PLAYER | HAS_MONSTER | HAS_DOWN_STAIRS | HAS_UP_STAIRS | HAS_ITEM | IS_IN_MACHINE))
									 || (terrainType < 0 && !(tileCatalog[dungeonType].flags & T_OBSTRUCTS_ITEMS)
										 && cellHasTerrainFlag(*x, *y, T_OBSTRUCTS_ITEMS))));
	if (failsafeCount >= 500) {
		return false;
	}
	return true;
}

void logLevel() {
	
	short i, j;
	char cellChar;
	
	printf("\n\n *** LEVEL %i ***", rogue.depthLevel);
	
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		if (i % 10 == 0) {
			printf("%i", i / 10);
		} else {
			printf(" ");
		}
	}
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		printf("%i", i % 10);
	}
	printf("\n");
	for( j=0; j<DROWS; j++ ) {
		if (j < 10) {
			printf(" ");
		}
		printf("%i: ", j);
		for(i=0; i<DCOLS; i++) {
			if (pmap[i][j].layers[LIQUID]) {
				cellChar = '0';
			} else {
				switch (pmap[i][j].layers[DUNGEON]) {
					case GRANITE:
						cellChar = ' ';
						break;
					case PERM_WALL:
						cellChar = '#';
						break;
					case LEFT_WALL:
					case RIGHT_WALL:
						cellChar = '|';
						break;
					case TOP_WALL:
					case BOTTOM_WALL:
						cellChar = '-';
						break;
					case FLOOR:
						cellChar = '.';
						break;
					case DOOR:
						cellChar = '+';
						break;
					case TORCH_WALL:
						cellChar = '*';
						break;
					case UP_STAIRS:
						cellChar = '<';
						break;
					case DOWN_STAIRS:
						cellChar = '>';
						break;
					default: // error
						cellChar = '&';
						break;
				}
			}
			printf("%c", cellChar);
		}
		printf("\n");
	}
	printf("\n");
}

void logBuffer(char array[DCOLS][DROWS]) {
//void logBuffer(short **array) {	
	short i, j;
	
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		if (i % 10 == 0) {
			printf("%i", i / 10);
		} else {
			printf(" ");
		}
	}
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		printf("%i", i % 10);
	}
	printf("\n");
	for( j=0; j<DROWS; j++ ) {
		if (j < 10) {
			printf(" ");
		}
		printf("%i: ", j);
		for( i=0; i<DCOLS; i++ ) {
			if (array[i][j] == 0) {
				printf(" ");
			} else if (array[i][j] == 1) {
				printf(".");
			} else if (array[i][j] == 2) {
				printf("o");
			} else if (array[i][j] == 3) {
				printf("#");
			} else {
				printf("%i", array[i][j]);
			}
		}
		printf("\n");
	}
	printf("\n");
}
