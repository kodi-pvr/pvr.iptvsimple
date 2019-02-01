/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#pragma once

#include <memory>
#include <string>

#include "EncAes128.h"

namespace iptvsimple
{
  namespace hls
  {
    struct ByteBuffer
    {
      uint8_t *data;
      int len;
      int pos;
    };
  } //namespace hls
} //namespace iptvsimple