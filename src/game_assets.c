/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_assets.h"
#include <ace/managers/system.h>

void gameAssetsCreate(void) {
	systemUse();
	// Blob gfx
	for(UBYTE i = 0; i < TILE_BLOB_COUNT; ++i) {
		char szName[15];
		sprintf(szName, "data/blob%hhu.bm", i);
		g_pBmBlobs[i] = bitmapCreateFromFile(szName, 0);
	}
	g_pBmBlobMask = bitmapCreateFromFile("data/blob_mask.bm", 0);

	// Blob links
	g_pBmLinks = bitmapCreateFromFile("data/links.bm", 0);
	g_pBmLinksMask = bitmapCreateFromFile("data/links_mask.bm", 0);
	systemUnuse();

	// Cursors
	g_pCursors = bitmapCreateFromFile("data/cursors.bm", 0);
	g_pCursorsMask = bitmapCreateFromFile("data/cursors_mask.bm", 0);

	// Font
	g_pFont = fontCreate("data/uni54.fnt");
	g_pTextBitmap = fontCreateTextBitMap(320, g_pFont->uwHeight);
}

void gameAssetsDestroy(void) {
	systemUse();
	logBlockBegin("gameAssetsDestroy()");
	// Blob gfx
	for(UBYTE i = 0; i < TILE_BLOB_COUNT; ++i) {
		bitmapDestroy(g_pBmBlobs[i]);
	}
	bitmapDestroy(g_pBmBlobMask);

	// Blob links
	bitmapDestroy(g_pBmLinks);
	bitmapDestroy(g_pBmLinksMask);

	// Cursors
	bitmapDestroy(g_pCursors);
	bitmapDestroy(g_pCursorsMask);

	// Font
	fontDestroy(g_pFont);
	fontDestroyTextBitMap(g_pTextBitmap);
	systemUnuse();
	logBlockEnd("gameAssetsDestroy()");
}

tBitMap *g_pBmBlobs[TILE_BLOB_COUNT], *g_pBmBlobMask;
tBitMap *g_pCursors, *g_pCursorsMask;
tBitMap *g_pBmLinks, *g_pBmLinksMask;
tFont *g_pFont;
tTextBitMap *g_pTextBitmap;
