/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_save.h"
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/managers/state.h>
#include "gui/dialog.h"
#include "gui/input.h"
#include "gui/button.h"
#include "gui/config.h"
#include "game_editor.h"
#include "game.h"
#include "assets.h"
#include "germz.h"

#define FILE_PATH_PREFIX "data/maps/"
#define FILE_PATH_EXTENSION ".json"
#define FILE_PATH_SIZE ( \
	sizeof(FILE_PATH_PREFIX) - 1 + sizeof(s_szFileName) + \
	sizeof(FILE_PATH_EXTENSION) - 1 + 1 \
)

typedef enum _tSaveInput {
	SAVE_INPUT_TITLE,
	SAVE_INPUT_AUTHOR,
	SAVE_INPUT_FILENAME,
	SAVE_INPUT_COUNT
} tSaveInput;

static tBitMap *s_pBmDialog;
static tInput *s_pInputs[SAVE_INPUT_COUNT];
static tSaveInput s_eCurrentInput;
static tButton *s_pButtonYes, *s_pButtonNo, *s_pButtonCancel;
static tStateManager *s_pDlgStateMachine;
static UBYTE s_isQuitting;

static char s_szFileName[30] = "";
static char s_szFilePath[FILE_PATH_SIZE];

static tState s_sStateOverwrite, s_sStateSelect, s_sStateSaving;

//----------------------------------------------------------------- STATE YES NO

void dialogSaveYesNoCreate(const char **pMsgLines, UBYTE ubLineCount, UBYTE isCancel) {
	UBYTE ubButtonCount = 2;
	if(isCancel) {
		++ubButtonCount;
	}

	dialogClear();
	buttonListCreate(ubButtonCount, s_pBmDialog, g_pFontSmall, g_pTextBitmap);
	const tGuiConfig *pConfig = guiGetConfig();

	UWORD uwDlgWidth = bitmapGetByteWidth(s_pBmDialog) * 8;
	UWORD uwDlgHeight = s_pBmDialog->Rows;
	UWORD uwBtnWidth = 50;
	UWORD uwBtnHeight = 20;
	UWORD uwBtnY = uwDlgHeight - uwBtnHeight - 10;

	for(UBYTE i = 0; i < ubLineCount; ++i) {
		fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, pMsgLines[i]);
		fontDrawTextBitMap(
			s_pBmDialog, g_pTextBitmap, uwDlgWidth / 2,
			uwBtnY / 2 + g_pFontSmall->uwHeight * (i - (ubLineCount / 2)),
			pConfig->ubColorText, FONT_LAZY | FONT_CENTER
		);
	}

	s_pButtonYes = buttonAdd(
		1 * uwDlgWidth / (ubButtonCount + 1) - uwBtnWidth / 2,
		uwDlgHeight - uwBtnHeight - 10, uwBtnWidth, uwBtnHeight, "Yes", 0, 0
	);
	s_pButtonNo = buttonAdd(
		2 * uwDlgWidth / (ubButtonCount + 1) - uwBtnWidth / 2,
		uwDlgHeight - uwBtnHeight - 10, uwBtnWidth, uwBtnHeight, "No", 0, 0
	);
	if(isCancel) {
		s_pButtonCancel = buttonAdd(
			3 * uwDlgWidth / (ubButtonCount + 1) - uwBtnWidth / 2,
			uwDlgHeight - uwBtnHeight - 10, uwBtnWidth, uwBtnHeight, "Cancel", 0, 0
		);
	}
	buttonSelect(s_pButtonNo);
	buttonDrawAll();
}

tButton *dialogSaveYesNoLoop(void) {
	tDirection eDir = gameEditorProcessSteer();
	if(eDir == DIRECTION_LEFT) {
		buttonSelectPrev();
		buttonDrawAll();
	}
	else if(eDir == DIRECTION_RIGHT) {
		buttonSelectNext();
		buttonDrawAll();
	}

	dialogProcess(gameGetBackBuffer());
	gamePostprocess();

	if(eDir == DIRECTION_FIRE || keyUse(KEY_RETURN) || keyUse(KEY_NUMENTER)) {
		return buttonGetSelected();
	}
	else if(keyUse(KEY_ESCAPE)) {
		return s_pButtonCancel;
	}
	return 0;
}

void dialogSaveYesNoDestroy(void) {
	buttonListDestroy();
}

//----------------------------------------------------------------- STATE SELECT

