/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>

#include <memory>
#include <unordered_map>

#include "iptvsimple/AddonSettings.h"

class IptvSimple;

class ATTR_DLL_LOCAL CIptvSimpleAddon : public kodi::addon::CAddonBase
{
public:
  CIptvSimpleAddon() = default;

  ADDON_STATUS Create() override;
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue) override;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance, KODI_ADDON_INSTANCE_HDL& hdl) override;
  void DestroyInstance(const kodi::addon::IInstanceInfo& instance, const KODI_ADDON_INSTANCE_HDL hdl) override;

private:
  std::unordered_map<std::string, IptvSimple*> m_usedInstances;
  std::shared_ptr<iptvsimple::AddonSettings> m_settings;
};
