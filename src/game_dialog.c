#include "game_dialog.h"
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/game.h>
#include "game.h"
#include "game_assets.h"

static tBitMap *s_pBmDialog, *s_pBmDialogBg[2];
static UBYTE s_isDlgSavedBg;

static void gameDialogGsCreate(void) {
	systemUse();
	s_pBmDialog = bitmapCreate(256, 128, 5, BMF_INTERLEAVED | BMF_CLEAR);
	s_pBmDialogBg[0] = bitmapCreate(256, 128, 5, BMF_INTERLEAVED);
	s_pBmDialogBg[1] = bitmapCreate(256, 128, 5, BMF_INTERLEAVED);
	systemUnuse();
}

static void gameDialogGsDestroy(void) {
	systemUse();
	bitmapDestroy(s_pBmDialog);
	bitmapDestroy(s_pBmDialogBg[0]);
	bitmapDestroy(s_pBmDialogBg[1]);
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
			s_isDlgSavedBg = 1;
		}
	}
	else {
		// Draw stuff on dialog bitmap
		fontFillTextBitMap(g_pFont, g_pTextBitmap, "dupa");
		fontDrawTextBitMap(
			s_pBmDialog, g_pTextBitmap, 0, 0, 5, MINTERM_COOKIE
		);

		// Copy dialog bitmap to screen
		blitCopy(
			s_pBmDialog, 0, 0, gameGetBackBuffer(), uwOffsX, uwOffsY,
			uwDlgWidth, uwDlgHeight, MINTERM_COOKIE, 0xFF
		);
	}

	isOdd = !isOdd;

	gamePostprocess();
}

void dialogShow(tDialog eDialog) {
	s_isDlgSavedBg = 0;
	gameChangeState(gameDialogGsCreate, gameDialogGsLoop, gameDialogGsDestroy);
}
