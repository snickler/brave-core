/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { OrderInfo, CheckoutHost, CheckoutHostListener } from '../interfaces'

function orderInfoFromDialogArgs (): OrderInfo {
  const argString = chrome.getVariableValue('dialogArguments')

  let args: any
  try {
    args = JSON.parse(argString)
  } catch {
    // Ignore
  }

  const { orderInfo } = Object(args)
  // TODO(zenparsing): Throw if orderInfo is invalid?

  return {
    description: orderInfo.description,
    total: orderInfo.total
  }
}

function addWebUIListeners (callbacks: object) {
  for (const key of Object.keys(callbacks)) {
    self.cr.addWebUIListener(key, callbacks[key])
  }
}

interface BalanceDetails {
  total: number
  rates: Record<string, number>
}

interface ExternalWalletDetails {
  status: number
}

export function createHost (): CheckoutHost {
  let hostListener: CheckoutHostListener | null = null
  let balanceDetails: BalanceDetails | null = null
  let externalWalletDetails: ExternalWalletDetails | null = null

  function sendWalletInfo () {
    if (hostListener && balanceDetails && externalWalletDetails) {
      const balance = balanceDetails.total
      // TODO(zenparsing): Any other status codes?
      const verified = externalWalletDetails.status === 1
      hostListener.onWalletUpdated({ balance, verified })
    }
  }

  addWebUIListeners({

    walletBalanceUpdated ({ details }: any) {
      // TODO(zenparsing): Details can be empty if rewards
      // are not enabled and the user does not have a wallet.
      // How do we detect this case without ignoring a real
      // error on startup? If we don't have any details, then
      // how do we get the rates which are required for
      // credit card processing?
      if (!details) {
        return
      }

      balanceDetails = details as BalanceDetails
      sendWalletInfo()

      if (hostListener) {
        hostListener.onExchangeRatesUpdated({
          rates: balanceDetails.rates,
          // TODO(zenparsing): Get from the service
          lastUpdated: new Date().toISOString()
        })
      }
    },

    externalWalletUpdated ({ details }: any) {
      if (!details) {
        return
      }
      externalWalletDetails = details as ExternalWalletDetails
      sendWalletInfo()
    }

  })

  return {

    onDialogClose () {
      chrome.send('dialogClose')
    },

    onPayWithCreditCard (...args) {
      console.log('onPayWithCreditCard', ...args)
      // TODO(zenparsing): Send update to service
    },

    onPayWithWallet (...args) {
      console.log('onPayWithWallet', ...args)
      // TODO(zenparsing): Send update to service
    },

    setListener (listener) {
      if (hostListener === listener) {
        return
      }

      hostListener = listener

      if (hostListener) {
        chrome.send('getWalletBalance')
        chrome.send('getExternalWallet')
        hostListener.onOrderUpdated(orderInfoFromDialogArgs())
      }
    }

  }
}
