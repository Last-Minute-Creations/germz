/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GAME_H_
#define _GERMZ_GAME_H_

#include "map_data.h"
#include <ace/utils/font.h>
#include "steer.h"
#include "bob_new.h"
#include "fade.h"

#define HUD_OFFS_X 256
#define HUD_MONITOR_SIZE 64

//------------------------------------------------------------------------ UTILS

void gameCopyBackToFront(void);

tBitMap *gameGetBackBuffer(void);

tBitMap *gameGetFrontBuffer(void);

void gameInitMap(void);

//------------------------------------------------------------------------ UTILS

UBYTE gamePreprocess(void);

void gamePostprocess(void);

tSteer *gameGetSteerForPlayer(UBYTE ubPlayer);

void gameDrawBlobAt(tTile eTile, UBYTE ubFrame, UWORD uwX, UWORD uwY);

void gameDrawMapTileAt(tUbCoordYX sPosTile, UBYTE ubFrame);

void gameDrawTileAt(tTile eTile, UWORD uwX, UWORD uwY, UBYTE ubFrame);

void gameInitCursorBobs(void);

void gameSetEditor(UBYTE isEditor);

tBobNew *gameGetCursorBob(UBYTE ubIdx);

tFade *gameGetFade(void);

void gameQuit(void);

#endif // _GERMZ_GAME_H_