void dialogSaveSelectCreate(void) {
	const tGuiConfig *pConfig = guiGetConfig();
	dialogClear();

	UBYTE ubPadX = 3;
	UBYTE ubPadY = 10;

	buttonListCreate(2, s_pBmDialog, g_pFontSmall, g_pTextBitmap);
	s_pInputs[SAVE_INPUT_TITLE] = inputCreate(
		s_pBmDialog, g_pFontSmall, g_pTextBitmap, ubPadX + 50, ubPadY, 100,
		sizeof(g_sMapData.szName), "Title", g_sMapData.szName
	);
	s_pInputs[SAVE_INPUT_AUTHOR] = inputCreate(
		s_pBmDialog, g_pFontSmall, g_pTextBitmap, ubPadX + 50, ubPadY + 20, 100,
		sizeof(g_sMapData.szAuthor), "Author", g_sMapData.szAuthor
	);
	s_pInputs[SAVE_INPUT_FILENAME] = inputCreate(
		s_pBmDialog, g_pFontSmall, g_pTextBitmap, ubPadX + 50, ubPadY + 40, 100,
		sizeof(s_szFileName), "File name", s_szFileName
	);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, FILE_PATH_EXTENSION);
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap,
		s_pInputs[SAVE_INPUT_FILENAME]->sPos.uwX + s_pInputs[SAVE_INPUT_FILENAME]->uwWidth + 2,
		s_pInputs[SAVE_INPUT_FILENAME]->sPos.uwY + inputGetHeight(s_pInputs[SAVE_INPUT_FILENAME]) / 2,
		pConfig->ubColorText, FONT_COOKIE | FONT_VCENTER
	);

	s_eCurrentInput = 0;
	UWORD uwBtnWidth = 50;
	UWORD uwBtnHeight = 20;
	UWORD uwDlgWidth = bitmapGetByteWidth(s_pBmDialog) * 8;
	UWORD uwDlgHeight = s_pBmDialog->Rows;
	s_pButtonYes = buttonAdd(
		uwDlgWidth / 3 - uwBtnWidth / 2, uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Save", 0, 0
	);
	s_pButtonNo = buttonAdd(
		uwDlgWidth * 2 / 3 - uwBtnWidth / 2, uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Cancel", 0, 0
	);
	buttonDrawAll();
	inputSetFocus(s_pInputs[s_eCurrentInput]);
}

void dialogSaveSelectLoop(void) {
	UBYTE isTab = keyUse(KEY_TAB);
	tDirection eDir = gameEditorProcessSteer();
	if(eDir == DIRECTION_UP || (isTab && keyCheck(KEY_LSHIFT))) {
		if(s_eCurrentInput) {
			if(s_eCurrentInput == SAVE_INPUT_COUNT) {
				// Focus out of button
				buttonSelect(0);
				buttonDrawAll();
			}
			else {
				inputLoseFocus(s_pInputs[s_eCurrentInput]);
			}
			--s_eCurrentInput;
			inputSetFocus(s_pInputs[s_eCurrentInput]);
		}
	}
	else if(eDir == DIRECTION_DOWN || (isTab && !keyCheck(KEY_LSHIFT))) {
		if(s_eCurrentInput < SAVE_INPUT_COUNT) {
			inputLoseFocus(s_pInputs[s_eCurrentInput]);
			++s_eCurrentInput;
			if(s_eCurrentInput == SAVE_INPUT_COUNT) {
				buttonSelect(s_pButtonYes);
				buttonDrawAll();
			}
			else {
				inputSetFocus(s_pInputs[s_eCurrentInput]);
			}
		}
	}
	else if(eDir == DIRECTION_LEFT && s_eCurrentInput == SAVE_INPUT_COUNT) {
		buttonSelect(s_pButtonYes);
		buttonDrawAll();
	}
	else if(eDir == DIRECTION_RIGHT && s_eCurrentInput == SAVE_INPUT_COUNT) {
		buttonSelect(s_pButtonNo);
		buttonDrawAll();
	}

	if(s_eCurrentInput < SAVE_INPUT_COUNT) {
		inputProcess(s_pInputs[s_eCurrentInput]);
	}

	dialogProcess(gameGetBackBuffer());
	gamePostprocess();

	if(eDir == DIRECTION_FIRE || keyUse(KEY_RETURN) || keyUse(KEY_NUMENTER)) {
		if(buttonGetSelected() == s_pButtonYes) {
			if(!strlen(g_sMapData.szName)) {
				// TODO: ERR
			}
			else if(!strlen(g_sMapData.szAuthor)) {
				// TODO: ERR
			}
			else if(!strlen(s_szFileName)) {
				// TODO: ERR
			}
			else {
				strcpy(s_szFilePath, FILE_PATH_PREFIX);
				strcat(s_szFilePath, s_szFileName);
				strcat(s_szFilePath, FILE_PATH_EXTENSION);
				if(fileExists(s_szFilePath)) {
					stateChange(s_pDlgStateMachine, &s_sStateOverwrite);
				}
				else {
					stateChange(s_pDlgStateMachine, &s_sStateSaving);
				}
			}
		}
		else if(buttonGetSelected() == s_pButtonNo) {
			statePop(g_pStateMachineGame);
		}
	}
	else if(keyUse(KEY_ESCAPE)) {
		statePop(g_pStateMachineGame);
	}
}

