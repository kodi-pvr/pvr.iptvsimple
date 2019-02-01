#include "TimeshiftBuffer.h"

#include "../client.h"

#include "StreamReader.h"
#include "p8-platform/util/util.h"

using namespace ADDON;
using namespace iptvsimple;

TimeshiftBuffer::TimeshiftBuffer(IStreamReader *strReader,
    const std::string &timeshiftBufferPath, const unsigned int readTimeout)
  : m_strReader(strReader)
{
  m_bufferPath = timeshiftBufferPath + "/tsbuffer.ts";
  m_readTimeout = (readTimeout) ? readTimeout
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

  if (!XBMC->DeleteFile(m_bufferPath.c_str()))
    XBMC->Log(LOG_ERROR, "%s Unable to delete file when timeshift buffer is deleted: %s", __FUNCTION__, m_bufferPath.c_str());

  SAFE_DELETE(m_strReader);
  XBMC->Log(LOG_DEBUG, "%s Timeshift: Stopped", __FUNCTION__);
}

bool TimeshiftBuffer::Start()
{
  if (m_strReader == nullptr
      || m_filebufferWriteHandle == nullptr
      || m_filebufferReadHandle == nullptr)
    return false;
  if (m_running)
    return true;

  XBMC->Log(LOG_INFO, "%s Timeshift: Started", __FUNCTION__);
  m_start = time(nullptr);
  m_running = true;
  m_inputThread = std::thread([&] { DoReadWrite(); });

  return true;
}

void TimeshiftBuffer::DoReadWrite()
{
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
}

int64_t TimeshiftBuffer::Seek(long long position, int whence)
{
  XBMC->Log(LOG_DEBUG, "Timeshift: SeekForPos: %lld, whence: %d - CurrentPos: %lld, Length: %lld", position, whence, Position(), Length());
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
    XBMC->Log(LOG_DEBUG, "%s Timeshift: Read timed out; waited %d", __FUNCTION__, m_readTimeout);
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