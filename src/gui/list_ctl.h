/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_LIST_CTL_H_
#define _GUI_LIST_CTL_H_

#include <ace/types.h>
#include <ace/utils/font.h>

#define LISTCTL_ENTRY_INVALID 0xFFFF
#define LISTCTL_DRAWSTATE_OK 0
#define LISTCTL_DRAWSTATE_NEEDS_REDRAW 1

typedef void (*tCbListCtlOnSelect)(void);

typedef struct _tListCtl {
	tUwRect sRect;
	UBYTE ubDrawState;
	UBYTE ubEntryHeight;
	UWORD uwEntryCnt;
	UWORD uwEntryMaxCnt;
	UWORD uwEntrySel;
	char **pEntries;
	tFont *pFont;
	tBitMap *pBfr;
	tCbListCtlOnSelect cbOnSelect;
	tTextBitMap *pEntryTextBfr;
	UBYTE isTextBfrAlloc;
} tListCtl;

tListCtl *listCtlCreate(
	tBitMap *pBfr, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight,
	tFont *pFont, UWORD uwEntryMaxCnt, tTextBitMap *pTextBfr,
	tCbListCtlOnSelect cbOnSelect
);

void listCtlDestroy(tListCtl *pCtl);

UWORD listCtlAddEntry(tListCtl *pCtl, const char *szTxt);

void listCtlRemoveEntry(tListCtl *pCtl, UWORD uwIdx);

void listCtlDraw(tListCtl *pCtl);

UBYTE listCtlProcessClick(tListCtl *pCtl, UWORD uwMouseX, UWORD uwMouseY);

void listCtlSelectPrev(tListCtl *pCtl);

void listCtlSelectNext(tListCtl *pCtl);

void listCtlSortEntries(tListCtl *pCtl);

#endif // _GUI_LIST_CTL_H_
