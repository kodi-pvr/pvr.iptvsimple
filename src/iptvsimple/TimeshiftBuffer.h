#pragma once

#include "IStreamReader.h"

#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>

namespace iptvsimple
{
  class TimeshiftBuffer
    : public IStreamReader
  {
  public:
    TimeshiftBuffer(IStreamReader *strReader,
        const std::string &timeshiftBufferPath, const unsigned int readTimeout);
    ~TimeshiftBuffer(void);
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
    static const int BUFFER_SIZE = 32 * 1024;
    static const int DEFAULT_READ_TIMEOUT = 30;
    static const int READ_WAITTIME = 50;

    void DoReadWrite();

    std::string m_bufferPath;
    IStreamReader *m_strReader;
    void *m_filebufferReadHandle;
    void *m_filebufferWriteHandle;
    int m_readTimeout;
    std::time_t m_start = 0;
    std::atomic<uint64_t> m_writePos = { 0 };

    std::atomic<bool> m_running = { false };
    std::thread m_inputThread;
    std::condition_variable m_condition;
    std::mutex m_mutex;
  };
} //namespace iptvsimple