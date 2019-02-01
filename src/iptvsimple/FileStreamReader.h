#pragma once

#include "IStreamReader.h"

#include "M3U8Streamer.h"

namespace iptvsimple
{
  class FileStreamReader
    : public IStreamReader
  {
  public:
    FileStreamReader(const std::string &streamUrl, const unsigned int m_readTimeout);
    ~FileStreamReader(void);
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
    void *m_streamHandle;
    std::time_t m_start = time(nullptr);

    long long m_totalBytesStreamed = 0;
  };
} //namespace iptvsimple