/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_load.h"
#include <gui/dialog.h>
#include <gui/list_ctl.h>
#include <gui/button.h>
#include <gui/config.h>
#include <ace/utils/dir.h>
#include <ace/managers/game.h>
#include "game.h"
#include "game_assets.h"

#define FILENAME_MAX 100

static tListCtl *s_pCtrl;

void dialogLoadGsCreate(void) {
	tBitMap *pBmDialog = dialogCreate(256, 128, gameGetBackBuffer(), gameGetFrontBuffer());

	// Initial draw
	UBYTE ubRowSize = g_pFont->uwHeight + 2;
	UBYTE ubPad = 1;
	UWORD uwWidth = 128;
	UWORD uwHeight = 126;

	tGuiConfig *pConfig = guiGetConfig();
	pConfig->ubColorLight = 16;
	pConfig->ubColorDark = 19;
	pConfig->ubColorFill = 18;
	pConfig->ubColorText = 17;
	buttonListCreate(5, pBmDialog, g_pFont, g_pTextBitmap);
	s_pCtrl = listCtlCreate(
		pBmDialog, ubPad, ubPad, uwWidth, uwHeight, g_pFont, 10, g_pTextBitmap, 0
	);

	tDir *pDir = dirOpen("data/maps");
	if(!pDir) {
		dirCreate("data/maps");
		pDir = dirOpen("data/maps");
	}
	if(!pDir) {
		// TODO: something better
		logWrite("Can't open or create maps dir!\n");
		gamePopState();
		return;
	}

	// Count relevant files
	char szFileName[FILENAME_MAX];
	while(dirRead(pDir, szFileName, FILENAME_MAX)) {
		UWORD uwLen = strlen(szFileName);
		if(uwLen < 20 && !strcmp(&szFileName[uwLen - 5], ".json")) {
			listCtlAddEntry(s_pCtrl, szFileName);
		}
	}
	dirClose(pDir);
	listCtlSortEntries(s_pCtrl);

	// blitRect2col(pBmDialog, ubPad, ubPad, uwWidth - ubPad - 1, uwHeight - ubPad - 1, 1, 5, 0);
	// UWORD uwOffsY = 3;
	// UWORD uwOffsX = 3;

	fontFillTextBitMap(g_pFont, g_pTextBitmap, "Title: dupa");
	fontDrawTextBitMap(pBmDialog, g_pTextBitmap, uwWidth + 3, 3, 17, FONT_COOKIE);

	fontFillTextBitMap(g_pFont, g_pTextBitmap, "Author: dupa");
	fontDrawTextBitMap(pBmDialog, g_pTextBitmap, uwWidth + 3, 3 + ubRowSize, 17, FONT_COOKIE);

	buttonAdd(uwWidth + 3, uwHeight - 30, 50, 20, "Load", 0, 0);
	listCtlDraw(s_pCtrl);
	buttonDrawAll();
}

void dialogLoadGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gamePopState();
		return;
	}

	if(!gamePreprocess()) {
		return;
	}

	tSteer *pSteer = gameGetSteerForPlayer(0);
	tDirection eDir = steerProcess(pSteer);

	if(eDir == DIRECTION_UP) {
		listCtlSelectPrev(s_pCtrl);
	}
	else if(eDir == DIRECTION_DOWN) {
		listCtlSelectNext(s_pCtrl);
	}

	// Copy dialog bitmap to screen
	dialogProcess(gameGetBackBuffer());

	gamePostprocess();
}

void dialogLoadGsDestroy(void) {
	buttonListDestroy();
	listCtlDestroy(s_pCtrl);
	dialogDestroy();
}


void dialogLoadShow(void) {
	gamePushState(dialogLoadGsCreate, dialogLoadGsLoop, dialogLoadGsDestroy);
}
