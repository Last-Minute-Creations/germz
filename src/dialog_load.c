/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_load.h"
#include <gui/dialog.h>
#include <gui/list_ctl.h>
#include <gui/button.h>
#include <ace/utils/dir.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include "game_editor.h"
#include "dialog_save.h"
#include "game.h"
#include "assets.h"
#include "germz.h"
#include "map_list.h"

static tListCtl *s_pCtrl;
static tMapData *s_pPreview;
static tBitMap *s_pBmDialog;
static ULONG s_ullChangeTimer;

static void dialogLoadGsCreate(void) {
	UWORD uwDlgWidth = 256;
	UWORD uwDlgHeight = 128;
	s_pBmDialog = dialogCreate(
		uwDlgWidth, uwDlgHeight, gameGetBackBuffer(), gameGetFrontBuffer()
	);
	s_pPreview = memAllocFast(sizeof(*s_pPreview));

	// Initial draw
	UBYTE ubPad = 1;
	UWORD uwWidth = uwDlgWidth / 2;
	UWORD uwHeight = uwDlgHeight - 2;

	buttonListCreate(5, s_pBmDialog, g_pFontSmall, g_pTextBitmap);
	s_pCtrl = mapListCreateCtl(s_pBmDialog, ubPad, ubPad, uwWidth, uwHeight);
	if(!s_pCtrl) {
		statePop(g_pStateMachineGame);
	}

	updateMapInfo(s_pCtrl, 0, s_pBmDialog, s_pPreview, 4);
	s_ullChangeTimer = timerGet();

	const UWORD uwBtnWidth = 50;
	const UWORD uwBtnHeight = 20;
	tButton *pButtonLoad = buttonAdd(
		uwWidth + (uwDlgWidth - uwWidth - uwBtnWidth) / 2,
		uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Load", 0, 0
	);
	listCtlDraw(s_pCtrl);
	buttonSelect(pButtonLoad);
	buttonDrawAll();
}

static void dialogLoadGsLoop(void) {
	if(!gamePreprocess()) {
		return;
	}

	UBYTE isMapSelected = 0;
	tDirection eDir = gameEditorGetSteerDir();
	if(eDir == DIRECTION_UP) {
		listCtlSelectPrev(s_pCtrl);
		clearMapInfo(s_pCtrl, 0, s_pBmDialog);
		s_ullChangeTimer = timerGet();
	}
	else if(eDir == DIRECTION_DOWN) {
		listCtlSelectNext(s_pCtrl);
		clearMapInfo(s_pCtrl, 0, s_pBmDialog);
		s_ullChangeTimer = timerGet();
	}
	else if(eDir == DIRECTION_FIRE || keyUse(KEY_RETURN) || keyUse(KEY_NUMENTER)) {
		// No processing via the OK button callback - code is shorter that way
		updateMapInfo(s_pCtrl, 0, s_pBmDialog, s_pPreview, 4);
		memcpy(&g_sMapData, s_pPreview, sizeof(g_sMapData));
		dialogSaveSetSaveName(listCtlGetSelection(s_pCtrl));
		isMapSelected = 1;
	}

	if(timerGetDelta(s_ullChangeTimer, timerGet()) >= 25) {
		updateMapInfo(s_pCtrl, 0, s_pBmDialog, s_pPreview, 4);
		s_ullChangeTimer = timerGet();
	}

	// Copy dialog bitmap to screen
	dialogProcess(gameGetBackBuffer());

	gamePostprocess();

	if(isMapSelected || keyUse(KEY_ESCAPE)) {
		statePop(g_pStateMachineGame);
	}
}

static void dialogLoadGsDestroy(void) {
	memFree(s_pPreview, sizeof(*s_pPreview));
	buttonListDestroy();
	listCtlDestroy(s_pCtrl);
	dialogDestroy();
}


void dialogLoadShow(void) {
	statePush(g_pStateMachineGame, &g_sStateDialogLoad);
}

tState g_sStateDialogLoad = {
	.cbCreate = dialogLoadGsCreate, .cbLoop = dialogLoadGsLoop,
	.cbDestroy = dialogLoadGsDestroy
};
