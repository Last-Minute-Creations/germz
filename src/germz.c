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
#include "assets.h"

tStateManager *g_pStateMachineGame;

static UBYTE s_ubCurrentMod = 0;

static void onSongEnd(void) {
	if(++s_ubCurrentMod >= MOD_COUNT) {
		s_ubCurrentMod = 0;
	}
	ptplayerLoadMod(g_pMods[s_ubCurrentMod], g_pModSamples, 0);
	ptplayerEnableMusic(1);
}

void genericCreate(void) {
	g_pStateMachineGame = stateManagerCreate();
	keyCreate();
	joyOpen();
	ptplayerCreate(1);
	assetsGlobalCreate();
	ptplayerLoadMod(g_pMods[s_ubCurrentMod], g_pModSamples, 0);
	ptplayerConfigureSongRepeat(0, onSongEnd);
	statePush(g_pStateMachineGame, &g_sStateLogo);
}

void genericProcess(void) {
	ptplayerProcess();
	keyProcess();
	joyProcess();
	stateProcess(g_pStateMachineGame);
}

void genericDestroy(void) {
	ptplayerDestroy();
	assetsGlobalDestroy();
	keyDestroy();
	joyClose();
	stateManagerDestroy(g_pStateMachineGame);
}
