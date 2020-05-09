/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_summary.h"
#include <ace/managers/game.h>

void gameSummaryGsCreate(void) {
	// Go to game -> will go to menu
	gamePopState();
}

void gameSummaryGsLoop(void) {

}

void gameSummaryGsDestroy(void) {

}
