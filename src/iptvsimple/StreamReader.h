#pragma once

#include "IStreamReader.h"

#include "M3U8Streamer.h"

namespace iptvsimple
{
  class StreamReader
    : public IStreamReader
  {
  public:
    StreamReader(M3U8Streamer *streamer) : m_streamer(streamer) {};
    bool Start() override;
    ssize_t ReadData(unsigned char *buffer, unsigned int size) override;
    int64_t Seek(long long position, int whence) override;
    int64_t Position() override;
    int64_t Length() override;
    std::time_t TimeStart() override;
    std::time_t TimeEnd() override;
    bool IsRealTime() override;
    bool IsTimeshifting() override;

  private:
    std::time_t m_start = time(nullptr);

    M3U8Streamer *m_streamer;
  };
} //namespace iptvsimple