#include "WebUtils.h"

#include <thread>

#include "../../../client.h"

#include "p8-platform/util/StringUtils.h"

using namespace ADDON;
using namespace iptvsimple::hls;

int WebUtils::GetData(const std::string &url, std::string &contents, std::string &effectiveUrl, int tries, bool startOnly)
{
  int httpCode;
  size_t size = 0;
  int tryCount = 0;

  while (tryCount < tries)
  {
    if (startOnly)
      contents = ReadFileContentsStartOnly(url, effectiveUrl, &httpCode);
    else
      contents = ReadFileContents(url, effectiveUrl, &httpCode);

    if (contents.empty())
    {
      tryCount++;
      XBMC->Log(LOG_DEBUG, "%s Try %d of %d failed", __FUNCTION__, tryCount, tries);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      continue;
    }
    break;
  }

  if (contents.empty()) {
    XBMC->Log(LOG_ERROR, "%s After %d tries, could not read file: %s", __FUNCTION__, tries, url.c_str());
    return false;
  }

  return httpCode;
}

std::string WebUtils::ReadFileContents(const std::string &url, std::string &effectiveURL, int *httpCode)
{
  std::string strContent;
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0x08); //READ_NO_CACHE
  if (fileHandle)
  {
    char* newURL = XBMC->GetFilePropertyValue(fileHandle, XFILE::FILE_PROPERTY_EFFECTIVE_URL, "");
    effectiveURL = newURL;
    XBMC->FreeString(newURL);

    char* responseLine = XBMC->GetFilePropertyValue(fileHandle, XFILE::FILE_PROPERTY_RESPONSE_PROTOCOL, "");
    std::string statusLine = responseLine;
    XBMC->FreeString(responseLine);
    *httpCode = ParseHttpCode(statusLine);

    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strContent.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }

  return strContent;
}

std::string WebUtils::ReadFileContentsStartOnly(const std::string &url, std::string &effectiveURL, int *httpCode)
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
    *httpCode = 1;
  else
    *httpCode = 200;

  return strContent;
}

int WebUtils::GetDataRange(const std::string &url, std::string &dataBuffer, size_t *size, const char* range, int tries)
{
  int httpCode;
  int tryCount = 0;

  while (tryCount < tries)
  {
    if (!ReadFileByteRange(url, dataBuffer, size, &httpCode, range))
    {
      tryCount++;
      XBMC->Log(LOG_DEBUG, "%s Try %d of %d failed", __FUNCTION__, tryCount, tries);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      continue;
    }
    break;
  }

  if (dataBuffer.empty())
  {
    XBMC->Log(LOG_ERROR, "%s After %d tries, could not read file: %s", __FUNCTION__, tries, url.c_str());
    return false;
  }

  return httpCode;
}

bool WebUtils::ReadFileByteRange(const std::string &url, std::string &dataBuffer, size_t *size, int *httpCode, const char* range)
{
  void *streamHandle = XBMC->CURLCreate(url.c_str());

  if (range != nullptr)
  {
    XBMC->CURLAddOption(streamHandle, XFILE::CURL_OPTION_OPTION, "CURLOPT_RANGE", range);
  }

  if (XBMC->CURLOpen(streamHandle, XFILE::READ_NO_CACHE))
  {
    char buffer[16384];
    while (int bytesRead = XBMC->ReadFile(streamHandle, buffer, 16384))
    {
      dataBuffer.append(buffer, bytesRead);
      *size += bytesRead;
    }

    char* responseLine = XBMC->GetFilePropertyValue(streamHandle, XFILE::FILE_PROPERTY_RESPONSE_PROTOCOL, "");
    std::string statusLine = responseLine;
    XBMC->FreeString(responseLine);
    *httpCode = ParseHttpCode(statusLine);

    XBMC->CloseFile(streamHandle);
  }

  return true;
}

int WebUtils::ParseHttpCode(std::string &statusLine)
{
  int httpCode;
  size_t found = statusLine.find(' ');
  if (found != std::string::npos)
  {
    statusLine = statusLine.substr(found, statusLine.size());
    statusLine = StringUtils::TrimLeft(statusLine);

    found = statusLine.find(' ');
    if (found != std::string::npos)
    {
      statusLine = statusLine.substr(0, found);
      std::sscanf(statusLine.c_str(), "%d",  &httpCode);
    }
  }

  return httpCode;
}