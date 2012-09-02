/*
 *  Light.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/21/09.
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

#include <math.h>
#include "Rogue.h"
#include "IncludeGlobals.h"

void logLights() {
	
	short i, j;
	
	printf("    ");
	for (i=0; i<COLS-2; i++) {
		printf("%i", i % 10);
	}
	printf("\n");
	for( j=0; j<DROWS-2; j++ ) {
		if (j < 10) {
			printf(" ");
		}
		printf("%i: ", j);
		for( i=0; i<DCOLS-2; i++ ) {
			if (tmap[i][j].light[0] == 0) {
				printf(" ");
			} else {
				printf("%i", max(0, tmap[i][j].light[0] / 10 - 1));
			}
		}
		printf("\n");
	}
	printf("\n");
}

void paintLight(lightSource *theLight, short x, short y, boolean isMinersLight, boolean maintainShadows) {
	short i, j, k;
	short colorComponents[3], randComponent, lightMultiplier;
	short fadeToPercent;
	float radius;
	char grid[DCOLS][DROWS];
	boolean dispelShadows;
	
#ifdef BROGUE_ASSERTS
	assert(rogue.RNG == RNG_SUBSTANTIVE);
#endif
	
	radius = randClump(theLight->lightRadius);
	radius /= 100;
	
	randComponent = rand_range(0, theLight->lightColor->rand);
	colorComponents[0] = randComponent + theLight->lightColor->red + rand_range(0, theLight->lightColor->redRand);
	colorComponents[1] = randComponent + theLight->lightColor->green + rand_range(0, theLight->lightColor->greenRand);
	colorComponents[2] = randComponent + theLight->lightColor->blue + rand_range(0, theLight->lightColor->blueRand);
	
	// the miner's light does not dispel IS_IN_SHADOW,
	// so the player can be in shadow despite casting his own light.
	dispelShadows = !maintainShadows && (colorComponents[0] + colorComponents[1] + colorComponents[2]) > 0;
	
	fadeToPercent = theLight->radialFadeToPercent;
	
	// zero out only the relevant rectangle of the grid
	for (i = max(0, x - radius); i < DCOLS && i < x + radius; i++) {
		for (j = max(0, y - radius); j < DROWS && j < y + radius; j++) {
			grid[i][j] = 0;
		}
	}
	
	getFOVMask(grid, x, y, radius, T_OBSTRUCTS_VISION, (theLight->passThroughCreatures ? 0 : (HAS_MONSTER | HAS_PLAYER)),
			   (!isMinersLight));
	
	for (i = max(0, x - radius); i < DCOLS && i < x + radius; i++) {
		for (j = max(0, y - radius); j < DROWS && j < y + radius; j++) {
			if (grid[i][j]) {
				lightMultiplier = 100 - (100 - fadeToPercent) * (sqrt((i-x) * (i-x) + (j-y) * (j-y)) / (radius));
				for (k=0; k<3; k++) {
					tmap[i][j].light[k] += colorComponents[k] * lightMultiplier / 100;;
				}
				if (dispelShadows) {
					pmap[i][j].flags &= ~IS_IN_SHADOW;
				}
			}
		}
	}
	
	tmap[x][y].light[0] += colorComponents[0];
	tmap[x][y].light[1] += colorComponents[1];
	tmap[x][y].light[2] += colorComponents[2];
	
	if (dispelShadows) {
		pmap[x][y].flags &= ~IS_IN_SHADOW;
	}
}


// sets miner's light strength and characteristics based on rings of illumination, scrolls of darkness and water submersion
void updateMinersLightRadius() {
	float fraction;
	float lightRadius;
	
	lightRadius = 100 * rogue.minersLightRadius;
	
	if (rogue.lightMultiplier < 0) {
		lightRadius /= (-1 * rogue.lightMultiplier + 1);
	} else {
		lightRadius *= (rogue.lightMultiplier);
		lightRadius = max(lightRadius, (rogue.lightMultiplier * 2 + 2));
	}
	
	
	if (player.status[STATUS_DARKNESS]) {
		fraction = (float) pow(1.0 - (((float) player.status[STATUS_DARKNESS]) / player.maxStatus[STATUS_DARKNESS]), 3);
		if (fraction < 0.05) {
			fraction = 0.05;
		}
	} else {
		fraction = 1;
	}
	lightRadius = lightRadius * fraction;
	
	if (lightRadius < 2) {
		lightRadius = 2;
	}
	
	if (rogue.inWater && lightRadius > 3) {
		lightRadius = max(lightRadius / 2, 3);
	}
	
	rogue.minersLight.radialFadeToPercent = 35 + max(0, min(65, rogue.lightMultiplier * 5)) * fraction;
	rogue.minersLight.lightRadius.upperBound = rogue.minersLight.lightRadius.lowerBound = clamp(lightRadius, -30000, 30000);
}

void updateDisplayDetail() {
	short i, j;
	
	for (i = 0; i < DCOLS; i++) {
		for (j = 0; j < DROWS; j++) {
			if (tmap[i][j].light[0] < -10
				&& tmap[i][j].light[1] < -10
				&& tmap[i][j].light[0] < -10) {
				
				displayDetail[i][j] = DV_DARK;
			} else if (pmap[i][j].flags & IS_IN_SHADOW) {
				displayDetail[i][j] = DV_UNLIT;
			} else {
				displayDetail[i][j] = DV_LIT;
			}
		}
	}
}

void updateLighting() {
	short i, j, k;
	enum dungeonLayers layer;
	enum tileType tile;
	creature *monst;

	// Copy Light over oldLight and then zero out Light.
	for (i = 0; i < DCOLS; i++) {
		for (j = 0; j < DROWS; j++) {
			for (k=0; k<3; k++) {
				tmap[i][j].oldLight[k] = tmap[i][j].light[k];
				tmap[i][j].light[k] = 0;
			}
			pmap[i][j].flags |= IS_IN_SHADOW;
		}
	}
	
	// Go through all glowing tiles.
	for (i = 0; i < DCOLS; i++) {
		for (j = 0; j < DROWS; j++) {
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				tile = pmap[i][j].layers[layer];
				if (tileCatalog[tile].glowLight) {
					paintLight(&(lightCatalog[tileCatalog[tile].glowLight]), i, j, false, false);
				}
			}
		}
	}
	
	// Cycle through monsters and paint their lights:
	CYCLE_MONSTERS_AND_PLAYERS(monst) {	
		if (monst->info.flags & MONST_INTRINSIC_LIGHT) {
			paintLight(&lightCatalog[monst->info.intrinsicLightType], monst->xLoc, monst->yLoc, false, false);
		}
		
		if (monst->status[STATUS_BURNING] && !(monst->info.flags & MONST_FIERY)) {
			paintLight(&lightCatalog[BURNING_CREATURE_LIGHT], monst->xLoc, monst->yLoc, false, false);
		}
		
		if (player.status[STATUS_TELEPATHIC] && &player != monst && !(monst->info.flags & MONST_INANIMATE)) {
			paintLight(&lightCatalog[TELEPATHY_LIGHT], monst->xLoc, monst->yLoc, false, true);
		}
	}
	
	// Also paint telepathy lights for dormant monsters.
	if (player.status[STATUS_TELEPATHIC]) {
		for (monst = dormantMonsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (!(monst->info.flags & MONST_INANIMATE)) {
				paintLight(&lightCatalog[TELEPATHY_LIGHT], monst->xLoc, monst->yLoc, false, true);
			}
		}
	}
	
	updateDisplayDetail();
	
	// Miner's light:
	paintLight(&rogue.minersLight, player.xLoc, player.yLoc, true, true);
    
    if (player.status[STATUS_INVISIBLE]) {
        player.info.foreColor = &playerInvisibleColor;
	} else if (playerInDarkness()) {
		player.info.foreColor = &playerInDarknessColor;
	} else if (pmap[player.xLoc][player.yLoc].flags & IS_IN_SHADOW) {
		player.info.foreColor = &playerInShadowColor;
	} else {
		player.info.foreColor = &playerInLightColor;
	}
}

boolean playerInDarkness() {
	return (tmap[player.xLoc][player.yLoc].light[0] + 10 < minersLightColor.red
			&& tmap[player.xLoc][player.yLoc].light[1] + 10 < minersLightColor.green
			&& tmap[player.xLoc][player.yLoc].light[2] + 10 < minersLightColor.blue);
}
