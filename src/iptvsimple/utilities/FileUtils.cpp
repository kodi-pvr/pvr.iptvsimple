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

#include "FileUtils.h"

#include "../Settings.h"
#include "../../client.h"
#include "zlib.h"

#include <kodi/kodi_vfs_types.h>

using namespace iptvsimple;
using namespace iptvsimple::utilities;

std::string FileUtils::PathCombine(const std::string& path, const std::string& fileName)
{
  std::string result = path;

  if (!result.empty())
  {
    if (result.at(result.size() - 1) == '\\' ||
        result.at(result.size() - 1) == '/')
    {
      result.append(fileName);
    }
    else
    {
      result.append("/");
      result.append(fileName);
    }
  }
  else
  {
    result.append(fileName);
  }

  return result;
}

std::string FileUtils::GetUserDataAddonFilePath(const std::string& fileName)
{
  return PathCombine(Settings::GetInstance().GetUserPath(), fileName);
}

int FileUtils::GetFileContents(const std::string& url, std::string& content)
{
  content.clear();
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      content.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }

  return content.length();
}

/*
 * This method uses zlib to decompress a gzipped file in memory.
 * Author: Andrew Lim Chong Liang
 * http://windrealm.org
 */

bool FileUtils::GzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes)
{
  if (compressedBytes.size() == 0)
  {
    uncompressedBytes = compressedBytes;
    return true;
  }

  uncompressedBytes.clear();

  unsigned uncompLength = compressedBytes.size();
  const unsigned half_length = compressedBytes.size() / 2;

  char* uncomp = static_cast<char*>(calloc(sizeof(char), uncompLength));

  z_stream strm;
  strm.next_in = (Bytef*)compressedBytes.c_str();
  strm.avail_in = compressedBytes.size();
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  int status = inflateInit2(&strm, 16 + MAX_WBITS);
  if (status != Z_OK)
  {
    free(uncomp);
    return false;
  }

  bool done = false;
  while (!done)
  {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength)
    {
      // Increase size of output buffer
      uncomp = static_cast<char*>(realloc(uncomp, uncompLength + half_length));
      if (!uncomp)
        return false;
      uncompLength += half_length;
    }

    strm.next_out = reinterpret_cast<Bytef*>(uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate(&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK)
      break;
  }

  status = inflateEnd(&strm);
  if (status != Z_OK)
  {
    free(uncomp);
    return false;
  }

  for (size_t i = 0; i < strm.total_out; ++i)
    uncompressedBytes += uncomp[i];

  free(uncomp);
  return true;
}

int FileUtils::GetCachedFileContents(const std::string& cachedName, const std::string& filePath,
                                       std::string& contents, const bool useCache /* false */)
{
  bool needReload = false;
  const std::string cachedPath = FileUtils::GetUserDataAddonFilePath(cachedName);

  // check cached file is exists
  if (useCache && XBMC->FileExists(cachedPath.c_str(), false))
  {
    struct __stat64 statCached;
    struct __stat64 statOrig;

    XBMC->StatFile(cachedPath.c_str(), &statCached);
    XBMC->StatFile(filePath.c_str(), &statOrig);

    needReload = statCached.st_mtime < statOrig.st_mtime || statOrig.st_mtime == 0;
  }
  else
  {
    needReload = true;
  }

  if (needReload)
  {
    FileUtils::GetFileContents(filePath, contents);

    // write to cache
    if (useCache && contents.length() > 0)
    {
      void* fileHandle = XBMC->OpenFileForWrite(cachedPath.c_str(), true);
      if (fileHandle)
      {
        XBMC->WriteFile(fileHandle, contents.c_str(), contents.length());
        XBMC->CloseFile(fileHandle);
      }
    }
    return contents.length();
  }

  return FileUtils::GetFileContents(cachedPath, contents);
}

bool FileUtils::FileExists(const std::string& file)
{
  return XBMC->FileExists(file.c_str(), false);
}

bool FileUtils::DeleteFile(const std::string& file)
{
  return XBMC->DeleteFile(file.c_str());
}

bool FileUtils::CopyFile(const std::string& sourceFile, const std::string& targetFile)
{
  bool copySuccessful = true;

  Logger::Log(LEVEL_DEBUG, "%s - Copying file: %s, to %s", __FUNCTION__, sourceFile.c_str(), targetFile.c_str());

  void* sourceFileHandle = XBMC->OpenFile(sourceFile.c_str(), 0x08); //READ_NO_CACHE

  if (sourceFileHandle)
  {
    const std::string fileContents = ReadFileContents(sourceFileHandle);

    XBMC->CloseFile(sourceFileHandle);

    void* targetFileHandle = XBMC->OpenFileForWrite(targetFile.c_str(), true);

    if (targetFileHandle)
    {
      XBMC->WriteFile(targetFileHandle, fileContents.c_str(), fileContents.length());
      XBMC->CloseFile(targetFileHandle);
    }
    else
    {
      Logger::Log(LEVEL_ERROR, "%s - Could not open target file to copy to: %s", __FUNCTION__, targetFile.c_str());
      copySuccessful = false;
    }
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s - Could not open source file to copy: %s", __FUNCTION__, sourceFile.c_str());
    copySuccessful = false;
  }

  return copySuccessful;
}

bool FileUtils::CopyDirectory(const std::string& sourceDir, const std::string& targetDir, bool recursiveCopy)
{
  bool copySuccessful = true;

  XBMC->CreateDirectory(targetDir.c_str());

  VFSDirEntry* entries;
  unsigned int numEntries;

  if (XBMC->GetDirectory(sourceDir.c_str(), "", &entries, &numEntries))
  {
    for (int i = 0; i < numEntries; i++)
    {
      if (entries[i].folder && recursiveCopy)
      {
        copySuccessful = CopyDirectory(sourceDir + "/" + entries[i].label, targetDir + "/" + entries[i].label, true);
      }
      else if (!entries[i].folder)
      {
        copySuccessful = CopyFile(sourceDir + "/" + entries[i].label, targetDir + "/" + entries[i].label);
      }
    }

    XBMC->FreeDirectory(entries, numEntries);
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s - Could not copy directory: %s, to directory: %s", __FUNCTION__, sourceDir.c_str(), targetDir.c_str());
    copySuccessful = false;
  }
  return copySuccessful;
}

std::string FileUtils::GetSystemAddonPath()
{
  char path[1024];
  XBMC->GetSetting("__addonpath__", path);

  return path;
}

std::string FileUtils::GetResourceDataPath()
{
  std::string resourcesDataPath = GetSystemAddonPath();
  resourcesDataPath += "/resources/data";

  return resourcesDataPath;
}

std::string FileUtils::ReadFileContents(void* fileHandle)
{
  std::string fileContents;

  char buffer[1024];
  int bytesRead = 0;

  // Read until EOF or explicit error
  while ((bytesRead = XBMC->ReadFile(fileHandle, buffer, sizeof(buffer) - 1)) > 0)
    fileContents.append(buffer, bytesRead);

  return fileContents;
}
