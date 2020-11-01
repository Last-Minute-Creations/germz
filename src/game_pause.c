/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_pause.h"
#include <ace/managers/key.h>
#include <ace/utils/bmframe.h>
#include "germz.h"
#include "gui/dialog.h"
#include "game.h"
#include "assets.h"
#include "color.h"
#include "menu_list.h"
#include "player.h"
#include "value_ptr.h"

static tBitMap *s_pBmDialog;
static tBitMap s_sBmDlgScanline;

static void onRestart(void);
static void onExit(void);
static void onBack(void);

static const tMenuListStyle s_sMenuStyle = {
	.ubColorActive = COLOR_P3_BRIGHT >> 1,
	.ubColorInactive = (COLOR_P3_BRIGHT + 2) >> 1,
	.ubColorShadow = 0xFF
};

static tOption s_pPauseOptions[] = {
	{
		.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
		.sOptCb = {.cbSelect = onBack}
	},
	{
		.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
		.sOptCb = {.cbSelect = onRestart}
	},
	{
		.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .pStyle = &s_sMenuStyle,
		.sOptCb = {.cbSelect = onExit}
	},
};

static const char *s_pPauseLabels[] = {
	"BACK", "RESTART MATCH", "EXIT TO MENU"
};

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

	fontDrawStr(
		g_pFontBig, &s_sBmDlgScanline, uwX, uwY, "PAUSED", 18 >> 1,
		FONT_COOKIE, g_pTextBitmap
	);
	uwY += uwRowSize * 3 / 2;

	if(1) {
		fontDrawStr(
			g_pFontBig, &s_sBmDlgScanline, uwX, uwY, "CURRENT SCORES", 18 >> 1,
			FONT_COOKIE, g_pTextBitmap
		);
		uwY += uwRowSize;

		if(1) {
			static const UBYTE pPlayerColors[] = {
				COLOR_P1_BRIGHT >> 1, COLOR_P2_BRIGHT >> 1,
				COLOR_P3_BRIGHT >> 1, COLOR_P4_BRIGHT >> 1
			};
			for(UBYTE i = 0; i < 4; ++i) {
				fontDrawStr(
					g_pFontBig, &s_sBmDlgScanline, 32 + i * 30, uwY, "9",
					pPlayerColors[i], FONT_COOKIE, g_pTextBitmap
				);
			}
		}
		else {
			// teams
		}
		uwY += uwRowSize * 2;
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

//------------------------------------------------------------------- PUBLIC FNS

void gamePauseEnable(tPauseKind eKind) {
	// Switch to pause gamestate - needs pop 'cuz we can't call destroy callback
	// of gameInit/gamePlay here
	statePush(g_pStateMachineGame, &g_sStateGamePause);
}
