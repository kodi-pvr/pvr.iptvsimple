/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Providers.h"

#include "../PVRIptvData.h"
#include "utilities/Logger.h"
#include "utilities/FileUtils.h"
#include "utilities/XMLUtils.h"

#include <kodi/tools/StringUtils.h>
#include <pugixml.hpp>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace kodi::tools;
using namespace pugi;

Providers::Providers()
{
}

bool Providers::Init()
{
  Clear();

  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + PROVIDER_DIR, PROVIDER_ADDON_DATA_BASE_DIR, true);

  std::string providerMappingsFile = Settings::GetInstance().GetProviderNameMapFile();
  if (LoadProviderMappingFile(providerMappingsFile))
    Logger::Log(LEVEL_INFO, "%s - Loaded '%d' providers mappings", __func__, m_providerMappingsMap.size());
  else
    Logger::Log(LEVEL_ERROR, "%s - could not load provider mappings XML file: %s", __func__, providerMappingsFile.c_str());

  return true;
}

void Providers::GetProviders(std::vector<kodi::addon::PVRProvider>& kodiProviders) const
{
  for (const auto& provider : m_providers)
  {
    kodi::addon::PVRProvider kodiProvider;

    provider->UpdateTo(kodiProvider);

    Logger::Log(LEVEL_DEBUG, "%s - Transfer provider '%s', unique id '%d'", __func__,
                provider->GetProviderName().c_str(), provider->GetUniqueId());

    kodiProviders.emplace_back(kodiProvider);
  }
}

int Providers::GetProviderUniqueId(const std::string& providerName)
{
  std::shared_ptr<Provider> provider = GetProvider(providerName);
  int uniqueId = PVR_PROVIDER_INVALID_UID;

  if (provider)
    uniqueId = provider->GetUniqueId();

  return uniqueId;
}

std::shared_ptr<Provider> Providers::GetProvider(int uniqueId)
{
  auto providerPair = m_providersUniqueIdMap.find(uniqueId);
  if (providerPair != m_providersUniqueIdMap.end())
    return providerPair->second;

  return {};
}

std::shared_ptr<Provider> Providers::GetProvider(const std::string& providerName)
{
  auto providerPair = m_providersNameMap.find(providerName);
  if (providerPair != m_providersNameMap.end())
    return providerPair->second;

  return {};
}

bool Providers::IsValid(int uniqueId)
{
  return GetProvider(uniqueId) != nullptr;
}

bool Providers::IsValid(const std::string &providerName)
{
  return GetProvider(providerName) != nullptr;
}

int Providers::GetNumProviders() const
{
  return m_providers.size();
}

void Providers::Clear()
{
  m_providers.clear();
  m_providersUniqueIdMap.clear();
  m_providersNameMap.clear();
}

std::shared_ptr<Provider> Providers::AddProvider(const std::string& providerName)
{
  if (!providerName.empty())
  {
    Provider provider;
    std::string providerKey = providerName;
    StringUtils::ToLower(providerKey);

    auto providerPair = m_providerMappingsMap.find(providerKey);
    if (providerPair != m_providerMappingsMap.end())
    {
      provider = providerPair->second;
    }
    else
    {
      provider.SetProviderName(providerName);
    }

    return AddProvider(provider);
  }
  return {};
}

namespace
{

int GenerateProviderUniqueId(const std::string& providerName)
{
  std::string concat(providerName);

  const char* calcString = concat.c_str();
  int uniqueId = 0;
  int c;
  while ((c = *calcString++))
    uniqueId = ((uniqueId << 5) + uniqueId) + c; /* iId * 33 + c */

  return abs(uniqueId);
}

} // unnamed namespace

std::shared_ptr<Provider> Providers::AddProvider(Provider& newProvider)
{
  std::shared_ptr<Provider> foundProvider = GetProvider(newProvider.GetProviderName());

  if (!foundProvider)
  {
    newProvider.SetUniqueId(GenerateProviderUniqueId(newProvider.GetProviderName()));

    m_providers.emplace_back(new Provider(newProvider));

    std::shared_ptr<Provider> provider = m_providers.back();

    m_providersUniqueIdMap.insert({provider->GetUniqueId(), provider});
    m_providersNameMap.insert({provider->GetProviderName(), provider});

    return provider;
  }

  return foundProvider;
}

