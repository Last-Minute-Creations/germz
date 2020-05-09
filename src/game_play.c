/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_play.h"
#include <ace/managers/game.h>
#include <ace/utils/font.h>
#include "game.h"
#include "game_assets.h"
#include "game_summary.h"
#include "game_editor.h"
#include "player.h"
#include "blob_anim.h"

static UBYTE s_isEven = 0;
static UBYTE s_ubCurrPlayer;

static tTextBitMap *s_pBmLine;

void displayUpdateHud(void) {
	static const UBYTE pPlayerColors[] = {8, 12, 16, 20};
	const tPlayer *pPlayer = playerFromIdx(s_ubCurrPlayer);
	UBYTE ubZeroBased = s_ubCurrPlayer - 1;
	const UBYTE ubMonitorPad = 7;
	UWORD uwMonitorX = HUD_OFFS_X + ubMonitorPad;
	UWORD uwMonitorY = ubZeroBased * HUD_MONITOR_SIZE + ubMonitorPad;

	tBitMap *pDisplay = gameGetBackBuffer();

	blitRect(pDisplay, uwMonitorX, uwMonitorY, 32, 10 + g_pFont->uwHeight, 6);
	if(!pPlayer->isDead) {
		if(pPlayer->pNodeCursor) {
			char szBfr[6];
			sprintf(szBfr, "%hd", pPlayer->pNodeCursor->wCharges);
			fontFillTextBitMap(g_pFont, s_pBmLine, szBfr);
			fontDrawTextBitMap(
				pDisplay, s_pBmLine, uwMonitorX, uwMonitorY,
				pPlayerColors[ubZeroBased], FONT_COOKIE
			);
		}

		if(pPlayer->isSelectingDestination) {
			char szBfr[6];
			sprintf(szBfr, "%hd", pPlayer->pNodePlepSrc->wCharges / 2);
			fontFillTextBitMap(g_pFont, s_pBmLine, szBfr);
			fontDrawTextBitMap(
				pDisplay, s_pBmLine, uwMonitorX, uwMonitorY + 10,
				pPlayerColors[ubZeroBased], FONT_COOKIE
			);
		}
	}
	if(s_isEven) {
		if(++s_ubCurrPlayer > 4) {
			s_ubCurrPlayer = 1;
		}
	}
	s_isEven = !s_isEven;
}

//-------------------------------------------------------------------- GAMESTATE

void gamePlayGsCreate(void) {
	s_isEven = 0;
	s_ubCurrPlayer = 1;
	s_pBmLine = fontCreateTextBitMap(64, g_pFont->uwHeight);
	blobAnimReset();
}

void gamePlayGsLoop(void) {
	if(keyUse(KEY_F1)) {
		gameChangeState(gameEditorGsCreate, gameEditorGsLoop, gameEditorGsDestroy);
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
		gameChangeState(
			gameSummaryGsCreate, gameSummaryGsLoop, gameSummaryGsDestroy
		);
		return;
	}
	gamePostprocess();
}

void gamePlayGsDestroy(void) {
	fontDestroyTextBitMap(s_pBmLine);
}
