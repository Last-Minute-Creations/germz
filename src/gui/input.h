/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_INPUT_H_
#define _GUI_INPUT_H_

#include <ace/types.h>
#include <ace/utils/font.h>

typedef struct _tInput {
	tUwCoordYX sPos;
	UWORD uwMaxChars;
	UWORD uwValueLength;
	UWORD uwWidth;
	char *szValue;
	tFont *pFont;
	tBitMap *pBfr;
	tTextBitMap *pTextBitMap;
	UBYTE isTextBitMapAllocated;
	UBYTE isValueBufferAllocated;
} tInput;

tInput *inputCreate(
	tBitMap *pBg, tFont *pFont, tTextBitMap *pTextBitMap, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwMaxChars, const char *szLabel, char *szValueBuffer
);

void inputDestroy(tInput *pInput);

void inputProcess(tInput *pInput);

const char *inputGetValue(const tInput *pInput);

UBYTE inputGetHeight(const tInput *pInput);

void inputLoseFocus(tInput *pInput);

void inputSetFocus(tInput *pInput);

#endif // _GUI_INPUT_H_
