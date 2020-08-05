#ifndef _GERMZ_GERMZ_H_
#define _GERMZ_GERMZ_H_

#include <ace/managers/state.h>

extern tStateManager *g_pStateMachineGame;

extern tState
	g_sStateLogo, g_sStateMenu, g_sStateGame, g_sStateGameInit,
	g_sStateGamePlay, g_sStateGameSummary, g_sStateEditor, g_sStateDialogSave,
	g_sStateDialogLoad;

#endif // _GERMZ_GERMZ_H_
