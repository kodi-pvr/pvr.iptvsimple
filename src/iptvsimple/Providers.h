/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Provider.h"

#include "Settings.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <kodi/addon-instance/pvr/Providers.h>

namespace iptvsimple
{
  static const std::string PROVIDER_DIR = "/providers";
  static const std::string PROVIDER_ADDON_DATA_BASE_DIR = ADDON_DATA_BASE_DIR + PROVIDER_DIR;

  class ATTRIBUTE_HIDDEN Providers
  {
  public:
    Providers();

    bool Init();

    void GetProviders(std::vector<kodi::addon::PVRProvider>& kodiProviders) const;

    int GetProviderUniqueId(const std::string& providerName);
    std::shared_ptr<iptvsimple::data::Provider> GetProvider(int uniqueId);
    std::shared_ptr<iptvsimple::data::Provider> GetProvider(const std::string& providerName);
    bool IsValid(int uniqueId);
    bool IsValid(const std::string& providerName);
    int GetNumProviders() const;
    void Clear();
    std::vector<std::shared_ptr<iptvsimple::data::Provider>>& GetProvidersList();

    std::shared_ptr<iptvsimple::data::Provider> AddProvider(const std::string& providerName);

  private:
    std::shared_ptr<iptvsimple::data::Provider> AddProvider(iptvsimple::data::Provider& provider);
    bool LoadProviderMappingFile(const std::string& xmlFile);

    std::vector<std::shared_ptr<iptvsimple::data::Provider>> m_providers;
    std::unordered_map<int, std::shared_ptr<iptvsimple::data::Provider>> m_providersUniqueIdMap;
    std::unordered_map<std::string, std::shared_ptr<iptvsimple::data::Provider>> m_providersNameMap;

    std::unordered_map<std::string, iptvsimple::data::Provider> m_providerMappingsMap;
  };
} //namespace iptvsimple
