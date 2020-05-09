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

static UBYTE s_isEven;
static UWORD s_uwCurr;

static tUbCoordYX getNth(UWORD uwN) {
	tUbCoordYX sPos = {.ubX = uwN % 16, .ubY = uwN / 16};
	return sPos;
}

static UBYTE initialAnim(void) {
	static const UBYTE ubStep = 2;
	static const UBYTE ubTailLength = 9;

	for(UBYTE i = 0; i < ubTailLength; ++i) {
		WORD wPos = s_uwCurr - i;
		if(wPos < 0) {
			break;
		}
		if(wPos >= 256) {
			continue;
		}
		tUbCoordYX sPos = getNth(wPos);
		BYTE bFrame = ((i + ubStep - 1) * BLOB_FRAME_COUNT) / ubTailLength;
		bFrame = MIN(bFrame, BLOB_FRAME_COUNT - 1);
		gameDrawMapTileAt(sPos.ubX, sPos.ubY, bFrame);
	}

	if(s_isEven) {
		s_uwCurr += ubStep;
		if(s_uwCurr - ubTailLength >= 16*16) {
			return 1;
		}
	}
	s_isEven = !s_isEven;
	return 0;
}

//-------------------------------------------------------------------- GAMESTATE

void gameInitGsCreate(void) {
	s_isEven = 0;
	s_uwCurr = 0;
	tBitMap *pDisplay = gameGetBackBuffer();

	blitRect(pDisplay, 0, 0, 320, 128, 0);
	blitRect(pDisplay, 0, 128, 320, 128, 0);
	bobNewDiscardUndraw();

	systemUse();
	bitmapLoadFromFile(pDisplay, "data/monitor.bm", HUD_OFFS_X, 0);
	systemUnuse();

	// Draw monitors on back buffer
	for(UBYTE i = 1; i < 4; ++i) {
		blitCopyAligned(
			pDisplay, HUD_OFFS_X, 0, pDisplay,
			HUD_OFFS_X, i * HUD_MONITOR_SIZE, HUD_MONITOR_SIZE, HUD_MONITOR_SIZE
		);
	}
	gameCopyBackToFront();

	gameInitMap();
}

void gameInitGsLoop(void) {
	if(!gamePreprocess()) {
		return;
	}
	if(initialAnim()) {
		gameChangeState(gamePlayGsCreate, gamePlayGsLoop, gamePlayGsDestroy);
		return;
	}
	gamePostprocess();
}

void gameInitGsDestroy(void) {

}
