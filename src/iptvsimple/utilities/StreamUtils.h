#pragma once

/*
 *      Copyright (C) 2005-2019 Team Kodi
 *      http://kodi.tv
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>

#include <kodi/xbmc_pvr_types.h>

namespace iptvsimple
{
  namespace utilities
  {
    enum class StreamType
      : int // same type as addon settings
    {
      HLS = 0,
      DASH,
      SMOOTH_STREAMING,
      OTHER_TYPE
    };

    class StreamUtils
    {
    public:
      static void SetStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, const std::string &name, const std::string &value);
      static const StreamType GetStreamType(const std::string& url);
      static const StreamType InspectStreamType(const std::string& url);
      static const std::string GetManifestType(const StreamType& streamType);
      static const std::string GetMimeType(const StreamType& streamType);
    };
  } // namespace utilities
} // namespace iptvsimple
