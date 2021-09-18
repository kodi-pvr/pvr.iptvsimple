/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace iptvsimple
{
  enum class StreamType
    : int // same type as addon settings
  {
    HLS = 0,
    DASH,
    SMOOTH_STREAMING,
    TS,
    PLUGIN,
    MIME_TYPE_UNRECOGNISED,
    OTHER_TYPE
  };

  namespace data
  {
    class StreamEntry
    {
    public:
      const std::string& GetStreamKey() const { return m_streamKey; }
      void SetStreamKey(const std::string& value) { m_streamKey = value; }

      const StreamType& GetStreamType() const { return m_streamType; }
      void SetStreamType(const StreamType& value) { m_streamType = value; }

      const std::string& GetMimeType() const { return m_mimeType; }
      void SetMimeType(const std::string& value) { m_mimeType = value; }

      time_t GetLastAccessTime() const { return m_lastAcessTime; }
      void SetLastAccessTime(time_t value) { m_lastAcessTime = value; }

    private:
      std::string m_streamKey; // URL or catchup source
      StreamType m_streamType;
      std::string m_mimeType;
      time_t m_lastAcessTime;
    };
  } //namespace data
} //namespace iptvsimple
