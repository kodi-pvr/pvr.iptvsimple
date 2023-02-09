/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace kodi
{
namespace addon
{
class IAddonInstance;
}
} // namespace kodi

namespace iptvsimple
{
namespace utilities
{
class SettingsMigration
{
public:
  static bool MigrateSettings(kodi::addon::IAddonInstance& target);
  static bool IsMigrationSetting(const std::string& key);

private:
  SettingsMigration() = delete;
  explicit SettingsMigration(kodi::addon::IAddonInstance& target) : m_target(target) {}

  void MigrateStringSetting(const char* key, const std::string& defaultValue);
  void MigrateIntSetting(const char* key, int defaultValue);
  void MigrateFloatSetting(const char* key, float defaultValue);
  void MigrateBoolSetting(const char* key, bool defaultValue);

  bool Changed() const { return m_changed; }

  kodi::addon::IAddonInstance& m_target;
  bool m_changed{false};
};

} // namespace utilities
} // namespace iptvsimple
