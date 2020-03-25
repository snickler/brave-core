/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/suggested_sites_provider.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/history_provider.h"

// As from autocomplete_provider.h:
// Search Secondary Provider (suggestion)                              |  100++
const int SuggestedSitesProvider::kRelevance = 1;


SuggestedSitesProvider::SuggestedSitesProvider(AutocompleteProviderClient* client)
    : AutocompleteProvider(AutocompleteProvider::TYPE_SEARCH) {
}

void SuggestedSitesProvider::Start(const AutocompleteInput& input,
                            bool minimal_changes) {
  matches_.clear();
  if (input.from_omnibox_focus() ||
      (input.type() == metrics::OmniboxInputType::EMPTY) ||
      (input.type() == metrics::OmniboxInputType::QUERY))
    return;

  const std::string input_text =
      base::ToLowerASCII(base::UTF16ToUTF8(input.text()));

  std::map<std::string, SuggestedSitesMatch> suggested_sites = {
    { "binance", { GURL("https://www.binance.com?ref=39346846&utm_source=brave"), GURL("https://www.binance.com"), base::ASCIIToUTF16("binance.com") }},
    { "coinbase", { GURL("https://www.coinbase.com/join/sezc_n"), GURL("https://www.coinbase.com"), base::ASCIIToUTF16("coinbase.com") }},
    { "shop.ledger", { GURL("https://shop.ledger.com/pages/ledger-nano-x?r=0ba5d7199327"), GURL("https://shop.ledger.com"), base::ASCIIToUTF16("shop.ledger.com") }},
    { "shop.trezor", { GURL("https://shop.trezor.io/product/trezor-one-metallic?offer_id=24&aff_id=3494"), GURL("https://shop.trezor.io"), base::ASCIIToUTF16("shop.trezor.io") }}
  };
 
  for (std::map<std::string, SuggestedSitesMatch>::const_iterator i =
      suggested_sites.begin(); (i != suggested_sites.end()) &&
          (matches_.size() < provider_max_matches()); ++i) {
    const std::string &current_site = i->first;
    size_t foundPos = current_site.find(input_text);
    if (std::string::npos != foundPos) {
      ACMatchClassifications styles =
          StylesForSingleMatch(input_text, current_site, foundPos);
      AddMatch(base::ASCIIToUTF16(current_site), i->second, styles);
    }
  }

  for (size_t i = 0; i < matches_.size(); ++i)
    matches_[i].relevance = kRelevance + matches_.size() - (i + 1);
  if ((matches_.size() == 1) && !matches_[0].inline_autocompletion.empty() &&
      AutocompleteMatch::AllowedToBeDefault(input, matches_[0])) {
    // If there's only one possible completion of the user's input and
    // allowing completions is okay, give the match a high enough score to
    // allow it to beat url-what-you-typed and be inlined.
    matches_[0].relevance = 1250;
    matches_[0].allowed_to_be_default_match = true;
  }
}

SuggestedSitesProvider::~SuggestedSitesProvider() {}

// static
ACMatchClassifications SuggestedSitesProvider::StylesForSingleMatch(
    const std::string &input_text,
    const std::string &site,
    const size_t &foundPos) {
  ACMatchClassifications styles;
  if (foundPos == 0) {
    styles.push_back(ACMatchClassification(
        0, ACMatchClassification::URL | ACMatchClassification::MATCH));
    if (site.length() > input_text.length()) {
      styles.push_back(ACMatchClassification(input_text.length(),
                                             ACMatchClassification::URL));
    }
  } else {
    styles.push_back(ACMatchClassification(0, ACMatchClassification::URL));
    styles.push_back(ACMatchClassification(
        foundPos, ACMatchClassification::URL | ACMatchClassification::MATCH));
    if (site.length() > foundPos + input_text.length()) {
      styles.push_back(
          ACMatchClassification(foundPos + input_text.length(), 0));
    }
  }
  return styles;
}

void SuggestedSitesProvider::AddMatch(const base::string16& match_string,
                               const SuggestedSitesMatch& data,
                               const ACMatchClassifications& styles) {
  LOG(ERROR) << "=Add suggested site match: " << match_string;
  AutocompleteMatch match(this, kRelevance, false,
                          AutocompleteMatchType::NAVSUGGEST);
  match.fill_into_edit = data.display;
  match.destination_url = data.destination_url;
  match.contents = data.display;
  match.contents_class = styles;
  match.description = base::ASCIIToUTF16("Brave suggested site");

  AutocompleteMatch::AddLastClassificationIfNecessary(
      &match.description_class, 0, ACMatchClassification::NONE);

  match.stripped_destination_url = data.stripped_destination_url;
  matches_.push_back(match);
}
