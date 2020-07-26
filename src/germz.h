#ifndef _GERMZ_GERMZ_H_
#define _GERMZ_GERMZ_H_

#include <ace/managers/state.h>

#define STATE(a_cbCreate, a_cbLoop, a_cbDestroy, a_cbSuspend, a_cbResume) \
	{ \
		.cbCreate = a_cbCreate, .cbLoop = a_cbLoop, .cbDestroy = a_cbDestroy, \
		.cbSuspend = a_cbSuspend, .cbResume = a_cbResume, .pPrev = 0 \
	}

extern tStateManager *g_pStateMachineGame;

extern tState
	g_sStateLogo, g_sStateMenu, g_sStateGame, g_sStateGameInit,
	g_sStateGamePlay, g_sStateGameSummary, g_sStateEditor, g_sStateDialogSave,
	g_sStateDialogLoad;

#endif // _GERMZ_GERMZ_H_
