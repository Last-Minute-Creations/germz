/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "map_list.h"
#include "assets.h"
#include <ace/utils/dir.h>

tListCtl *mapListCreateCtl(tBitMap *pBg, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
	tListCtl *pCtrl = listCtlCreate(
		pBg, uwX, uwY, uwWidth, uwHeight, g_pFontSmall, 50, g_pTextBitmap, 0
	);

	tDir *pDir = dirOpen("data/maps");
	if(!pDir) {
		dirCreate("data/maps");
		pDir = dirOpen("data/maps");
	}
	if(!pDir) {
		// TODO: something better
		logWrite("Can't open or create maps dir!\n");
		return 0;
	}

	// Count relevant files
	char szFileName[MAP_FILENAME_MAX];
	while(dirRead(pDir, szFileName, MAP_FILENAME_MAX)) {
		UWORD uwLen = strlen(szFileName);
		if(uwLen < 20 && !strcmp(&szFileName[uwLen - 5], ".json")) {
			// Trim extension and add to list
			szFileName[strlen(szFileName) - strlen(".json")] = '\0';
			if(listCtlAddEntry(pCtrl, szFileName) == LISTCTL_ENTRY_INVALID) {
				logWrite("ERR: map limit reached\n");
				break;
			}
		}
	}
	dirClose(pDir);
	listCtlSortEntries(pCtrl);

	return pCtrl;
}

void mapListDrawPreview(
	const tMapData *pMapData, tBitMap *pBmDest, UWORD uwX, UWORD uwY
) {
	static const UBYTE pTileColors[TILE_COUNT] = {
		[TILE_BLOB_P1] = 12,
		[TILE_BLOB_P2] = 16,
		[TILE_BLOB_P3] = 20,
		[TILE_BLOB_P4] = 24,
		[TILE_BLOB_NEUTRAL] = 2,
		[TILE_BLANK] = 3,
		[TILE_EDITOR_BLANK] = 3,
		[TILE_PATH_H1] = 28,
		[TILE_PATH_H2] = 28,
		[TILE_PATH_H3] = 28,
		[TILE_PATH_H4] = 28,
		[TILE_PATH_V1] = 28,
		[TILE_PATH_V2] = 28,
		[TILE_PATH_V3] = 28,
		[TILE_PATH_V4] = 28,
		[TILE_PATH_X1] = 28,
		[TILE_PATH_X2] = 28,
		[TILE_PATH_X3] = 28,
		[TILE_PATH_X4] = 28,
	};

	blitRect(pBmDest, uwX -  1, uwY -  1, 64 + 2,      1, 18);
	blitRect(pBmDest, uwX -  1, uwY -  1,      1, 64 + 2, 18);
	blitRect(pBmDest, uwX     , uwY + 64, 64 + 1,      1, 20);
	blitRect(pBmDest, uwX + 64, uwY     ,      1, 64 + 1, 20);
	const UBYTE ubTileSize = 4;
	for(UBYTE ubTileY = 0; ubTileY < MAP_SIZE; ++ubTileY) {
		for(UBYTE ubTileX = 0; ubTileX < MAP_SIZE; ++ubTileX) {
			UBYTE ubColor = pTileColors[pMapData->pTiles[ubTileX][ubTileY]];
			blitRect(
				pBmDest, uwX + ubTileX * ubTileSize, uwY + ubTileY * ubTileSize,
				ubTileSize, ubTileSize, ubColor
			);
		}
	}
}
