/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/Filesystem.h>
#include <memory>
#include <string>

namespace iptvsimple
{
  class InstanceSettings;

  namespace utilities
  {
    static const int LZMA_OUT_BUF_MAX = 409600;

    class FileUtils
    {
    public:
      static std::string PathCombine(const std::string& path, const std::string& fileName);
      static std::string GetUserDataAddonFilePath(const std::string& userFilePath, const std::string& fileName);
      static int GetFileContents(const std::string& url, std::string& content);
      static bool GzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes);
      static bool XzDecompress(const std::string& compressedBytes, std::string& uncompressedBytes);
      static int GetCachedFileContents(std::shared_ptr<iptvsimple::InstanceSettings>& settings,
                                       const std::string& cachedName, const std::string& filePath,
                                       std::string& content, const bool useCache = false);
      static bool FileExists(const std::string& file);
      static bool DeleteFile(const std::string& file);
      static bool CopyFile(const std::string& sourceFile, const std::string& targetFile);
      static bool CopyDirectory(const std::string& sourceDir, const std::string& targetDir, bool recursiveCopy);
      static std::string GetSystemAddonPath();
      static std::string GetResourceDataPath();

    private:
      static std::string ReadFileContents(kodi::vfs::CFile& fileHandle);
    };
  } // namespace utilities
} // namespace iptvsimple
