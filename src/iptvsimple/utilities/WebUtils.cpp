/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "WebUtils.h"

#include "../../client.h"

#include <cctype>
#include <iomanip>
#include <sstream>

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::utilities;

// http://stackoverflow.com/a/17708801
const std::string WebUtils::UrlEncode(const std::string& value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (auto c : value)
  {
    // Keep alphanumeric and other accepted characters intact
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
  }

  return escaped.str();
}

std::string WebUtils::ReadFileContentsStartOnly(const std::string& url, int* httpCode)
{
  std::string strContent;
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0x08); //READ_NO_CACHE
  if (fileHandle)
  {
    char buffer[1024];
    if (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strContent.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }

  if (strContent.empty())
    *httpCode = 500;
  else
    *httpCode = 200;

  return strContent;
}

bool WebUtils::IsHttpUrl(const std::string& url)
{
  return StringUtils::StartsWith(url, HTTP_PREFIX) || StringUtils::StartsWith(url, HTTPS_PREFIX);
}
