#include "FileStreamReader.h"

#include "../client.h"

using namespace ADDON;
using namespace iptvsimple;

FileStreamReader::FileStreamReader(const std::string &streamUrl, const unsigned int readTimeout)
{
  m_streamHandle = XBMC->CURLCreate(streamUrl.c_str());
  if (readTimeout > 0)
    XBMC->CURLAddOption(m_streamHandle, XFILE::CURL_OPTION_PROTOCOL,
      "connection-timeout", std::to_string(readTimeout).c_str());

  XBMC->Log(LOG_DEBUG, "%s FileStreamReader: Started; url=%s", __FUNCTION__, streamUrl.c_str());
}

FileStreamReader::~FileStreamReader(void)
{
  if (m_streamHandle)
    XBMC->CloseFile(m_streamHandle);
  XBMC->Log(LOG_DEBUG, "%s FileStreamReader: Stopped", __FUNCTION__);
}

bool FileStreamReader::Start()
{
  return XBMC->CURLOpen(m_streamHandle, XFILE::READ_NO_CACHE);
}

ssize_t FileStreamReader::ReadData(unsigned char *buffer, unsigned int size)
{
  ssize_t read = XBMC->ReadFile(m_streamHandle, buffer, size);
  m_totalBytesStreamed += read;

  return read;
}

int64_t FileStreamReader::Seek(long long position, int whence)
{
  return XBMC->SeekFile(m_streamHandle, position, whence);
}

int64_t FileStreamReader::Position()
{
  return XBMC->GetFilePosition(m_streamHandle);
}

int64_t FileStreamReader::Length()
{
  return XBMC->GetFileLength(m_streamHandle);
}

std::time_t FileStreamReader::TimeStart()
{
  return m_start;
}

std::time_t FileStreamReader::TimeEnd()
{
  return std::time(nullptr);
}

bool FileStreamReader::IsRealTime()
{
  return true;
}

bool FileStreamReader::IsTimeshifting()
{
  return false;
}