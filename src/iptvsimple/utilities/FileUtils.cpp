/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "FileUtils.h"

#include "../InstanceSettings.h"

#include <lzma.h>
#include <zlib.h>

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

std::string FileUtils::GetUserDataAddonFilePath(const std::string& userFilePath, const std::string& fileName)
{
  return PathCombine(userFilePath, fileName);
}

int FileUtils::GetFileContents(const std::string& url, std::string& content)
{
  content.clear();
  kodi::vfs::CFile file;
  if (file.OpenFile(url))
  {
    char buffer[1024];
    while (int bytesRead = file.Read(buffer, 1024))
      content.append(buffer, bytesRead);
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

bool FileUtils::XzDecompress(const std::string& compressedBytes, std::string& uncompressedBytes)
{
  if (compressedBytes.size() == 0)
  {
    uncompressedBytes = compressedBytes;
    return true;
  }

  uncompressedBytes.clear();

  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED);

  if (ret != LZMA_OK)
    return false;

  uint8_t* in_buf = (uint8_t*) compressedBytes.c_str();
  uint8_t out_buf[LZMA_OUT_BUF_MAX];
  size_t out_len;

  strm.next_in = in_buf;
  strm.avail_in = compressedBytes.size();
  do
  {
    strm.next_out = out_buf;
    strm.avail_out = LZMA_OUT_BUF_MAX;
    ret = lzma_code(&strm, LZMA_FINISH);

    out_len = LZMA_OUT_BUF_MAX - strm.avail_out;
    uncompressedBytes.append((char*) out_buf, out_len);
    out_buf[0] = 0;
  } while (strm.avail_out == 0);
  lzma_end (&strm);

  return true;
}

int FileUtils::GetCachedFileContents(std::shared_ptr<iptvsimple::InstanceSettings>& settings,
                                     const std::string& cachedName, const std::string& filePath,
                                     std::string& contents, const bool useCache /* false */)
{
  bool needReload = false;
  const std::string cachedPath = FileUtils::GetUserDataAddonFilePath(settings->GetUserPath(), cachedName);

  // check cached file is exists
  if (useCache && kodi::vfs::FileExists(cachedPath, false))
  {
    kodi::vfs::FileStatus statCached;
    kodi::vfs::FileStatus statOrig;

    kodi::vfs::StatFile(cachedPath, statCached);
    kodi::vfs::StatFile(filePath, statOrig);

    needReload = statCached.GetModificationTime() < statOrig.GetModificationTime() || statOrig.GetModificationTime() == 0;
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
      kodi::vfs::CFile file;
      if (file.OpenFileForWrite(cachedPath, true))
      {
        file.Write(contents.c_str(), contents.length());
      }
    }
    return contents.length();
  }

  return FileUtils::GetFileContents(cachedPath, contents);
}

bool FileUtils::FileExists(const std::string& file)
{
  return kodi::vfs::FileExists(file, false);
}

bool FileUtils::DeleteFile(const std::string& file)
{
  return kodi::vfs::DeleteFile(file);
}

bool FileUtils::CopyFile(const std::string& sourceFile, const std::string& targetFile)
{
  kodi::vfs::CFile file;
  bool copySuccessful = true;

  Logger::Log(LEVEL_DEBUG, "%s - Copying file: %s, to %s", __FUNCTION__, sourceFile.c_str(), targetFile.c_str());

  if (file.OpenFile(sourceFile, ADDON_READ_NO_CACHE))
  {
    const std::string fileContents = ReadFileContents(file);

    file.Close();

    if (file.OpenFileForWrite(targetFile, true))
    {
      file.Write(fileContents.c_str(), fileContents.length());
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

  kodi::vfs::CreateDirectory(targetDir);

  std::vector<kodi::vfs::CDirEntry> entries;
  if (kodi::vfs::GetDirectory(sourceDir, "", entries))
  {
    for (const auto& entry : entries)
    {
      if (entry.IsFolder() && recursiveCopy)
      {
        copySuccessful = CopyDirectory(sourceDir + "/" + entry.Label(), targetDir + "/" + entry.Label(), true);
      }
      else if (!entry.IsFolder())
      {
        copySuccessful = CopyFile(sourceDir + "/" + entry.Label(), targetDir + "/" + entry.Label());
      }
    }
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
  return kodi::addon::GetAddonPath();
}

std::string FileUtils::GetResourceDataPath()
{
  return kodi::addon::GetAddonPath("/resources/data");
}

std::string FileUtils::ReadFileContents(kodi::vfs::CFile& file)
{
  std::string fileContents;

  char buffer[1024];
  int bytesRead = 0;

  // Read until EOF or explicit error
  while ((bytesRead = file.Read(buffer, sizeof(buffer) - 1)) > 0)
    fileContents.append(buffer, bytesRead);

  return fileContents;
}
