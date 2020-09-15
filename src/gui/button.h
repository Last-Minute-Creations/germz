/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_BUTTON_H_
#define _GUI_BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/font.h>

#define BUTTON_MAX_TEXT 20
#define BUTTON_INVALID 0xFF

typedef void (*tBtnOnClick)(void *pData);

typedef struct _tButton {
	tUwRect sRect;
	char szText[BUTTON_MAX_TEXT];
	tBtnOnClick onClick;
	void *pData;
} tButton;

void buttonListCreate(
	UBYTE ubButtonCount, tBitMap *pBfr, const tFont *pFont, tTextBitMap *pTextBfr
);

void buttonListDestroy(void);

tButton *buttonAdd(
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight,
	const char *szText, tBtnOnClick cbOnClick, void *pData
);

void buttonDrawAll(void);

void buttonSelect(tButton *pButton);

void buttonSelectNext(void);

void buttonSelectPrev(void);

tButton *buttonGetSelected(void);

UBYTE buttonListProcessMouseClick(UWORD uwX, UWORD uwY);

void buttonClick(const tButton *pButton);

#ifdef __cplusplus
}
#endif

#endif // _GUI_BUTTON_H_
