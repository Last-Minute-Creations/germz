/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/system.h>
#include <ace/managers/game.h>
#include "map.h"
#include "display.h"
#include "menu.h"
#include "ai.h"
#include "player.h"

static UBYTE s_isMainLoop;
static tMap *s_pMap = 0;

void gameGsCreate(void) {
	displayCreate();

	s_pMap = mapCreateFromFile("data/maps/map1.json");
	if(!s_pMap) {
		logWrite("MAP CREATE FAIL\n");
		return;
	}
	aiCreate(s_pMap);
	plepSetMap(s_pMap);
	s_isMainLoop = 0;

	for(UBYTE i = 0; i < s_pMap->ubPlayerCount; ++i) {
		playerReset(i, s_pMap->pPlayerStartNodes[i], menuGetSteerForPlayer(i));
	}
	mapUpdateNodeCountForPlayers(s_pMap);
	displayEnable();
}

void mainLoop(void) {
	bobNewBegin();
	displayQueueProcess();
	displayUpdateHud();
	playerProcess();
	bobNewPushingDone();
	mapProcessNodes(s_pMap);
	bobNewEnd();
}

void gameGsLoop(void) {
	displayDebugColor(0x00F);
	if(keyUse(KEY_ESCAPE)) {
		gameChangeState(menuGsCreate, menuGsLoop, menuGsDestroy);
		return;
	}

	if(s_isMainLoop) {
		mainLoop();
	}
	else {
		if(displayInitialAnim(s_pMap)) {
			s_isMainLoop = 1;
		}
	}

	displayProcess();
	if(keyUse(KEY_C)) {
		displayToggleDebug();
	}
	if(keyCheck(KEY_B)) {
		displayLag();
	}
}

void gameGsDestroy(void) {
	displayDestroy();
	aiDestroy();
	if(s_pMap) {
		mapDestroy(s_pMap);
	}
}
