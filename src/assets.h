/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_ASSETS_H_
#define _GERMZ_ASSETS_H_

#include <ace/utils/font.h>
#include <ace/utils/ptplayer.h>
#include "map.h"

#define BLOB_COLOR_COUNT 5
#define BLOB_FRAME_COUNT 9
#define MOD_COUNT 2

typedef struct _tNodeModsBase {
	UBYTE ubChargeRate;
	UBYTE ubChargeRateNeutral;
	WORD wCapacity;
} tNodeModsBase;

typedef struct _tDefs {
	tNodeModsBase sBaseMods;
} tDefs;

/**
 * @brief Everything that haven't found its place everywhere else.
 * All those things need to be loaded constantly for game to function properly.
 */

void assetsGlobalCreate(void);

void assetsGlobalDestroy(void);

/**
 * @brief Loads assets from files.
 */
void assetsGameCreate(void);

/**
 * @brief Frees loaded assets.
 */
void assetsGameDestroy(void);

// Global assets
extern tPtplayerMod *g_pMods[MOD_COUNT];
extern UWORD *g_pModSamples;
extern tFont *g_pFontSmall, *g_pFontBig;
extern tTextBitMap *g_pTextBitmap;
extern tBitMap *g_pFrameDisplay;
extern tDefs g_sDefs;
extern tBitMap *g_pCursors, *g_pCursorsMask;
extern tBitMap *g_pBmHudTarget;

// Game assets
extern tBitMap *g_pBmBlobs[BLOB_COLOR_COUNT], *g_pBmBlobMask;
extern tBitMap *g_pBmLinks, *g_pBmLinksMask;
extern tPtplayerSfx *g_pSfxPlep1, *g_pSfxPlep2;

#endif // _GERMZ_GAME_ASSETS_H_
