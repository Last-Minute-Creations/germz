/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MAP_LIST_H_
#define _GERMZ_MAP_LIST_H_

#include "gui/list_ctl.h"
#include "map_data.h"

#define MAP_FILENAME_MAX 100

tListCtl *mapListCreateCtl(
	tBitMap *pBmBfr, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
);

UBYTE mapListLoadMap(const tListCtl *pCtrl, tMapData *pMapData);

void mapInfoDrawAuthorTitle(
	const tMapData *pMapData, tBitMap *pBmBuffer, UWORD uwX, UWORD uwY
);

void clearMapInfo(tBitMap *pBmBuffer, UWORD uwX, UWORD uwY);

void mapListDrawPreview(
	const tMapData *pMapData, tBitMap *pBmDest, UWORD uwX, UWORD uwY,
	UBYTE ubTileSize
);

#endif // _GERMZ_MAP_LIST_H_
