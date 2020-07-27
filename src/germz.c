/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/state.h>
#include <ace/utils/ptplayer.h>
#include "germz.h"
#include "logo.h"

tStateManager *g_pStateMachineGame;
static tPtplayerMod *s_pMod;

void genericCreate(void) {
	g_pStateMachineGame = stateManagerCreate();
	keyCreate();
	joyOpen();
	ptplayerCreate(1);
	s_pMod = ptplayerModCreate("data/germz2-25.mod");
	ptplayerLoadMod(s_pMod, 0, 0);
	statePush(g_pStateMachineGame, &g_sStateLogo);
	ptplayerEnableMusic(1);
}

void genericProcess(void) {
	ptplayerProcess();
	keyProcess();
	joyProcess();
	stateProcess(g_pStateMachineGame);
}

void genericDestroy(void) {
	ptplayerModDestroy(s_pMod);
	ptplayerDestroy();
	keyDestroy();
	joyClose();
	stateManagerDestroy(g_pStateMachineGame);
}
