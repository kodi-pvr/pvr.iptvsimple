/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <p8-platform/os.h> // Exception to header ordering, some strange quirk, don't know why

#include <string>

// Prevent conflicts with Windows macros where have this names.
#ifdef _WIN32 // windows
#ifdef CreateDirectory
#undef CreateDirectory
#endif // CreateDirectory
#ifdef DeleteFile
#undef DeleteFile
#endif // DeleteFile
#endif // _WIN32

namespace iptvsimple
{
  namespace utilities
  {
    class FileUtils
    {
    public:
      static std::string PathCombine(const std::string& path, const std::string& fileName);
      static std::string GetUserDataAddonFilePath(const std::string& fileName);
      static int GetFileContents(const std::string& url, std::string& content);
      static bool GzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes);
      static int GetCachedFileContents(const std::string& cachedName, const std::string& filePath,
                                       std::string& content, const bool useCache = false);
      static bool FileExists(const std::string& file);
      static bool DeleteFile(const std::string& file);
      static bool CopyFile(const std::string& sourceFile, const std::string& targetFile);
      static bool CopyDirectory(const std::string& sourceDir, const std::string& targetDir, bool recursiveCopy);
      static std::string GetSystemAddonPath();
      static std::string GetResourceDataPath();

    private:
      static std::string ReadFileContents(void* fileHandle);
    };
  } // namespace utilities
} // namespace iptvsimple
