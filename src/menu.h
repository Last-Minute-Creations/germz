/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MENU_H_
#define _GERMZ_MENU_H_

#include "steer.h"

/**
 * @brief
 *
 * @param ubPlayerIdx 0: P1
 * @return tSteer
 */
tSteer menuGetSteerForPlayer(UBYTE ubPlayerIdx);

/**
 * @brief
 *
 * @param ubPlayerIdx 0: P1
 * @return UBYTE
 */
UBYTE menuIsPlayerActive(UBYTE ubPlayerIdx);

void menuStartWithCampaignResult(void);

#endif // _GERMZ_MENU_H_
