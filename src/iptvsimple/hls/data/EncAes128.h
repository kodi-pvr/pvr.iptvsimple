/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#pragma once

#include <string>

namespace iptvsimple
{
  namespace hls
  {
    static const int KEYLEN = 16;
    static const int KEYLEN_HEX_CHAR = (KEYLEN * 2) + 2;

    enum class EncryptionType : int
    {
      ENC_NONE = 0x00,
      ENC_AES128 = 0x01,
      ENC_AES_SAMPLE = 0x02
    };

    struct EncAes128
    {
      bool m_ivIsStatic;
      uint8_t m_ivValue[KEYLEN];
      uint8_t m_keyValue[KEYLEN];
      std::string m_keyUrl;
    };
  } //namespace hls
} //namespace iptvsimple