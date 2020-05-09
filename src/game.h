/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GAME_H_
#define _GERMZ_GAME_H_

#include "map_data.h"
#include <ace/utils/font.h>
#include "steer.h"
#include "bob_new.h"

#define HUD_OFFS_X 256
#define HUD_MONITOR_SIZE 64

//------------------------------------------------------------------------ UTILS

void gameCopyBackToFront(void);

tBitMap *gameGetBackBuffer(void);

void gameInitMap(void);

//------------------------------------------------------------------------ UTILS

void gameGsCreate(void);

void gameGsLoop(void);

void gameGsDestroy(void);

UBYTE gamePreprocess(void);

void gamePostprocess(void);

/**
 * @brief
 *
 * @param ubPlayer 0: P1
 */
tSteer *gameGetSteerForPlayer(UBYTE ubPlayer);

void gameDrawBlobAt(tTile eTile, UBYTE ubFrame, UBYTE ubTileX, UBYTE ubTileY);

void gameDrawMapTileAt(UBYTE ubX, UBYTE ubY, UBYTE ubFrame);

void gameDrawTileAt(tTile eTile, UBYTE ubX, UBYTE ubY, UBYTE ubFrame);

tBobNew *gameGetCursorBob(UBYTE ubIdx);

#endif // _GERMZ_GAME_H_
