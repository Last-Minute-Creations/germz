/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GERMZ_H_
#define _GERMZ_GERMZ_H_

#include <ace/managers/state.h>

extern tStateManager *g_pStateMachineGame;

extern tState
	g_sStateLogo, g_sStateMenu, g_sStateGame, g_sStateGameInit,
	g_sStateGamePlay, g_sStateGamePause, g_sStateEditor, g_sStateDialogSave,
	g_sStateDialogLoad, g_sStateDialogTest, g_sStateCutscene;

#endif // _GERMZ_GERMZ_H_
