#include "StreamReader.h"

#include "../client.h"

using namespace ADDON;
using namespace iptvsimple;

bool StreamReader::Start()
{
  return true;
}

ssize_t StreamReader::ReadData(unsigned char *buffer, unsigned int size)
{
  return m_streamer->ReadStreamData(buffer, size);
}

int64_t StreamReader::Seek(long long position, int whence)
{
  return 0;
}

int64_t StreamReader::Position()
{
  return m_streamer->GetTotalBytesStreamed();
}

int64_t StreamReader::Length()
{
  return m_streamer->GetTotalBytesStreamed();
}

std::time_t StreamReader::TimeStart()
{
  return m_start;
}

std::time_t StreamReader::TimeEnd()
{
  return std::time(nullptr);
}

bool StreamReader::IsRealTime()
{
  return true;
}

bool StreamReader::IsTimeshifting()
{
  return false;
}