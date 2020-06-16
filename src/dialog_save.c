/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_save.h"
#include <ace/managers/game.h>
#include "gui/dialog.h"
#include "gui/input.h"
#include "game.h"
#include "game_assets.h"

static tBitMap *s_pBmDialog;
static tInput *s_pInput;

void dialogSaveGsCreate(void) {
	s_pBmDialog = dialogCreate(256, 128, gameGetBackBuffer(), gameGetFrontBuffer());
	s_pInput = inputCreate(s_pBmDialog, g_pFont, 3, 3, 100, 30);
}

void dialogSaveGsLoop(void) {
	gamePreprocess();

	dialogProcess(gameGetBackBuffer());

	gamePostprocess();
	if(keyUse(KEY_ESCAPE)) {
		gamePopState();
	}
}

void dialogSaveGsDestroy(void) {
	inputDestroy(s_pInput);
	dialogDestroy();
}



void dialogSaveShow(void) {
	gamePushState(dialogSaveGsCreate, dialogSaveGsLoop, dialogSaveGsDestroy);
}
