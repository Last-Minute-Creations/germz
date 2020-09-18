/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "map_list.h"
#include "assets.h"
#include <ace/utils/dir.h>

static const char *s_szFilePrev;
static UBYTE s_isMapInfoRefreshed;

static void fillBg(
	const tBitMap *pBmBg, tBitMap *pBmDest,
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
) {
	if(pBmBg) {
		blitCopy(
			pBmBg, uwX, uwY, pBmDest, uwX, uwY, uwWidth, uwHeight, MINTERM_COOKIE
		);
	}
	else {
		blitRect(pBmDest, uwX, uwY, uwWidth, uwHeight, 0);
	}
}

static void mapListDrawPreview(
	const tMapData *pMapData, tBitMap *pBmDest, UWORD uwX, UWORD uwY,
	UBYTE ubTileSize
) {
	static const UBYTE pTileColors[TILE_COUNT] = {
		[TILE_BLOB_P1] = 12,
		[TILE_BLOB_P2] = 16,
		[TILE_BLOB_P3] = 20,
		[TILE_BLOB_P4] = 24,
		[TILE_BLOB_NEUTRAL] = 2,
		[TILE_SUPER_P1] = 11,
		[TILE_SUPER_P2] = 15,
		[TILE_SUPER_P3] = 19,
		[TILE_SUPER_P4] = 23,
		[TILE_SUPER_NEUTRAL] = 4,
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

	const UBYTE ubSize = ubTileSize * 16;
	blitRect(pBmDest, uwX -      1, uwY -      1, ubSize + 2,          1, 18);
	blitRect(pBmDest, uwX -      1, uwY -      1,          1, ubSize + 2, 18);
	blitRect(pBmDest, uwX         , uwY + ubSize, ubSize + 1,          1, 20);
	blitRect(pBmDest, uwX + ubSize, uwY         ,          1, ubSize + 1, 20);
	UWORD uwPosY = uwY;
	for(UBYTE ubTileY = 0; ubTileY < MAP_SIZE; ++ubTileY) {
		UWORD uwPosX = uwX;
		for(UBYTE ubTileX = 0; ubTileX < MAP_SIZE; ++ubTileX) {
			UBYTE ubColor = pTileColors[pMapData->pTiles[ubTileX][ubTileY]];
			blitRect(pBmDest, uwPosX, uwPosY, ubTileSize, ubTileSize, ubColor);
			uwPosX += ubTileSize;
		}
		uwPosY += ubTileSize;
	}
}

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

	s_isMapInfoRefreshed = 0;
	s_szFilePrev = 0;

	return pCtrl;
}

UBYTE updateMapInfo(
	const tListCtl *pCtrl, const tBitMap *pBmBg, tBitMap *pBmBuffer,
	tMapData *pMapData, UBYTE ubMapPreviewTileSize
) {
	const char *szFile = listCtlGetSelection(pCtrl);
	if(szFile == s_szFilePrev && s_isMapInfoRefreshed) {
		return 0;
	}

	char szPath[MAP_FILENAME_MAX];
	sprintf(szPath, "data/maps/%s.json", szFile);
	mapDataInitFromFile(pMapData, szPath);

	UWORD uwOffsX = pCtrl->sRect.uwX + pCtrl->sRect.uwWidth + 3;
	UWORD uwOffsY = 3;
	const UBYTE ubRowWidth = 100;
	const UBYTE ubRowHeight = g_pFontSmall->uwHeight + 2;

	char szLine[10 + MAX(MAP_NAME_MAX, MAP_AUTHOR_MAX)];
	fillBg(pBmBg, pBmBuffer, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight);
	sprintf(szLine, "Title: %s", pMapData->szName);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, szLine);
	fontDrawTextBitMap(pBmBuffer, g_pTextBitmap, uwOffsX, uwOffsY, 19, FONT_COOKIE);

	uwOffsY += ubRowHeight;
	fillBg(pBmBg, pBmBuffer, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight);
	sprintf(szLine, "Author: %s", pMapData->szAuthor);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, szLine);
	fontDrawTextBitMap(pBmBuffer, g_pTextBitmap, uwOffsX, uwOffsY, 19, FONT_COOKIE);
	uwOffsY += ubRowHeight + 1;

	// Draw map
	UWORD uwPreviewWidth = 2 + ubMapPreviewTileSize * 16;
	uwOffsX += (((bitmapGetByteWidth(pBmBuffer)*8 - uwOffsX) - uwPreviewWidth) / 2);
	mapListDrawPreview(pMapData, pBmBuffer, uwOffsX, uwOffsY, ubMapPreviewTileSize);

	s_isMapInfoRefreshed = 1;
	s_szFilePrev = szFile;
	return 1;
}

void clearMapInfo(
	const tListCtl *pCtrl, const tBitMap *pBmBg, tBitMap *pBmBuffer
) {
	const UWORD uwOffsX = pCtrl->sRect.uwX + pCtrl->sRect.uwWidth + 3;
	UWORD uwOffsY = 3;
	const UBYTE ubRowWidth = 100;
	const UBYTE ubRowHeight = g_pFontSmall->uwHeight + 2;

	fillBg(pBmBg, pBmBuffer, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, "Loading map...");
	fontDrawTextBitMap(pBmBuffer, g_pTextBitmap, uwOffsX, uwOffsY, 19, FONT_COOKIE);

	uwOffsY += ubRowHeight;
	fillBg(pBmBg, pBmBuffer, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight);

	s_isMapInfoRefreshed = 0;
}
