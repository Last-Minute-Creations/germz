#include "game_dialog.h"
#include <stdlib.h>
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/game.h>
#include <ace/utils/dir.h>
#include "gui/button.h"
#include "gui/list_ctl.h"
#include "gui/config.h"
#include "game.h"
#include "game_assets.h"

#define FILENAME_MAX 100

static tBitMap *s_pBmDialog, *s_pBmDialogBg[2];
static UBYTE s_isDlgSavedBg;

static tListCtl *s_pCtrl;

static UBYTE onDlgCreate(void) {
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
	buttonListCreate(5, s_pBmDialog, g_pFont, g_pTextBitmap);
	s_pCtrl = listCtlCreate(
		s_pBmDialog, ubPad, ubPad, uwWidth, uwHeight, g_pFont, 10, g_pTextBitmap, 0
	);

	tDir *pDir = dirOpen("data/maps");
	if(!pDir) {
		dirCreate("data/maps");
		pDir = dirOpen("data/maps");
	}
	if(!pDir) {
		// TODO: something better
		logWrite("Can't open maps dir!\n");
		gamePopState();
		return 0;
	}

	char szFileName[FILENAME_MAX];

	// Count relevant files
	while(dirRead(pDir, szFileName, FILENAME_MAX)) {
		UWORD uwLen = strlen(szFileName);
		if(uwLen < 20 && !strcmp(&szFileName[uwLen - 5], ".json")) {
			listCtlAddEntry(s_pCtrl, szFileName);
		}
	}
	dirClose(pDir);
	listCtlSortEntries(s_pCtrl);

	// blitRect2col(s_pBmDialog, ubPad, ubPad, uwWidth - ubPad - 1, uwHeight - ubPad - 1, 1, 5, 0);
	// UWORD uwOffsY = 3;
	// UWORD uwOffsX = 3;

	fontFillTextBitMap(g_pFont, g_pTextBitmap, "Title: dupa");
	fontDrawTextBitMap(s_pBmDialog, g_pTextBitmap, uwWidth + 3, 3, 5, FONT_COOKIE);

	fontFillTextBitMap(g_pFont, g_pTextBitmap, "Author: dupa");
	fontDrawTextBitMap(s_pBmDialog, g_pTextBitmap, uwWidth + 3, 3 + ubRowSize, 5, FONT_COOKIE);

	buttonAdd(uwWidth + 3, uwHeight - 30, 50, 20, "Load", 0, 0);
	listCtlDraw(s_pCtrl);
	buttonDrawAll();

	// Everything's good!
	return 1;
}

static void onDlgDestroy(void) {
	buttonListDestroy();
	listCtlDestroy(s_pCtrl);
}

static void gameDialogGsCreate(void) {
	systemUse();
	s_pBmDialog = bitmapCreate(256, 128, 5, BMF_INTERLEAVED | BMF_CLEAR);
	s_pBmDialogBg[0] = bitmapCreate(256, 128, 5, BMF_INTERLEAVED);
	s_pBmDialogBg[1] = bitmapCreate(256, 128, 5, BMF_INTERLEAVED);
	systemUnuse();
}

static void gameDialogGsLoop(void) {
	static UBYTE isOdd = 0;

	if(keyUse(KEY_ESCAPE)) {
		gamePopState();
		return;
	}

	if(!gamePreprocess()) {
		return;
	}

	UWORD uwDlgWidth = bitmapGetByteWidth(s_pBmDialog) * 8;
	UWORD uwDlgHeight = s_pBmDialog->Rows;
	UWORD uwOffsX = (320 - uwDlgWidth) / 2;
	UWORD uwOffsY = (256 - uwDlgHeight) / 2;
	if(!s_isDlgSavedBg) {
		blitCopy(
			gameGetBackBuffer(), uwOffsX, uwOffsY, s_pBmDialogBg[isOdd], 0, 0,
			uwDlgHeight, uwDlgHeight, MINTERM_COOKIE, 0xFF
		);
		if(isOdd) {
			onDlgCreate();
			s_isDlgSavedBg = 1;
		}
	}
	else {
		// Draw stuff on dialog bitmap
		tSteer *pSteer = gameGetSteerForPlayer(0);
		tDirection eDir = steerProcess(pSteer);

		if(eDir == DIRECTION_UP) {
			listCtlSelectPrev(s_pCtrl);
		}
		else if(eDir == DIRECTION_DOWN) {
			listCtlSelectNext(s_pCtrl);
		}

		// Copy dialog bitmap to screen
		blitCopy(
			s_pBmDialog, 0, 0, gameGetBackBuffer(), uwOffsX, uwOffsY,
			uwDlgWidth, uwDlgHeight, MINTERM_COOKIE, 0xFF
		);
	}

	isOdd = !isOdd;

	gamePostprocess();
}

static void gameDialogGsDestroy(void) {
	systemUse();
	onDlgDestroy();
	bitmapDestroy(s_pBmDialog);
	bitmapDestroy(s_pBmDialogBg[0]);
	bitmapDestroy(s_pBmDialogBg[1]);
	systemUnuse();
}

void dialogShow(tDialog eDialog) {
	s_isDlgSavedBg = 0;
	gameChangeState(gameDialogGsCreate, gameDialogGsLoop, gameDialogGsDestroy);
}
