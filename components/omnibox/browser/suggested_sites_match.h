/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_MATCH_H_
#define COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_MATCH_H_

#include "base/strings/string16.h"
#include "url/gurl.h"

// This is the provider for Brave Suggested Sites
struct SuggestedSitesMatch {
  GURL destination_url;
  GURL stripped_destination_url;
  base::string16 display;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_MATCH_H_
