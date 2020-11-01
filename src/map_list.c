/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "map_list.h"
#include <ace/utils/dir.h>
#include <ace/utils/string.h>
#include "assets.h"
#include "gui/border.h"
#include "color.h"

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

void mapListDrawPreview(
	const tMapData *pMapData, tBitMap *pBmDest, UWORD uwX, UWORD uwY,
	UBYTE ubTileSize
) {
	static const UBYTE pTileColors[TILE_COUNT] = {
		[TILE_BLOB_P1] = 12,
		[TILE_BLOB_P2] = 16,
		[TILE_BLOB_P3] = 20,
		[TILE_BLOB_P4] = 24,
		[TILE_BLOB_NEUTRAL] = 6,
		[TILE_SUPER_P1] = COLOR_P1_BRIGHT,
		[TILE_SUPER_P2] = COLOR_P2_BRIGHT,
		[TILE_SUPER_P3] = COLOR_P3_BRIGHT,
		[TILE_SUPER_P4] = COLOR_P4_BRIGHT,
		[TILE_SUPER_NEUTRAL] = 4,
		[TILE_BLANK] = COLOR_SPECIAL_1,
		[TILE_EDITOR_BLANK] = COLOR_SPECIAL_1,
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

	// const UBYTE ubSize = ubTileSize * 16;
	// guiDraw3dBorder(pBmDest, uwX, uwY, ubSize + 2, ubSize + 2);
	UWORD uwPosY = uwY;
	for(UBYTE ubTileY = 0; ubTileY < MAP_SIZE; ++ubTileY) {
		UWORD uwPosX = uwX;
		for(UBYTE ubTileX = 0; ubTileX < MAP_SIZE; ++ubTileX) {
			UBYTE ubColor = pTileColors[pMapData->pTiles[ubTileX][ubTileY]] >> 1;
			blitRect(pBmDest, uwPosX, uwPosY, ubTileSize, ubTileSize, ubColor);
			uwPosX += ubTileSize;
		}
		uwPosY += ubTileSize;
	}
}

tListCtl *mapListCreateCtl(
	const tBitMap *pBmBg, tBitMap *pBmBfr, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwHeight
) {
	tListCtl *pCtrl = listCtlCreate(
		pBmBg, pBmBfr, uwX, uwY, uwWidth, uwHeight, g_pFontSmall, 50,
		g_pTextBitmap, 0
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

UBYTE updateMapInfo(const tListCtl *pCtrl, tMapData *pMapData) {
	const char *szFile = listCtlGetSelection(pCtrl);
	if(szFile == s_szFilePrev && s_isMapInfoRefreshed) {
		return 0;
	}

	char szPath[MAP_FILENAME_MAX];
	sprintf(szPath, "data/maps/%s.json", szFile);
	mapDataInitFromFile(pMapData, szPath);

	s_isMapInfoRefreshed = 1;
	s_szFilePrev = szFile;
	return 1;
}

void mapInfoDrawAuthorTitle(
	const tMapData *pMapData, const tBitMap *pBmBg, tBitMap *pBmBuffer,
	UWORD uwX, UWORD uwY
) {
	const UBYTE ubRowWidth = 100;
	const UBYTE ubRowHeight = g_pFontSmall->uwHeight + 2;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	fontDrawStr(
		g_pFontSmall, pBmBuffer, uwX, uwY, "Title:",
		18 >> 1, FONT_COOKIE, g_pTextBitmap
	);
	uwY += ubRowHeight;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	fontDrawStr(
		g_pFontSmall, pBmBuffer, uwX + 5, uwY, pMapData->szName,
		18 >> 1, FONT_COOKIE, g_pTextBitmap
	);
	uwY += ubRowHeight;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	fontDrawStr(
		g_pFontSmall, pBmBuffer, uwX, uwY, "Author:",
		18 >> 1, FONT_COOKIE, g_pTextBitmap
	);
	uwY += ubRowHeight;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	fontDrawStr(
		g_pFontSmall, pBmBuffer, uwX + 5, uwY, pMapData->szAuthor,
		18 >> 1, FONT_COOKIE, g_pTextBitmap
	);
}

void clearMapInfo(
	const tBitMap *pBmBg, tBitMap *pBmBuffer, UWORD uwX, UWORD uwY
) {
	const UBYTE ubRowWidth = 100;
	const UBYTE ubRowHeight = g_pFontSmall->uwHeight + 2;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	fontDrawStr(
		g_pFontSmall, pBmBuffer, uwX, uwY, "Loading map...",
		18 >> 1, FONT_COOKIE, g_pTextBitmap
	);
	uwY += ubRowHeight;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	uwY += ubRowHeight;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);
	uwY += ubRowHeight;

	fillBg(pBmBg, pBmBuffer, uwX, uwY, ubRowWidth, ubRowHeight);

	s_isMapInfoRefreshed = 0;
}
