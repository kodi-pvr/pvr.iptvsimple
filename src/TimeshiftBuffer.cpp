#include "TimeshiftBuffer.h"
#include "StreamReader.h"
#include "client.h"

#include "p8-platform/util/util.h"

#define BUFFER_SIZE 32 * 1024
#define DEFAULT_READ_TIMEOUT 60
#define READ_WAITTIME 50

using namespace ADDON;

TimeshiftBuffer::TimeshiftBuffer(IStreamReader *strReader,
    const std::string &m_timeshiftBufferPath, const unsigned int m_readTimeoutX)
  : m_strReader(strReader)
{
  m_bufferPath = m_timeshiftBufferPath + "/tsbuffer.ts";
  m_readTimeout = (m_readTimeoutX) ? m_readTimeout
      : DEFAULT_READ_TIMEOUT;

  m_filebufferWriteHandle = XBMC->OpenFileForWrite(m_bufferPath.c_str(), true);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_filebufferReadHandle = XBMC->OpenFile(m_bufferPath.c_str(), XFILE::READ_NO_CACHE);
}

TimeshiftBuffer::~TimeshiftBuffer(void)
{
  m_running = false;
  if (m_inputThread.joinable())
    m_inputThread.join();

  if (m_filebufferWriteHandle)
  {
    // XBMC->TruncateFile doesn't work for unknown reasons
    XBMC->CloseFile(m_filebufferWriteHandle);
    void *tmp;
    if ((tmp = XBMC->OpenFileForWrite(m_bufferPath.c_str(), true)) != nullptr)
      XBMC->CloseFile(tmp);
  }
  if (m_filebufferReadHandle)
    XBMC->CloseFile(m_filebufferReadHandle);
  SAFE_DELETE(m_strReader);
  XBMC->Log(LOG_DEBUG, "Timeshift: Stopped");
}

bool TimeshiftBuffer::Start()
{
  if (m_strReader == nullptr
      || m_filebufferWriteHandle == nullptr
      || m_filebufferReadHandle == nullptr)
    return false;
  if (m_running)
    return true;

  XBMC->Log(LOG_INFO, "Timeshift: Started");
  m_start = time(nullptr);
  m_running = true;
  m_inputThread = std::thread([&] { DoReadWrite(); });

  return true;
}

void TimeshiftBuffer::DoReadWrite()
{
  XBMC->Log(LOG_DEBUG, "Timeshift: Thread started");
  uint8_t buffer[BUFFER_SIZE];

  m_strReader->Start();
  while (m_running)
  {
    ssize_t read = m_strReader->ReadData(buffer, sizeof(buffer));

    // don't handle any errors here, assume write fully succeeds
    ssize_t write = XBMC->WriteFile(m_filebufferWriteHandle, buffer, read);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_writePos += write;

    m_condition.notify_one();
  }
  XBMC->Log(LOG_DEBUG, "Timeshift: Thread stopped");
  return;
}

int64_t TimeshiftBuffer::Seek(long long position, int whence)
{
  return XBMC->SeekFile(m_filebufferReadHandle, position, whence);
}

int64_t TimeshiftBuffer::Position()
{
  return XBMC->GetFilePosition(m_filebufferReadHandle);
}

int64_t TimeshiftBuffer::Length()
{
  return m_writePos;
}

ssize_t TimeshiftBuffer::ReadData(unsigned char *buffer, unsigned int size)
{
  int64_t requiredLength = Position() + size;

  /* make sure we never read above the current write position */
  std::unique_lock<std::mutex> lock(m_mutex);
  bool available = m_condition.wait_for(lock, std::chrono::seconds(m_readTimeout),
    [&] { return Length() >= requiredLength; });

  if (!available)
  {
    XBMC->Log(LOG_DEBUG, "Timeshift: Read timed out; waited %d", m_readTimeout);
    return -1;
  }

  return XBMC->ReadFile(m_filebufferReadHandle, buffer, size);
}

std::time_t TimeshiftBuffer::TimeStart()
{
  return m_start;
}

std::time_t TimeshiftBuffer::TimeEnd()
{
  return std::time(nullptr);
}

bool TimeshiftBuffer::IsRealTime()
{
  // other PVRs use 10 seconds here, but we aren't doing any demuxing
  // we'll therefore just asume 1 secs needs about 1mb
  return Length() - Position() <= 10 * 1048576;
}

bool TimeshiftBuffer::IsTimeshifting()
{
  return true;
}