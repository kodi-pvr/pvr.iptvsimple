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

#include "StreamUtils.h"

#include "WebUtils.h"

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::utilities;

void StreamUtils::SetStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, const std::string &name, const std::string &value)
{
  strncpy(properties[*propertiesCount].strName, name.c_str(), sizeof(properties[*propertiesCount].strName) - 1);
  strncpy(properties[*propertiesCount].strValue, value.c_str(), sizeof(properties[*propertiesCount].strValue) - 1);
  (*propertiesCount)++;
}

const StreamType StreamUtils::GetStreamType(const std::string& url)
{
  if (url.find(".m3u8") != std::string::npos)
    return StreamType::HLS;

  return StreamType::OTHER_TYPE;
}

const StreamType StreamUtils::InspectStreamType(const std::string& url)
{
  int httpCode = 0;
  const std::string source = WebUtils::ReadFileContentsStartOnly(url, &httpCode);

  if (httpCode == 200)
  {
    if (StringUtils::StartsWith(source, "#EXTM3U") && source.find("#EXT-X-STREAM-INF") != std::string::npos)
      return StreamType::HLS;
  }

  return StreamType::OTHER_TYPE;
}
