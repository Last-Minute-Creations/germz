/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GAME_DIALOG_H_
#define _GERMZ_GAME_DIALOG_H_

typedef enum _tDialog {
	DIALOG_SAVE,
	DIALOG_LOAD,
	DIALOG_QUIT,
} tDialog;

void dialogShow(tDialog eDialog);

#endif // _GERMZ_GAME_DIALOG_H_
