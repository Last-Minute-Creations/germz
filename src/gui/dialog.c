/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog.h"
#include <ace/managers/blit.h>
#include <ace/managers/system.h>

static tBitMap *s_pBmDialog, *s_pBmDialogBgFirst, *s_pBmDialogBgSecond;
static tBitMap *s_pBmFirst, *s_pBmSecond; // For synchronizing save/restore
static UWORD s_uwWidth, s_uwHeight, s_uwOffsX, s_uwOffsY;

tBitMap *dialogCreate(
	UWORD uwWidth, UWORD uwHeight, tBitMap *pBack, tBitMap *pFront
) {
	s_uwWidth = uwWidth;
	s_uwHeight = uwHeight;
	s_uwOffsX = (320 - s_uwWidth) / 2;
	s_uwOffsY = (256 - s_uwHeight) / 2;

	UBYTE ubFlags = (bitmapIsInterleaved(pBack) ? BMF_INTERLEAVED : 0);
	systemUse();
	s_pBmDialog = bitmapCreate(s_uwWidth, s_uwHeight, pBack->Depth, ubFlags | BMF_CLEAR);
	s_pBmDialogBgFirst = bitmapCreate(s_uwWidth, s_uwHeight, pBack->Depth, ubFlags);
	systemUnuse();
	s_pBmFirst = pBack;
	blitCopy(
		s_pBmFirst, s_uwOffsX, s_uwOffsY, s_pBmDialogBgFirst, 0, 0,
		s_uwWidth, s_uwHeight, MINTERM_COOKIE, 0xFF
	);

	if(pFront != pBack) {
		s_pBmSecond = pFront;
		s_pBmDialogBgSecond = bitmapCreate(s_uwWidth, s_uwHeight, pBack->Depth, ubFlags);
		blitCopy(
			s_pBmSecond, s_uwOffsX, s_uwOffsY, s_pBmDialogBgSecond, 0, 0,
			s_uwWidth, s_uwHeight, MINTERM_COOKIE, 0xFF
		);
	}
	else {
		s_pBmSecond = 0;
	}
	return s_pBmDialog;
}

void dialogDestroy(void) {
	blitCopy(
		s_pBmDialogBgFirst, 0, 0, s_pBmFirst, s_uwOffsX, s_uwOffsY,
		s_uwWidth, s_uwHeight, MINTERM_COOKIE, 0xFF
	);
	if(s_pBmSecond) {
		blitCopy(
			s_pBmDialogBgSecond, 0, 0, s_pBmSecond, s_uwOffsX, s_uwOffsY,
			s_uwWidth, s_uwHeight, MINTERM_COOKIE, 0xFF
		);
		bitmapDestroy(s_pBmDialogBgSecond);
	}
	systemUse();
	bitmapDestroy(s_pBmDialog);
	bitmapDestroy(s_pBmDialogBgFirst);
	systemUnuse();
}

void dialogProcess(tBitMap *pBack) {
	// Copy current dialog bitmap contents to backbuffer
	blitCopy(
		s_pBmDialog, 0, 0, pBack, s_uwOffsX, s_uwOffsY,
		s_uwWidth, s_uwHeight, MINTERM_COOKIE, 0xFF
	);
}
