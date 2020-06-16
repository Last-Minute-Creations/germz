/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "input.h"
#include <ace/managers/system.h>
#include "config.h"
#include "border.h"

tInput *inputCreate(
	tBitMap *pBg, tFont *pFont, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwMaxChars
) {
	systemUse();
	tInput *pInput = memAllocFast(sizeof(*pInput));
	if(!pInput) {
		goto fail;
	}
	pInput->pValue = memAllocFast(uwMaxChars);
	if(!pInput->pValue) {
		goto fail;
	}
	systemUnuse();

	pInput->sPos.uwX = uwX;
	pInput->sPos.uwY = uwY;
	pInput->uwWidth = uwWidth;
	pInput->uwMaxChars = uwMaxChars;
	pInput->pFont = pFont;
	pInput->pBfr = pBg;

	// Initial draw
	guiDraw3dBorder(pBg, uwX, uwY, uwWidth, pFont->uwHeight + 4);

	return pInput;
fail:
	inputDestroy(pInput);
	systemUnuse();
	return 0;
}

void inputDestroy(tInput *pInput) {
	systemUse();
	if(pInput) {
		if(pInput->pValue) {
			memFree(pInput->pValue, pInput->uwMaxChars);
		}
		memFree(pInput, sizeof(*pInput));
	}
	systemUnuse();
}

void inputProcess(tInput *pInput) {
	// Process keystrokes

	// Draw changed letters + cursor
}
