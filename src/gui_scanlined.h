/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_SCANLINED_H_
#define _GUI_SCANLINE_H_

// Function callbacks to draw/processed scanlined ingame gui

#include "gui/button.h"
#include "gui/input.h"
#include "gui/list_ctl.h"

void guiScanlinedInit(tBitMap *pBfr);

void guiScanlinedButtonDraw(tGuiButton *pButton);

void guiScanlinedInputDraw(const tGuiInput *pInput);

UBYTE guiScanlinedInputGetHeight(const tGuiInput *pInput);

void guiScanlinedBgClear(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight);

void guiScanlinedListCtlDrawPos(
	tListCtl *pCtl, UWORD uwIdx, UBYTE ubPosOnView, UBYTE isSelected
);

#endif // _GUI_SCANLINE_H_
