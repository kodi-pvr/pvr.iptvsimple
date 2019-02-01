#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
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

namespace iptvsimple
{
  namespace hls
  {
    static const int MAX_RETRIES = 30;
    static const int OPEN_MAX_RETRIES = 3;

    class WebUtils
    {
    public:

      static int GetData(const std::string &url, std::string &contents, std::string &effectiveUrl, int tries, bool startOnly = false);
      static int GetDataRange(const std::string &url, std::string &dataBuffer, size_t *size, const char* range, int tries);

    private:

      static std::string ReadFileContents(const std::string &url, std::string &effectiveUrl, int *httpCode);
      static std::string ReadFileContentsStartOnly(const std::string &url, std::string &effectiveUrl, int *httpCode);
      static bool ReadFileByteRange(const std::string &url, std::string &dataBuffer, size_t *size, int *httpCode, const char* range);
      static int ParseHttpCode(std::string &statusLine);
    };
  } // namespace hls
} //namespace iptvsimple