void dialogSaveSelectDestroy(void) {
	buttonListDestroy();
	for(tSaveInput eInput = 0; eInput < SAVE_INPUT_COUNT; ++eInput) {
		inputDestroy(s_pInputs[eInput]);
	}
}

static tState s_sStateSelect = {
	.cbCreate = dialogSaveSelectCreate, .cbLoop = dialogSaveSelectLoop,
	.cbDestroy = dialogSaveSelectDestroy
};

//-------------------------------------------------------------- STATE OVERWRITE

void dialogSaveOverwriteCreate(void) {
	static const char *szMessages[] = {
		"File already exists",
		s_szFilePath,
		"Do you want to overwrite?",
	};

	dialogSaveYesNoCreate(
		szMessages, sizeof(szMessages) / sizeof(szMessages[0]), 0
	);
}

void dialogSaveOverwriteLoop(void) {
	tButton *pButton = dialogSaveYesNoLoop();
	if(pButton) {
		if(pButton == s_pButtonYes) {
			stateChange(s_pDlgStateMachine, &s_sStateSaving);
		}
		else {
			stateChange(s_pDlgStateMachine, &s_sStateSelect);
		}
	}
}

static tState s_sStateOverwrite = {
	.cbCreate = dialogSaveOverwriteCreate, .cbLoop = dialogSaveOverwriteLoop,
	.cbDestroy = dialogSaveYesNoDestroy
};

//----------------------------------------------------------------- STATE SAVING

static void dialogSaveSavingCreate(void) {
	dialogClear();
	const tGuiConfig *pConfig = guiGetConfig();

	fontFillTextBitMap(
		g_pFontSmall, g_pTextBitmap, "Saving map. Don't turn off the power..."
	);
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap,
		(bitmapGetByteWidth(s_pBmDialog) * 8) / 2, s_pBmDialog->Rows / 2,
		pConfig->ubColorText, FONT_LAZY | FONT_CENTER
	);
}

void dialogSaveSavingLoop(void) {
	dialogProcess(gameGetBackBuffer());
	gamePostprocess();

	mapDataSaveToFile(&g_sMapData, s_szFilePath);
	if(s_isQuitting) {
		statePop(g_pStateMachineGame);
		gameQuit();
	}
	else {
		statePop(g_pStateMachineGame);
	}
}

static tState s_sStateSaving = {
	.cbCreate = dialogSaveSavingCreate, .cbLoop = dialogSaveSavingLoop
};

//--------------------------------------------------------------------- QUITTING

void dialogSaveConfirmQuitCreate(void) {
	static const char *szMessages[] = {
		"Save before quit?",
	};

	dialogSaveYesNoCreate(
		szMessages, sizeof(szMessages) / sizeof(szMessages[0]), 1
	);
}

void dialogSaveConfirmQuitLoop(void) {
	tButton *pButton = dialogSaveYesNoLoop();
	if(pButton) {
		if(pButton == s_pButtonYes) {
			stateChange(s_pDlgStateMachine, &s_sStateSelect);
		}
		else if(pButton == s_pButtonNo) {
			statePop(g_pStateMachineGame); // Pop to game_editor
			gameQuit();
		}
		else {
			statePop(g_pStateMachineGame);
		}
	}
}

static tState s_sStateConfirmQuit = {
	.cbCreate = dialogSaveConfirmQuitCreate, .cbLoop = dialogSaveConfirmQuitLoop,
	.cbDestroy = dialogSaveYesNoDestroy
};

//----------------------------------------------------------------------- DIALOG

static void dialogSaveGsCreate(void) {
	s_eCurrentInput = 0;
	UWORD uwDlgWidth = 256;
	UWORD uwDlgHeight = 128;

	s_pBmDialog = dialogCreate(
		uwDlgWidth, uwDlgHeight, gameGetBackBuffer(), gameGetFrontBuffer()
	);

	s_pDlgStateMachine = stateManagerCreate();
	if(s_isQuitting) {
		statePush(s_pDlgStateMachine, &s_sStateConfirmQuit);
	}
	else {
		statePush(s_pDlgStateMachine, &s_sStateSelect);
	}
}

static void dialogSaveGsLoop(void) {
	if(!gamePreprocess()) {
		return;
	}
	stateProcess(s_pDlgStateMachine);
}

static void dialogSaveGsDestroy(void) {
	stateManagerDestroy(s_pDlgStateMachine);
	dialogDestroy();
}

void dialogSaveShow(UBYTE isQuitting) {
	s_isQuitting = isQuitting;
	statePush(g_pStateMachineGame, &g_sStateDialogSave);
}

void dialogSaveSetSaveName(const char *szName) {
	strcpy(s_szFileName, szName);
}

tState g_sStateDialogSave = {
	.cbCreate = dialogSaveGsCreate, .cbLoop = dialogSaveGsLoop,
	.cbDestroy = dialogSaveGsDestroy
};
