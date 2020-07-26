/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_play.h"
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/utils/font.h>
#include "game.h"
#include "game_assets.h"
#include "game_summary.h"
#include "game_editor.h"
#include "player.h"
#include "blob_anim.h"
#include "germz.h"

#define HUD_BG 7

static UBYTE s_isEven = 0;
static tPlayerIdx s_eCurrPlayer;

void displayUpdateHud(void) {
	static const UBYTE pPlayerColors[] = {8, 12, 16, 20};
	const tPlayer *pPlayer = playerFromIdx(s_eCurrPlayer);
	const UBYTE ubMonitorPad = 7;
	UWORD uwMonitorX = HUD_OFFS_X + ubMonitorPad;
	UWORD uwMonitorY = s_eCurrPlayer * HUD_MONITOR_SIZE + ubMonitorPad;

	tBitMap *pDisplay = gameGetBackBuffer();

	blitRect(pDisplay, uwMonitorX, uwMonitorY, 32, 10 + g_pFont->uwHeight, HUD_BG);
	if(!pPlayer->isDead) {
		if(pPlayer->pNodeCursor) {
			char szBfr[6];
			sprintf(szBfr, "%hd", pPlayer->pNodeCursor->wCharges);
			fontFillTextBitMap(g_pFont, g_pTextBitmap, szBfr);
			fontDrawTextBitMap(
				pDisplay, g_pTextBitmap, uwMonitorX, uwMonitorY,
				pPlayerColors[s_eCurrPlayer], FONT_COOKIE
			);
		}

		if(pPlayer->isSelectingDestination) {
			char szBfr[6];
			sprintf(szBfr, "%hd", pPlayer->pNodePlepSrc->wCharges / 2);
			fontFillTextBitMap(g_pFont, g_pTextBitmap, szBfr);
			fontDrawTextBitMap(
				pDisplay, g_pTextBitmap, uwMonitorX, uwMonitorY + 10,
				pPlayerColors[s_eCurrPlayer], FONT_COOKIE
			);
		}
	}
	if(s_isEven) {
		s_eCurrPlayer = (s_eCurrPlayer + 1) & 3;
	}
	s_isEven = !s_isEven;
}

//-------------------------------------------------------------------- GAMESTATE

void gamePlayGsCreate(void) {
	s_isEven = 0;
	s_eCurrPlayer = 1;
	blobAnimReset();
}

void gamePlayGsLoop(void) {
	if(keyUse(KEY_F1)) {
		// Ensure that player pushes off that button so it won't bug editor
		while(keyCheck(KEY_F1)) {
			keyProcess();
		}
		stateChange(g_pStateMachineGame, &g_sStateEditor);
		return;
	}
	if(!gamePreprocess()) {
		return;
	}
	blobAnimQueueProcess();
	displayUpdateHud();
	blitWait();
	UBYTE ubAliveCount = playerProcess();
	bobNewPushingDone();
	mapProcessNodes();

	if(!ubAliveCount) {
		stateChange(g_pStateMachineGame, &g_sStateGameSummary);
		return;
	}
	gamePostprocess();
}

void gamePlayGsDestroy(void) {
}

tState g_sStateGamePlay = STATE(
	gamePlayGsCreate, gamePlayGsLoop, gamePlayGsDestroy, 0, 0
);
