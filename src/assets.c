/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "assets.h"
#include "gui/config.h"
#include <ace/managers/system.h>
#include <json/json.h>

void assetsGlobalCreate(void) {
	// Defs
	tJson *pJsonDefs = jsonCreate("data/defs.json");

	UWORD uwTokJson = jsonGetDom(pJsonDefs, "baseMods.ubChargeRate");
	ULONG ulVal = jsonTokToUlong(pJsonDefs, uwTokJson);
	g_sDefs.sBaseMods.ubChargeRate = ulVal;

	uwTokJson = jsonGetDom(pJsonDefs, "baseMods.ubChargeRateNeutral");
	ulVal = jsonTokToUlong(pJsonDefs, uwTokJson);
	g_sDefs.sBaseMods.ubChargeRateNeutral = ulVal;

	uwTokJson = jsonGetDom(pJsonDefs, "baseMods.wCapacity");
	ulVal = jsonTokToUlong(pJsonDefs, uwTokJson);
	g_sDefs.sBaseMods.wCapacity = ulVal;

	jsonDestroy(pJsonDefs);

	// Mods
	g_pMod = ptplayerModCreate("data/germz2-25.mod");

	// Font
	g_pFontSmall = fontCreate("data/uni54.fnt");
	g_pFontBig = fontCreate("data/germz.fnt");
	g_pTextBitmap = fontCreateTextBitMap(320, g_pFontBig->uwHeight);

	// Bitmaps
	g_pFrameDisplay = bitmapCreateFromFile("data/display.bm", 0);

	tGuiConfig *pConfig = guiGetConfig();
	pConfig->ubColorLight = 20 >> 1;
	pConfig->ubColorDark = 20 >> 1;
	pConfig->ubColorFill = 20 >> 1;
	pConfig->ubColorText = 18 >> 1;
}

void assetsGlobalDestroy(void) {
	// Mods
	ptplayerModDestroy(g_pMod);

	// Font
	fontDestroy(g_pFontSmall);
	fontDestroy(g_pFontBig);
	fontDestroyTextBitMap(g_pTextBitmap);

	// Bitmaps
	bitmapDestroy(g_pFrameDisplay);
}

void assetsGameCreate(void) {
	systemUse();
	// Blob gfx
	for(UBYTE i = 0; i < BLOB_COLOR_COUNT; ++i) {
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

	// Sounds
	g_pSfxPlep1 = ptplayerSfxCreateFromFile("data/sfx/plep1.sfx");
	g_pSfxPlep2 = ptplayerSfxCreateFromFile("data/sfx/plep2.sfx");

	// HUD icon
	g_pBmHudTarget = bitmapCreateFromFile("data/hud_target.bm", 0);
}

void assetsGameDestroy(void) {
	systemUse();
	logBlockBegin("gameAssetsDestroy()");
	// Blob gfx
	for(UBYTE i = 0; i < BLOB_COLOR_COUNT; ++i) {
		bitmapDestroy(g_pBmBlobs[i]);
	}
	bitmapDestroy(g_pBmBlobMask);

	// Blob links
	bitmapDestroy(g_pBmLinks);
	bitmapDestroy(g_pBmLinksMask);

	// Cursors
	bitmapDestroy(g_pCursors);
	bitmapDestroy(g_pCursorsMask);

	// Sounds
	ptplayerSfxDestroy(g_pSfxPlep1);
	ptplayerSfxDestroy(g_pSfxPlep2);

	// HUD icon
	bitmapDestroy(g_pBmHudTarget);

	systemUnuse();
	logBlockEnd("gameAssetsDestroy()");
}

// Global assets
tPtplayerMod *g_pMod;
tFont *g_pFontSmall, *g_pFontBig;
tTextBitMap *g_pTextBitmap;
tBitMap *g_pFrameDisplay;
tDefs g_sDefs;

// Game assets
tBitMap *g_pBmBlobs[BLOB_COLOR_COUNT], *g_pBmBlobMask;
tBitMap *g_pCursors, *g_pCursorsMask;
tBitMap *g_pBmLinks, *g_pBmLinksMask;
tBitMap *g_pBmHudTarget;
tPtplayerSfx *g_pSfxPlep1, *g_pSfxPlep2;
