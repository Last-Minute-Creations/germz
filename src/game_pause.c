/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_pause.h"
#include <ace/managers/key.h>
#include <ace/utils/bmframe.h>
#include <ace/utils/string.h>
#include "germz.h"
#include "gui/dialog.h"
#include "game.h"
#include "assets.h"
#include "color.h"
#include "menu_list.h"
#include "player.h"
#include "value_ptr.h"

#define PAUSE_MENU_MAX 3

static tBitMap *s_pBmDialog;
static tBitMap s_sBmDlgScanline;
static tPauseKind s_eKind;

static void onRestart(void);
static void onExit(void);
static void onBack(void);
static void onNextMap(void);

static const tMenuListStyle s_sMenuStyle = {
	.ubColorActive = COLOR_P3_BRIGHT >> 1,
	.ubColorInactive = (COLOR_P3_BRIGHT + 2) >> 1,
	.ubColorShadow = 0xFF
};

static tOption s_pPauseOptions[PAUSE_MENU_MAX];
static const char *s_pPauseLabels[PAUSE_MENU_MAX];

static void gamePauseGsCreate(void) {
	UWORD uwDlgWidth = 192;
	UWORD uwDlgHeight = 160;
	s_pBmDialog = dialogCreate(
		uwDlgWidth, uwDlgHeight, gameGetBackBuffer(), gameGetFrontBuffer()
	);

	s_sBmDlgScanline.BytesPerRow = s_pBmDialog->BytesPerRow;
	s_sBmDlgScanline.Rows = s_pBmDialog->Rows;
	s_sBmDlgScanline.Depth = s_pBmDialog->Depth - 1;
	s_sBmDlgScanline.Planes[0] = s_pBmDialog->Planes[1];
	s_sBmDlgScanline.Planes[1] = s_pBmDialog->Planes[2];
	s_sBmDlgScanline.Planes[2] = s_pBmDialog->Planes[3];
	s_sBmDlgScanline.Planes[3] = s_pBmDialog->Planes[4];

	bmFrameDraw(
		g_pFrameDisplay, s_pBmDialog, 0, 0, uwDlgWidth / 16, uwDlgHeight / 16, 16
	);

	UWORD uwX = 16;
	UWORD uwY = 16;
	UWORD uwRowSize = g_pFontBig->uwHeight + 2;

	switch(s_eKind) {
		case PAUSE_KIND_BATTLE_PAUSE:
		case PAUSE_KIND_CAMPAIGN_PAUSE:
		case PAUSE_KIND_CAMPAIGN_WIN:
		case PAUSE_KIND_CAMPAIGN_DEFEAT: {
			static const char *pMsgs[] = {
				[PAUSE_KIND_BATTLE_PAUSE] = "PAUSED",
				[PAUSE_KIND_CAMPAIGN_PAUSE] = "PAUSED",
				[PAUSE_KIND_CAMPAIGN_WIN] = "YOU WIN",
				[PAUSE_KIND_CAMPAIGN_DEFEAT] = "DEFEATED"
			};
			fontDrawStr(
				g_pFontBig, &s_sBmDlgScanline, uwX, uwY, pMsgs[s_eKind], 18 >> 1,
				FONT_COOKIE, g_pTextBitmap
			);
		} break;
		case PAUSE_KIND_BATTLE_SUMMARY: {
			UBYTE ubPlayer = 0;
			UBYTE ubColor = COLOR_P1_BRIGHT + 4 * ubPlayer;
			char szWins[15];
			sprintf(szWins, "PLAYER %hhu WINS", ubPlayer + 1);
			fontDrawStr(
				g_pFontBig, &s_sBmDlgScanline, uwX, uwY, szWins, ubColor >> 1,
				FONT_COOKIE, g_pTextBitmap
			);
		} break;
	}
	uwY += uwRowSize * 3 / 2;

	if(
		s_eKind == PAUSE_KIND_BATTLE_PAUSE || s_eKind == PAUSE_KIND_BATTLE_SUMMARY
	) {
		fontDrawStr(
			g_pFontBig, &s_sBmDlgScanline, uwX, uwY, "CURRENT SCORES", 18 >> 1,
			FONT_COOKIE, g_pTextBitmap
		);
		uwY += uwRowSize;

		if(1) {
			UBYTE *pScores = gameGetScores();
			char szScore[4];
			UBYTE ubPlayerCount = mapDataGetPlayerCount(&g_sMapData);
			UBYTE ubMask = g_sMapData.ubPlayerMask;
			UBYTE i = 0;
			for(UBYTE ubPlayer = 0; ubPlayer < 4; ++ubPlayer) {
				if(ubMask & 1) {
					stringDecimalFromULong(pScores[ubPlayer], szScore);
					UBYTE ubColor = COLOR_P1_BRIGHT + 4 * ubPlayer;
					UWORD uwPlayerX = uwX + (i + 1) * ((uwDlgWidth - 2 * uwX) / (ubPlayerCount + 1));
					fontDrawStr(
						g_pFontBig, &s_sBmDlgScanline, uwPlayerX, uwY, szScore,
						ubColor >> 1, FONT_HCENTER | FONT_COOKIE, g_pTextBitmap
					);
					++i;
				}
				ubMask >>= 1;
			}
		}
		else {
			// teams
		}
		uwY += uwRowSize * 2;
	}

	UBYTE ubOptionCount = 0;

	if(s_eKind == PAUSE_KIND_BATTLE_PAUSE || s_eKind == PAUSE_KIND_CAMPAIGN_PAUSE) {
		s_pPauseLabels[ubOptionCount] = "BACK";
		s_pPauseOptions[ubOptionCount++] = (tOption){
			.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
			.sOptCb = {.cbSelect = onBack}
		};
	}

	if(s_eKind == PAUSE_KIND_CAMPAIGN_WIN) {
		s_pPauseLabels[ubOptionCount] = "CONTINUE";
		s_pPauseOptions[ubOptionCount++] = (tOption){
			.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
			.sOptCb = {.cbSelect = onNextMap}
		};
	}
	else {
		s_pPauseLabels[ubOptionCount] = "RESTART MATCH";
		s_pPauseOptions[ubOptionCount++] = (tOption){
			.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
			.sOptCb = {.cbSelect = onRestart}
		};

		s_pPauseLabels[ubOptionCount] = "EXIT TO MENU";
		s_pPauseOptions[ubOptionCount++] = (tOption){
			.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
			.sOptCb = {.cbSelect = onExit}
		};
	}

	menuListInit(
		s_pPauseOptions, s_pPauseLabels, 3, g_pFontBig, g_pTextBitmap,
		valuePtrPack(COLOR_CONSOLE_BG >> 1), &s_sBmDlgScanline, uwX, uwY,
		&s_sMenuStyle
	);
}

