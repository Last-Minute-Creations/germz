/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_play.h"
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/utils/font.h>
#include <ace/utils/string.h>
#include "game.h"
#include "game_assets.h"
#include "game_summary.h"
#include "game_editor.h"
#include "player.h"
#include "blob_anim.h"
#include "germz.h"

#define HUD_ALIAS_BG 0
#define HUD_GRABBED_OFFS_Y 15

typedef enum _tHudState {
	HUD_STATE_PLEPS_PREPARE,
	HUD_STATE_PLEPS_DRAW,
	HUD_STATE_GRABBED_PLEPS_PREPARE,
	HUD_STATE_GRABBED_PLEPS_DRAW,
	HUD_STATE_NEXT_PLAYER
} tHudState;

static UBYTE s_isHudDrawnOnce;
static UBYTE s_isHudDrawCurrent;
static tPlayerIdx s_eHudCurrPlayer;
static UBYTE s_pHudPlayersWereDead[4];
tHudState s_eHudState;

static tBitMap s_sBmHudAlias;

static void updateHud(void) {
	static const UBYTE pPlayerColors[] = {
		10 >> 2, 14 >> 2, 18 >> 2, 22 >> 2, 6 >> 2
	};
	// colors are arranged so that only top 3 bitplanes needs updating:
	//  6: 00110 neutral
	// 10: 01010 red
	// 14: 01110 green
	// 18: 10010 yellow
	// 22: 10110 blue
	//  2: 00010 bg
	//  3: 00011 scanlines dark, so do scanlines with LSbit

	const tPlayer *pPlayer = playerFromIdx(s_eHudCurrPlayer);
	const UBYTE ubMonitorPad = 7;
	const UWORD uwMonitorX = HUD_OFFS_X + ubMonitorPad;
	const UWORD uwMonitorY = s_eHudCurrPlayer * HUD_MONITOR_SIZE + ubMonitorPad;
	tBitMap *pDisplay = gameGetBackBuffer();
	s_sBmHudAlias.Planes[0] = pDisplay->Planes[2];
	s_sBmHudAlias.Planes[1] = pDisplay->Planes[3];
	s_sBmHudAlias.Planes[2] = pDisplay->Planes[4];

	tHudState eNextState = s_eHudState;
	if(pPlayer->isDead) {
		if(!s_pHudPlayersWereDead[s_eHudCurrPlayer]) {
			if(s_isHudDrawnOnce) {
				// This is second pass - don't go here anymore
				s_pHudPlayersWereDead[s_eHudCurrPlayer] = 1;
			}
			// Erase whole HUD
			blitRect(
				&s_sBmHudAlias, uwMonitorX, uwMonitorY,
				32, HUD_GRABBED_OFFS_Y + g_pFontBig->uwHeight, HUD_ALIAS_BG
			);
		}
		eNextState = HUD_STATE_NEXT_PLAYER;
	}
	else {
		switch(s_eHudState) {
			case HUD_STATE_PLEPS_PREPARE:
				if(pPlayer->pNodeCursor) {
					char szBfr[6];
					stringDecimalFromULong(pPlayer->pNodeCursor->wCharges, szBfr);
					fontFillTextBitMap(g_pFontBig, g_pTextBitmap, szBfr);
					s_isHudDrawCurrent = 1;
				}
				else {
					s_isHudDrawCurrent = 0; // just clear display in next state
				}
				s_isHudDrawnOnce = 1; // prevent processing falling here in another frame
				eNextState = HUD_STATE_PLEPS_DRAW;
				break;
			case HUD_STATE_PLEPS_DRAW:
				blitRect(
					&s_sBmHudAlias, uwMonitorX, uwMonitorY,
					32, g_pTextBitmap->uwActualHeight, HUD_ALIAS_BG
				);
				if(s_isHudDrawCurrent) {
					fontDrawTextBitMap(
						&s_sBmHudAlias, g_pTextBitmap, uwMonitorX, uwMonitorY,
						pPlayerColors[playerToIdx(pPlayer->pNodeCursor->pPlayer)], FONT_COOKIE
					);
				}
				eNextState = HUD_STATE_GRABBED_PLEPS_PREPARE;
				break;
			case HUD_STATE_GRABBED_PLEPS_PREPARE:
				if(pPlayer->isSelectingDestination) {
					char szBfr[6];
					stringDecimalFromULong(pPlayer->pNodePlepSrc->wCharges / 2, szBfr);
					fontFillTextBitMap(g_pFontBig, g_pTextBitmap, szBfr);
					s_isHudDrawCurrent = 1;
				}
				else {
					s_isHudDrawCurrent = 0; // just clear display in next state
				}
				s_isHudDrawnOnce = 1; // prevent processing falling here in another frame
				eNextState = HUD_STATE_GRABBED_PLEPS_DRAW;
				break;
			case HUD_STATE_GRABBED_PLEPS_DRAW:
				blitRect(
					&s_sBmHudAlias, uwMonitorX, uwMonitorY + HUD_GRABBED_OFFS_Y,
					32, g_pTextBitmap->uwActualHeight, HUD_ALIAS_BG
				);
				if(s_isHudDrawCurrent) {
					fontDrawTextBitMap(
						&s_sBmHudAlias, g_pTextBitmap, uwMonitorX, uwMonitorY + HUD_GRABBED_OFFS_Y,
						pPlayerColors[s_eHudCurrPlayer], FONT_COOKIE
					);
				}
				eNextState = HUD_STATE_NEXT_PLAYER;
				break;
			default:
				break;
		}
	}

	if(!s_isHudDrawnOnce) {
		// Stay in same state for one more frame (double buffering)
		s_isHudDrawnOnce = 1;
	}
	else {
		s_isHudDrawnOnce = 0;
		if(eNextState == HUD_STATE_NEXT_PLAYER) {
			// Switch player, reset state machine
			s_eHudCurrPlayer = (s_eHudCurrPlayer + 1) & 3;
			s_eHudState = 0;
		}
		else {
			// Proceed to next state
			s_eHudState = eNextState;
		}
	}
}

//-------------------------------------------------------------------- GAMESTATE

static void gamePlayGsCreate(void) {
	tBitMap *pDisplay = gameGetBackBuffer();
	s_sBmHudAlias.Rows = pDisplay->Rows;
	s_sBmHudAlias.BytesPerRow = pDisplay->BytesPerRow;
	s_sBmHudAlias.Depth = 3;

	s_isHudDrawnOnce = 0;
	s_isHudDrawCurrent = 0;
	s_eHudCurrPlayer = 0;
	memset(s_pHudPlayersWereDead, 0, sizeof(s_pHudPlayersWereDead));
	s_eHudState = 0;
	blobAnimReset();
}

static void gamePlayGsLoop(void) {
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
	updateHud();
	blitWait();
	UBYTE ubAliveCount = playerProcess();
	playerPushCursors();
	bobNewPushingDone();
	mapProcessNodes();

	if(!ubAliveCount) {
		stateChange(g_pStateMachineGame, &g_sStateGameSummary);
		return;
	}
	gamePostprocess();
}

static void gamePlayGsDestroy(void) {
}

tState g_sStateGamePlay = {
	.cbCreate = gamePlayGsCreate, .cbLoop = gamePlayGsLoop,
	.cbDestroy = gamePlayGsDestroy
};
