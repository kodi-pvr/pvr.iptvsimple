#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>

#include "../data/ByteBuffer.h"
#include "../data/EncAes128.h"
#include "../data/HlsMediaSegment.h"
#include "../mpegts/mpegts.h"

namespace iptvsimple
{
  namespace hls
  {
    class AesUtils
    {
    public:

      static bool DecryptAes128(std::shared_ptr<HlsMediaSegment> mediaSegment, ByteBuffer &buffer);
      static bool DecryptSampleAes(std::shared_ptr<HlsMediaSegment> mediaSegment, ByteBuffer &buffer);

    private:

      static std::string ReplaceString(std::string text, const std::string &from, const std::string &to);
      static bool FillKeyValue(EncAes128 &encAes128);

      static int SampleSesHandlePesData(std::shared_ptr<HlsMediaSegment> mediaSegment, ByteBuffer &out, ByteBuffer &in, uint8_t *pcr, uint16_t pid, audiotype_t audio_codec, uint8_t *counter);
      static int SampleAesDecryptNalUnits(std::shared_ptr<HlsMediaSegment> mediaSegment, uint8_t *buf_in, int size);
      static bool SampleAesAppendAvData(ByteBuffer &out, ByteBuffer &in, const uint8_t *pcr, uint16_t pid, uint8_t *cont_count);
      static uint8_t* FFAvcFindStartcode(uint8_t *p, uint8_t *end);
      static uint8_t* FFAvcFindStartcodeInternal(uint8_t *p, uint8_t *end);
      static bool SampleAesDecryptAudioData(std::shared_ptr<HlsMediaSegment> mediaSegment, uint8_t *ptr, uint32_t size, audiotype_t audio_codec);
      static uint8_t* RemoveEmulationPrev(const uint8_t *src, const uint8_t *src_end, uint8_t *dst, uint8_t *dst_end);
    };
  } // namespace hls
} //namespace iptvsimple
