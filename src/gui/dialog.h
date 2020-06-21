/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_DIALOG_H_
#define _GUI_DIALOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/utils/bitmap.h>

typedef void *tCbDialog(void);

tBitMap *dialogCreate(
	UWORD uwWidth, UWORD uwHeight, tBitMap *pBack, tBitMap *pFront
);

void dialogDestroy(void);

void dialogProcess(tBitMap *pBack);

#ifdef __cplusplus
}
#endif

#endif // _GUI_DIALOG_H_