std::vector<std::shared_ptr<Provider>>& Providers::GetProvidersList()
{
  return m_providers;
}

bool Providers::LoadProviderMappingFile(const std::string& xmlFile)
{
  m_providerMappingsMap.clear();

  if (!FileUtils::FileExists(xmlFile.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s No XML file found: %s", __func__, xmlFile.c_str());
    return false;
  }

  Logger::Log(LEVEL_DEBUG, "%s Loading XML File: %s", __func__, xmlFile.c_str());

  std::string fileContents;
  FileUtils::GetFileContents(xmlFile, fileContents);

  if (fileContents.empty())
  {
    Logger::Log(LEVEL_ERROR, "%s No Content in XML file: %s", __func__, xmlFile.c_str());
    return false;
  }

  char* buffer = &(fileContents[0]);
  xml_document xmlDoc;
  xml_parse_result result = xmlDoc.load_string(buffer);

  if (!result)
  {
    std::string errorString;
    int offset = GetParseErrorString(buffer, result.offset, errorString);
    Logger::Log(LEVEL_ERROR, "%s - Unable parse EPG XML: %s, offset: %d: \n[ %s \n]", __FUNCTION__, result.description(), offset, errorString.c_str());
    return false;
  }

  const auto& rootElement = xmlDoc.child("providerMappings");
  if (!rootElement)
  {
    Logger::Log(LEVEL_ERROR, "%s - Invalid EPG XML: no <providerMappings> tag found", __FUNCTION__);
    return false;
  }

  for (const auto& mappingNode : rootElement.children("providerMapping"))
  {
    std::string mappedName;

    if (!GetAttributeValue(mappingNode, "mappedName", mappedName) || mappedName.empty())
      continue;

    StringUtils::ToLower(mappedName);
    Provider provider;

    std::string name = GetNodeValue(mappingNode, "name");
    if (name.empty())
    {
      Logger::Log(LEVEL_ERROR, "%s Could not read <name> element for provider mapping: %s", __func__, mappedName.c_str());
      return false;
    }
    provider.SetProviderName(name);

    std::string type = GetNodeValue(mappingNode, "type");
    if (!type.empty())
    {
      StringUtils::ToLower(type);
      if (type == "addon")
        provider.SetProviderType(PVR_PROVIDER_TYPE_ADDON);
      else if (type == "satellite")
        provider.SetProviderType(PVR_PROVIDER_TYPE_SATELLITE);
      else if (type == "cable")
        provider.SetProviderType(PVR_PROVIDER_TYPE_CABLE);
      else if (type == "aerial")
        provider.SetProviderType(PVR_PROVIDER_TYPE_AERIAL);
      else if (type == "iptv")
        provider.SetProviderType(PVR_PROVIDER_TYPE_IPTV);
      else
        provider.SetProviderType(PVR_PROVIDER_TYPE_UNKNOWN);
    }

    std::string iconPath = GetNodeValue(mappingNode, "iconPath");
    if (!iconPath.empty())
      provider.SetIconPath(iconPath);

    std::string countriesString = GetNodeValue(mappingNode, "countries");
    if (!countriesString.empty())
    {
      std::vector<std::string> countries = StringUtils::Split(countriesString, PROVIDER_STRING_TOKEN_SEPARATOR);
      provider.SetCountries(countries);
    }

    std::string languagesString = GetNodeValue(mappingNode, "languages");
    if (!languagesString.empty())
    {
      std::vector<std::string> languages = StringUtils::Split(languagesString, PROVIDER_STRING_TOKEN_SEPARATOR);
      provider.SetLanguages(languages);
    }

    m_providerMappingsMap.insert({mappedName, provider});

    Logger::Log(LEVEL_DEBUG, "%s Read Provider Mapping from: %s to %s", __func__, mappedName.c_str(), provider.GetProviderName().c_str());
  }

  return true;
}