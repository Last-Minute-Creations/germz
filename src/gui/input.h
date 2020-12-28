/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_INPUT_H_
#define _GUI_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <ace/utils/font.h>

struct _tGuiInput;

typedef enum _tGuiInputDrawFlags {
	GUI_INPUT_DRAW_BORDER = 1,
	GUI_INPUT_DRAW_TEXT = 2,
	GUI_INPUT_DRAW_CURSOR = 4,
} tGuiInputDrawFlags;

typedef UBYTE (*tGuiInputCbCharAllowed)(char c);
typedef void (*tGuiInputCbDraw)(const struct _tGuiInput *pInput);
typedef UBYTE (*tGuiInputCbGetHeight)(const struct _tGuiInput *pInput);

typedef struct _tGuiInput {
	tUwCoordYX sPos;
	UWORD uwMaxChars;
	UWORD uwValueLength;
	UWORD uwWidth;
	char *szValue;
	const char *szLabel;
	tGuiInputCbDraw cbDraw;
	tGuiInputCbGetHeight cbGetHeight;
	tGuiInputCbCharAllowed cbCharAllowed;
	UBYTE isValueBufferAllocated;
	tGuiInputDrawFlags eDrawFlags;
	UBYTE isFocus;
} tGuiInput;

tGuiInput *inputCreate(
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwMaxChars,
	const char *szLabel, char *szValueBuffer, tGuiInputCbDraw cbDraw,
	tGuiInputCbGetHeight cbGetHeight, tGuiInputCbCharAllowed cbCharAllowed
);

void inputDestroy(tGuiInput *pInput);

void inputProcess(tGuiInput *pInput);

const char *inputGetValue(const tGuiInput *pInput);

UBYTE inputGetHeight(const tGuiInput *pInput);

void inputLoseFocus(tGuiInput *pInput);

void inputSetFocus(tGuiInput *pInput);

#ifdef __cplusplus
}
#endif

#endif // _GUI_INPUT_H_
