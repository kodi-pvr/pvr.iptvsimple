#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <map>
#include <string>

#include <kodi/libXBMC_pvr.h>

namespace iptvsimple
{
  namespace data
  {
    static const std::string CHANNEL_LOGO_EXTENSION = ".png";

    class Channel
    {
    public:
      Channel() = default;
      Channel(const Channel &c) : m_radio(c.IsRadio()), m_uniqueId(c.GetUniqueId()),
        m_channelNumber(c.GetChannelNumber()), m_encryptionSystem(c.GetEncryptionSystem()),
        m_tvgShift(c.GetTvgShift()), m_channelName(c.GetChannelName()), m_iconPath(c.GetIconPath()),
        m_streamURL(c.GetStreamURL()), m_tvgId(c.GetTvgId()), m_tvgName(c.GetTvgName()),
        m_properties(c.GetProperties()) {};
      ~Channel() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      int GetChannelNumber() const { return m_channelNumber; }
      void SetChannelNumber(int value) { m_channelNumber = value; }

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

      const std::string& GetTvgId() const { return m_tvgId; }
      void SetTvgId(const std::string& value) { m_tvgId = value; }

      const std::string& GetTvgName() const { return m_tvgName; }
      void SetTvgName(const std::string& value) { m_tvgName = value; }

      const std::map<std::string, std::string>& GetProperties() const { return m_properties; }
      void SetProperties(std::map<std::string, std::string>& value) { m_properties = value; }
      void AddProperty(const std::string& prop, const std::string& value) { m_properties.insert({prop, value}); }
      std::string GetProperty(const std::string& propName) const;

      void UpdateTo(Channel& left) const;
      void UpdateTo(PVR_CHANNEL& left) const;
      void Reset();
      void SetIconPathFromTvgLogo(const std::string& tvgLogo, std::string& channelName);

    private:
      void RemoveProperty(const std::string& propName);
      void TryToAddPropertyAsHeader(const std::string& propertyName, const std::string& headerName);

      bool m_radio = false;
      int m_uniqueId = 0;
      int m_channelNumber = 0;
      int m_encryptionSystem = 0;
      int m_tvgShift = 0;
      std::string m_channelName = "";
      std::string m_iconPath = "";
      std::string m_streamURL = "";
      std::string m_tvgId = "";
      std::string m_tvgName = "";
      std::map<std::string, std::string> m_properties;
    };
  } //namespace data
} //namespace iptvsimple