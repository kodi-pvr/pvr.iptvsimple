/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include <kodi/addon-instance/pvr/Channels.h>

namespace iptvsimple
{
  enum class CatchupMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    DEFAULT,
    APPEND,
    SHIFT,
    FLUSSONIC,
    XTREAM_CODES,
    TIMESHIFT, // Obsolete but still used by some providers, predates SHIFT
    VOD
  };

  constexpr int IGNORE_CATCHUP_DAYS = -1;

  class InstanceSettings;

  namespace data
  {
    static const std::string CHANNEL_LOGO_EXTENSION = ".png";

    class Channel
    {
    public:
      static const std::string GetCatchupModeText(const CatchupMode& catchupMode);

      Channel(std::shared_ptr<iptvsimple::InstanceSettings> settings) : m_settings(settings) {};
      Channel(const Channel &c) : m_radio(c.IsRadio()), m_uniqueId(c.GetUniqueId()),
        m_channelNumber(c.GetChannelNumber()), m_subChannelNumber(c.GetSubChannelNumber()),
        m_encryptionSystem(c.GetEncryptionSystem()), m_tvgShift(c.GetTvgShift()), m_channelName(c.GetChannelName()),
        m_iconPath(c.GetIconPath()), m_streamURL(c.GetStreamURL()), m_hasCatchup(c.HasCatchup()),
        m_catchupMode(c.GetCatchupMode()), m_catchupDays(c.GetCatchupDays()), m_catchupSource(c.GetCatchupSource()),
        m_isCatchupTSStream(c.IsCatchupTSStream()), m_catchupSupportsTimeshifting(c.CatchupSupportsTimeshifting()),
        m_catchupSourceTerminates(c.CatchupSourceTerminates()), m_catchupGranularitySeconds(c.GetCatchupGranularitySeconds()),
        m_catchupCorrectionSecs(c.GetCatchupCorrectionSecs()), m_tvgId(c.GetTvgId()), m_tvgName(c.GetTvgName()),
        m_providerUniqueId(c.GetProviderUniqueId()), m_properties(c.GetProperties()),
        m_inputStreamName(c.GetInputStreamName()), m_settings(c.m_settings) {};
      ~Channel() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      int GetChannelNumber() const { return m_channelNumber; }
      void SetChannelNumber(int value) { m_channelNumber = value; }

      int GetSubChannelNumber() const { return m_subChannelNumber; }
      void SetSubChannelNumber(int value) { m_subChannelNumber = value; }

      int GetEncryptionSystem() const { return m_encryptionSystem; }
      void SetEncryptionSystem(int value) { m_encryptionSystem = value; }

      int GetTvgShift() const { return m_tvgShift; }
      void SetTvgShift(int value) { m_tvgShift = value; }

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value) { m_channelName = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& url);

      bool IsCatchupSupported() const; // Does the M3U entry or default settings denote catchup support
      bool HasCatchup() const { return m_hasCatchup; } // Does the M3U entry denote catchup support
      void SetHasCatchup(bool value) { m_hasCatchup = value; }
      const CatchupMode& GetCatchupMode() const { return m_catchupMode; }
      void SetCatchupMode(const CatchupMode& value) { m_catchupMode = value; }

      int GetCatchupDays() const { return m_catchupDays; }
      bool IgnoreCatchupDays() const { return m_catchupDays == IGNORE_CATCHUP_DAYS; }
      void SetCatchupDays(int catchupDays);
      int GetCatchupDaysInSeconds() const { return m_catchupDays * 24 * 60 * 60; }

      const std::string& GetCatchupSource() const { return m_catchupSource; }
      void SetCatchupSource(const std::string& value) { m_catchupSource = value; }

      bool IsCatchupTSStream() const { return m_isCatchupTSStream; }
      void SetCatchupTSStream(bool value) { m_isCatchupTSStream = value; }

      bool CatchupSupportsTimeshifting() const { return m_catchupSupportsTimeshifting; }
      void SetCatchupSupportsTimeshifting(bool value) { m_catchupSupportsTimeshifting = value; }

      bool CatchupSourceTerminates() const { return m_catchupSourceTerminates; }
      void SetCatchupSourceTerminates(bool value) { m_catchupSourceTerminates = value; }

      int GetCatchupGranularitySeconds() const { return m_catchupGranularitySeconds; }
      void SetCatchupGranularitySeconds(int value) { m_catchupGranularitySeconds = value; }

      int GetCatchupCorrectionSecs() const { return m_catchupCorrectionSecs; }
      void SetCatchupCorrectionSecs(int value) { m_catchupCorrectionSecs = value; }

      const std::string& GetTvgId() const { return m_tvgId; }
      void SetTvgId(const std::string& value) { m_tvgId = value; }

      const std::string& GetTvgName() const { return m_tvgName; }
      void SetTvgName(const std::string& value) { m_tvgName = value; }

      int GetProviderUniqueId() const { return m_providerUniqueId; }
      void SetProviderUniqueId(unsigned int value) { m_providerUniqueId = value; }

      bool SupportsLiveStreamTimeshifting() const;

      const std::map<std::string, std::string>& GetProperties() const { return m_properties; }
      void SetProperties(std::map<std::string, std::string>& value) { m_properties = value; }
      void AddProperty(const std::string& prop, const std::string& value) { m_properties.insert({prop, value}); }
      std::string GetProperty(const std::string& propName) const;
      bool HasMimeType() const { return !GetProperty(PVR_STREAM_PROPERTY_MIMETYPE).empty(); }
      std::string GetMimeType() const { return GetProperty(PVR_STREAM_PROPERTY_MIMETYPE); }

      const std::string& GetInputStreamName() const { return m_inputStreamName; };
      void SetInputStreamName(const std::string& value) { m_inputStreamName = value; }

      void UpdateTo(Channel& left) const;
      void UpdateTo(kodi::addon::PVRChannel& left) const;
      void Reset();
      void SetIconPathFromTvgLogo(const std::string& tvgLogo, std::string& channelName);
      void ConfigureCatchupMode();

      bool ChannelTypeAllowsGroupsOnly() const;

    private:
      void RemoveProperty(const std::string& propName);
      void TryToAddPropertyAsHeader(const std::string& propertyName, const std::string& headerName);

      bool GenerateAppendCatchupSource(const std::string& url);
      void GenerateShiftCatchupSource(const std::string& url);
      bool GenerateFlussonicCatchupSource(const std::string& url);
      bool GenerateXtreamCodesCatchupSource(const std::string& url);

      bool m_radio = false;
      int m_uniqueId = 0;
      int m_channelNumber = 0;
      int m_subChannelNumber = 0;
      int m_encryptionSystem = 0;
      int m_tvgShift = 0;
      std::string m_channelName = "";
      std::string m_iconPath = "";
      std::string m_streamURL = "";
      bool m_hasCatchup = false;
      CatchupMode m_catchupMode = CatchupMode::DISABLED;
      int m_catchupDays = 0;
      std::string m_catchupSource = "";
      bool m_isCatchupTSStream = false;
      bool m_catchupSupportsTimeshifting = false;
      bool m_catchupSourceTerminates = false;
      int m_catchupGranularitySeconds = 1;
      int m_catchupCorrectionSecs = 0;
      std::string m_tvgId = "";
      std::string m_tvgName = "";
      int m_providerUniqueId = PVR_PROVIDER_INVALID_UID;

      std::map<std::string, std::string> m_properties;
      std::string m_inputStreamName;

      std::shared_ptr<iptvsimple::InstanceSettings> m_settings;
    };
  } //namespace data
} //namespace iptvsimple
