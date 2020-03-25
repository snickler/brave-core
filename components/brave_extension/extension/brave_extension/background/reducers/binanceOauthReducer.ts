/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as webNavigationTypes from '../../constants/webNavigationTypes'
import { Actions } from '../../types/actions/index'

export default function binanceOauthReducer (state = {}, action: Actions) {
  switch (action.type) {
    case webNavigationTypes.ON_COMMITTED: {
      console.log('----starts with check: ', action.url)
      if (action.isMainFrame && action.url.startsWith('https://brave.com/binance/oauth/callback?code=')) {
        console.log('---yep')
        chrome.tabs.remove(action.tabId, () => {})
      }
      break
    }
  }
  return state
}
