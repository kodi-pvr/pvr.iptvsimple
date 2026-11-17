/*
 *  Copyright (C) 2026 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "InstanceSettings.h"

#include <memory>
#include <string>

namespace iptvsimple
{
  class ATTR_DLL_LOCAL XtreamCodes
  {
  public:
    static std::string GetApiUrl(const std::shared_ptr<iptvsimple::InstanceSettings>& settings);

    // Builds an M3U playlist of the account's live streams from the player API
    static bool GetPlaylist(const std::shared_ptr<iptvsimple::InstanceSettings>& settings, std::string& playlist);
  };
} //namespace iptvsimple
