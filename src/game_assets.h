/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GAME_ASSETS_H_
#define _GERMZ_GAME_ASSETS_H_

#include "map.h"
#include <ace/utils/font.h>
#define BLOB_FRAME_COUNT 9

/**
 * @brief Everything that haven't found its place everywhere else.
 * All those things need to be loaded constantly for game to function properly.
 */

/**
 * @brief Loads assets from files.
 */
void gameAssetsCreate(void);

/**
 * @brief Frees loaded assets.
 */
void gameAssetsDestroy(void);

extern tBitMap *g_pBmBlobs[TILE_BLOB_COUNT], *g_pBmBlobMask;
extern tBitMap *g_pCursors, *g_pCursorsMask;
extern tBitMap *g_pBmLinks, *g_pBmLinksMask;
extern tFont *g_pFont;
extern tTextBitMap *g_pTextBitmap;

#endif // _GERMZ_GAME_ASSETS_H_