static void gamePauseGsLoop(void) {
	if(!gamePreprocess()) {
		return;
	}

	UBYTE isAnyPlayerMoved = 0;
	for(tPlayerIdx ePlayerIdx = PLAYER_1; ePlayerIdx <= PLAYER_4; ++ePlayerIdx) {
		const tPlayer *pPlayer = playerFromIdx(ePlayerIdx);
		if(steerIsPlayer(pPlayer->pSteer)) {
			steerProcess(pPlayer->pSteer);
			if(steerDirUse(pPlayer->pSteer, DIRECTION_UP)) {
				menuListNavigate(-1);
				isAnyPlayerMoved = 1;
			}
			else if(steerDirUse(pPlayer->pSteer, DIRECTION_DOWN)) {
				menuListNavigate(+1);
				isAnyPlayerMoved = 1;
			}
		}
	}

	menuListDraw();
	dialogProcess(gameGetBackBuffer());

	gamePostprocess();

	// Process pressing fire after gamePostProcess and only if noone moved up/down
	for(tPlayerIdx ePlayerIdx = PLAYER_1; ePlayerIdx <= PLAYER_4; ++ePlayerIdx) {
		const tPlayer *pPlayer = playerFromIdx(ePlayerIdx);
		if(!pPlayer->isDead && steerIsPlayer(pPlayer->pSteer) && !isAnyPlayerMoved) {
			if(steerDirUse(pPlayer->pSteer, DIRECTION_FIRE)) {
				menuListEnter();
				// Don't process anything else past this point
				return;
			}
		}
	}

	if(keyUse(KEY_ESCAPE)) {
		onBack();
	}
}

static void gamePauseGsDestroy(void) {
	dialogDestroy();
}

tState g_sStateGamePause = {
	.cbCreate = gamePauseGsCreate, .cbLoop = gamePauseGsLoop,
	.cbDestroy = gamePauseGsDestroy
};

//--------------------------------------------------------------- MENU CALLBACKS

static void onRestart(void) {
	// Back to original state
	statePop(g_pStateMachineGame);

	// Restart game
	gameRestart();
}

static void onExit(void) {
	// Back to original state
	statePop(g_pStateMachineGame);

	// Make game quit
	gameQuit();
}

static void onBack(void) {
	// Back to original state
	statePop(g_pStateMachineGame);

	// Do nothing else
}

static void onNextMap(void) {

}

//------------------------------------------------------------------- PUBLIC FNS

void gamePauseEnable(tPauseKind eKind) {
	s_eKind = eKind;
	// Switch to pause gamestate - needs pop 'cuz we can't call destroy callback
	// of gameInit/gamePlay here
	statePush(g_pStateMachineGame, &g_sStateGamePause);
}
