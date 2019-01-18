#pragma once

#ifndef PVR_IPTVSIMPLE_ISTREAMREADER_H
#define PVR_IPTVSIMPLE_ISTREAMREADER_H

#include "libXBMC_addon.h"

#include <ctime>

enum class Timeshift
  : int // same type as addon settings
{
  OFF = 0,
  ON_PLAYBACK = 1,
  ON_PAUSE = 2
};

class IStreamReader
{
public:
  virtual ~IStreamReader(void) = default;
  virtual bool Start() = 0;
  virtual ssize_t ReadData(unsigned char *buffer, unsigned int size) = 0;
  virtual int64_t Seek(long long position, int whence) = 0;
  virtual int64_t Position() = 0;
  virtual int64_t Length() = 0;
  virtual std::time_t TimeStart() = 0;
  virtual std::time_t TimeEnd() = 0;
  virtual bool IsRealTime() = 0;
  virtual bool IsTimeshifting() = 0;
};

#endif