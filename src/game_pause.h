/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GAME_PAUSE_H_
#define _GERMZ_GAME_PAUSE_H_

typedef enum _tPauseKind {
	PAUSE_KIND_PAUSE,
	PAUSE_KIND_BATTLE_SUMMARY,
	PAUSE_KIND_CAMPAIGN_DEFEAT,
} tPauseKind;

void gamePauseEnable(tPauseKind eKind);

#endif // _GERMZ_GAME_PAUSE_H_
