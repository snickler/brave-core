/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_JSON_PARSER_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_JSON_PARSER_H_

#include <map>
#include <string>

class BinanceJSONParser {
 public:
  static bool GetTokensFromJSON(const std::string& json,
                                std::string *value, std::string type);
  static bool GetAccountBalancesFromJSON(const std::string& json,
                                        std::map<std::string, std::string>*);
};

#endif  // BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_JSON_PARSER_H_
