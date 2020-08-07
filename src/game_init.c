/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_init.h"
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include "game.h"
#include "game_assets.h"
#include "game_play.h"
#include "map.h"
#include "fade.h"
#include "germz.h"

static UBYTE s_isEven;
static UBYTE s_ubCurrY;
static UBYTE s_ubFrame = 0;

static UBYTE initialAnim(void) {
	UBYTE isDrawnAnyBlob = 0;
	UBYTE ubY = s_ubCurrY;
	while(!isDrawnAnyBlob && ubY < MAP_SIZE) {
		for(UBYTE ubX = 0; ubX < MAP_SIZE; ++ubX) {
			tTile eTile = g_sMapData.pTiles[ubX][ubY];
			if(tileIsNode(eTile)) {
				isDrawnAnyBlob = 1;
			}
			tUbCoordYX sPos = {.ubX = ubX, .ubY = ubY};
			gameDrawMapTileAt(sPos, s_ubFrame);
		}
		if(!isDrawnAnyBlob || ++s_ubFrame >= BLOB_FRAME_COUNT) {
			s_ubFrame = 0;
			++ubY;
		}
	};

	if(s_isEven) {
		s_ubCurrY = ubY;
		if(ubY >= MAP_SIZE) {
			return 1;
		}
	}
	s_isEven = !s_isEven;
	return 0;
}

//-------------------------------------------------------------------- GAMESTATE

static void gameInitGsCreate(void) {
	s_isEven = 0;
	s_ubCurrY = 0;
	s_ubFrame = 0;
	tBitMap *pDisplay = gameGetBackBuffer();

	blitRect(pDisplay, 0, 0, 320, 128, 0);
	blitRect(pDisplay, 0, 128, 320, 128, 0);
	bobNewDiscardUndraw();

	bitmapLoadFromFile(pDisplay, "data/game_hud.bm", HUD_OFFS_X, 0);
	bitmapLoadFromFile(pDisplay, "data/game_bg.bm", 0, 0);

	// Draw monitors on back buffer
	for(UBYTE i = 1; i < 4; ++i) {
		blitCopyAligned(
			pDisplay, HUD_OFFS_X, 0, pDisplay,
			HUD_OFFS_X, i * HUD_MONITOR_SIZE, HUD_MONITOR_SIZE, HUD_MONITOR_SIZE
		);
	}

	// Draw tiled bg on back buffer
	for(UBYTE x = 0; x < 256 / 64; ++x) {
		for(UBYTE y = 0; y < 256 / 64; ++y) {
			if(x == 0 && y < 2) {
				continue;
			}
			UBYTE ubSrcY = ((x + y) & 1) ? 64 : 0;
			blitCopyAligned(pDisplay, 0, ubSrcY, pDisplay, x * 64, y * 64, 64, 64);
		}
	}

	gameCopyBackToFront();

	bobNewManagerReset();
	gameInitCursorBobs();
	gameInitMap();
	bobNewReallocateBgBuffers();
}

static void gameInitGsLoop(void) {
	tFadeState eState = fadeProcess(gameGetFade());
	if(!gamePreprocess()) {
		return;
	}
	if(initialAnim() && eState == FADE_STATE_IDLE) {
		stateChange(g_pStateMachineGame, &g_sStateGamePlay);
		return;
	}
	gamePostprocess();
}

static void gameInitGsDestroy(void) {

}

tState g_sStateGameInit = {
	.cbCreate = gameInitGsCreate, .cbLoop = gameInitGsLoop,
	.cbDestroy = gameInitGsDestroy
};
