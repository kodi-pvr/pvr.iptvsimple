/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "AddonSettings.h"

#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/SettingsMigration.h"

#include "kodi/General.h"

using namespace iptvsimple;
using namespace iptvsimple::utilities;

AddonSettings::AddonSettings()
{
  ReadSettings();
}

void AddonSettings::ReadSettings()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + CHANNEL_GROUPS_DIR, CHANNEL_GROUPS_ADDON_DATA_BASE_DIR, true);

  // Only instance settings with this add-on!
}

ADDON_STATUS AddonSettings::SetSetting(const std::string& settingName,
                                       const kodi::addon::CSettingValue& settingValue)
{
  if (SettingsMigration::IsMigrationSetting(settingName))
  {
    // ignore settings from pre-multi-instance setup
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}
