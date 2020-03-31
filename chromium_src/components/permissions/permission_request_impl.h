/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request.h"

#define GetIconId                 \
  GetIconId_ChromiumImpl() const; \
  IconId GetIconId

#define GetMessageTextFragment                 \
  GetMessageTextFragment_ChromiumImpl() const; \
  base::string16 GetMessageTextFragment

#include "../../../../components/permissions/permission_request_impl.h"
#undef GetMessageTextFragment
#undef GetIconId
