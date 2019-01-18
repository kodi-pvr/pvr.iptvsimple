#pragma once

#include "IStreamReader.h"

class StreamReader
  : public IStreamReader
{
public:
  StreamReader(const std::string &streamURL,
      const unsigned int m_readTimeout);
  ~StreamReader(void);
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
};