/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "addon.h"
#include "IptvSimple.h"
#include "iptvsimple/utilities/SettingsMigration.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

ADDON_STATUS CIptvSimpleAddon::Create()
{
  /* Init settings */
  m_settings.reset(new AddonSettings());

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([this](LogLevel level, const char* message)
  {
    /* Convert the log level */
    ADDON_LOG addonLevel;

    switch (level)
    {
      case LogLevel::LEVEL_FATAL:
        addonLevel = ADDON_LOG::ADDON_LOG_FATAL;
        break;
      case LogLevel::LEVEL_ERROR:
        addonLevel = ADDON_LOG::ADDON_LOG_ERROR;
        break;
      case LogLevel::LEVEL_WARNING:
        addonLevel = ADDON_LOG::ADDON_LOG_WARNING;
        break;
      case LogLevel::LEVEL_INFO:
        addonLevel = ADDON_LOG::ADDON_LOG_INFO;
        break;
      default:
        addonLevel = ADDON_LOG::ADDON_LOG_DEBUG;
    }

    kodi::Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.iptvsimple");

  Logger::Log(LogLevel::LEVEL_INFO, "%s starting IPTV Simple PVR client...", __func__);

  return ADDON_STATUS_OK;
}

ADDON_STATUS CIptvSimpleAddon::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  return m_settings->SetSetting(settingName, settingValue);
}

ADDON_STATUS CIptvSimpleAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance, KODI_ADDON_INSTANCE_HDL& hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    IptvSimple* usedInstance = new IptvSimple(instance);
    if (!usedInstance->Initialise())
    {
      delete usedInstance;
      return ADDON_STATUS_PERMANENT_FAILURE;
    }

    // Try to migrate settings from a pre-multi-instance setup
    if (SettingsMigration::MigrateSettings(*usedInstance))
    {
      // Initial client operated on old/incomplete settings
      delete usedInstance;
      usedInstance = new IptvSimple(instance);
    }
    hdl = usedInstance;

    // Store this instance also on this class, currently support Kodi only one
    // instance, for that it becomes stored here to interact e.g. about
    // settings. In future becomes another way added.
    m_usedInstances.emplace(std::make_pair(instance.GetID(), usedInstance));
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

void CIptvSimpleAddon::DestroyInstance(const kodi::addon::IInstanceInfo& instance, const KODI_ADDON_INSTANCE_HDL hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    const auto& it = m_usedInstances.find(instance.GetID());
    if (it != m_usedInstances.end())
    {
      m_usedInstances.erase(it);
    }
  }
}

ADDONCREATOR(CIptvSimpleAddon)
