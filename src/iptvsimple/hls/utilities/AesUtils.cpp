/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#include "AesUtils.h"

#include "../../../client.h"

#include "WebUtils.h"
#include "../data/ByteBuffer.h"
#include "../data/EncAes128.h"
#include "../mpegts/aes.h"

using namespace ADDON;

using namespace iptvsimple::hls;

bool AesUtils::DecryptAes128(std::shared_ptr<HlsMediaSegment> mediaSegment, ByteBuffer &buffer)
{
  // The AES128 method encrypts whole segments.
  // Simply decrypting them is enough.
  if (!FillKeyValue(mediaSegment->m_encAes))
    return false;

  void *ctx = AES128_CBC_CTX_new();
  /* some AES-128 encrypted segments could be not correctly padded
    * and decryption with padding will fail - example stream with such problem is welcome
    * From other hand dump correctly padded segment will contain trashes, which will cause many
    * errors during processing such TS, for example by DVBInspector,
    * if padding will be not removed.
    */
//#if 1

  int outSize = 0;
  AES128_CBC_DecryptInit(ctx, mediaSegment->m_encAes.m_keyValue, mediaSegment->m_encAes.m_ivValue, true);
  AES128_CBC_DecryptPadded(ctx, buffer.data, buffer.data, buffer.len, &outSize);
  // decoded data size could be less then input because of the padding
  buffer.len = outSize;

// #else
//   AES128_CBC_DecryptInit(ctx, s->enc_aes.key_value, s->enc_aes.iv_value, false);
//   AES128_CBC_DecryptUpdate(ctx, buffer.data, buffer.data, buffer.len);
// #endif

  AES128_CBC_free(ctx);

  return true;
}

std::string AesUtils::ReplaceString(std::string text, const std::string& from, const std::string& to)
{
  size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos)
  {
    text.replace(pos, from.length(), to);
    pos += to.length();
  }
  return text;
}

bool AesUtils::FillKeyValue(EncAes128 &encAes128)
{
  /* temporary we will create cache with keys url here
    * this will make this function thread unsafe but at now
    * it is not problem because it is used only from one thread
    *
    * last allocation of cache_key_url will not be free
    * but this is not big problem since this code is run
    * as standalone process
    *(system will free all memory allocated by process at it exit).
    *
    * But this must be fixed for clear valgrind memory leak detection.
    */
  static char cacheKeyValue[KEYLEN] = "";
  static std::string cacheKeyUrl;

  if (!encAes128.m_keyUrl.empty())
  {
    if (!cacheKeyUrl.empty() && 0 == strcmp(cacheKeyUrl.c_str(), encAes128.m_keyUrl.c_str()))
    {
      std::copy(std::begin(cacheKeyValue), std::end(cacheKeyValue), std::begin(encAes128.m_keyValue));
    }
    else
    {
      std::string keyUrl;
      size_t size = 0;

      keyUrl = encAes128.m_keyUrl;

      std::string dataBuffer;

      int httpCode = WebUtils::GetDataRange(keyUrl, dataBuffer, &size, nullptr, OPEN_MAX_RETRIES);
      if (!(httpCode == 200 || httpCode == 206) || dataBuffer.empty())
      {
        XBMC->Log(LOG_ERROR, "%s Getting key-file [%s] failed", __FUNCTION__, keyUrl.c_str());
        return false;
      }

      std::copy(dataBuffer.begin(), dataBuffer.end(), std::begin(encAes128.m_keyValue));

      cacheKeyUrl = encAes128.m_keyUrl;
      std::copy(std::begin(encAes128.m_keyValue), std::end(encAes128.m_keyValue), std::begin(cacheKeyUrl));
    }

    encAes128.m_keyUrl.clear();
  }

  return true;
}

bool AesUtils::DecryptSampleAes(std::shared_ptr<HlsMediaSegment> mediaSegment, ByteBuffer &buffer)
{
  int ret = 0;

  if (!FillKeyValue(mediaSegment->m_encAes))
    return false;

  if (buffer.len > TS_PACKET_LENGTH && buffer.data[0] == TS_SYNC_BYTE)
  {
    pmt_data_t pmt = {0};
    if (find_pmt(buffer.data, buffer.len, &pmt))
    {
      bool write_pmt = true;
      uint16_t audio_PID = PID_UNSPEC;
      uint16_t video_PID = PID_UNSPEC;
      audiotype_t audio_codec = AUDIO_UNKNOWN;
      uint32_t i;
      // https://developer.apple.com/library/archive/documentation/AudioVideo/Conceptual/HLS_Sample_Encryption/TransportStreamSignaling/TransportStreamSignaling.html
      for (i=0; i < pmt.component_num; ++i)
      {
        uint8_t stream_type = pmt.components[i].stream_type;
        switch (stream_type)
        {
          case 0xdb:
              video_PID = pmt.components[i].elementary_PID;
              stream_type = 0x1B; // AVC video stream as defined in ITU-T Rec. H.264 | ISO/IEC 14496-10 Video, or AVC base layer of an HEVC video stream as defined in ITU-T H.265 | ISO/IEC 23008-2
              break;
          case 0xcf:
              audio_codec = AUDIO_ADTS;
              audio_PID = pmt.components[i].elementary_PID;
              stream_type = 0x0F; // ISO/IEC 13818-7 Audio with ADTS transport syntax
              break;
          case 0xc1:
              audio_codec = AUDIO_AC3;
              audio_PID = pmt.components[i].elementary_PID;
              stream_type = 0x81; // User Private / AC-3 (ATSC)
              break;
          case 0xc2:
              audio_codec = AUDIO_EC3;
              audio_PID = pmt.components[i].elementary_PID;
              stream_type = 0x87; // User Private / E-AC-3 (ATSC)
              break;
          default:
              XBMC->Log(LOG_DEBUG, "%s Unknown component type: 0x%02hhx, pid: 0x%03hx", __FUNCTION__, pmt.components[i].stream_type, pmt.components[i].elementary_PID);
              break;
        }

        if (stream_type != pmt.components[i].stream_type)
        {
          // we update stream type to reflect unencrypted data
          pmt.components[i].stream_type = stream_type;
          pmt.data[pmt.components[i].offset] = stream_type;
        }
      }

      if (audio_PID != PID_UNSPEC || video_PID != PID_UNSPEC)
      {
        uint8_t audio_counter = 0;
        uint8_t video_counter = 0;
        uint8_t audio_pcr[7]; // first byte is adaptation filed flags
        uint8_t video_pcr[7]; // - || -
        ByteBuffer outBuffer = {};
        outBuffer.data = new uint8_t[buffer.len];
        outBuffer.len = buffer.len;

        ByteBuffer audioBuffer = {};
        ByteBuffer videoBuffer = {};

        if (audio_PID != PID_UNSPEC)
        {
          audioBuffer.data = new uint8_t[buffer.len];
          audioBuffer.len = buffer.len;
        }

        if (video_PID != PID_UNSPEC)
        {
          videoBuffer.data = new uint8_t[buffer.len];
          videoBuffer.len = buffer.len;
        }

        // collect all audio and video data
        uint32_t packet_id = 0;
        uint8_t *ptr = buffer.data;
        uint8_t *end = ptr + buffer.len;
        while (ptr + TS_PACKET_LENGTH <= end)
        {
          if (*ptr != TS_SYNC_BYTE)
          {
            //MSG_WARNING("Expected sync byte but got 0x%02hhx!\n", *ptr);
            XBMC->Log(LOG_NOTICE, "%s Expected sync byte but got 0x%02hhx", __FUNCTION__, *ptr);
            ptr += 1;
            continue;
          }
          ts_packet_t packed = {0};
          parse_ts_packet(ptr, &packed);

          if (packed.pid == pmt.pid)
          {
            if (write_pmt)
            {
              write_pmt = false;
              pmt_update_crc(&pmt);
              memcpy(&outBuffer.data[outBuffer.pos], pmt.data, TS_PACKET_LENGTH);
              outBuffer.pos += TS_PACKET_LENGTH;
            }
          }
          else if (packed.pid == audio_PID || packed.pid == video_PID)
          {
            ByteBuffer *pCurrBuffer = packed.pid == audio_PID ? &audioBuffer : &videoBuffer;
            uint8_t *pcr = packed.pid == audio_PID ? audio_pcr : video_pcr;
            uint8_t *counter = packed.pid == audio_PID ? &audio_counter : &video_counter;

            if (packed.unitstart)
            {
              // consume previous data if any
              if (pCurrBuffer->pos)
              {
                SampleSesHandlePesData(mediaSegment, outBuffer, *pCurrBuffer, pcr, packed.pid, packed.pid == audio_PID ? audio_codec : AUDIO_UNKNOWN, counter);
              }

              if ((packed.afc & 2) && (ptr[5] & 0x10))
              { // remember PCR if available
                memcpy(pcr, ptr + 4 + 1, 6);
              }
              else if ((packed.afc & 2) && (ptr[5] & 0x20))
              { // remember discontinuity_indicator if set
                pcr[0] = ptr[5];
              }
              else
              {
                pcr[0] = 0;
              }
              pCurrBuffer->pos = 0;
            }

            if (packed.payload_offset < TS_PACKET_LENGTH)
            {
              memcpy(&(pCurrBuffer->data[pCurrBuffer->pos]), ptr + packed.payload_offset, TS_PACKET_LENGTH - packed.payload_offset);
              pCurrBuffer->pos += TS_PACKET_LENGTH - packed.payload_offset;
            }
          }
          else
          {
            memcpy(&outBuffer.data[outBuffer.pos], ptr, TS_PACKET_LENGTH);
            outBuffer.pos += TS_PACKET_LENGTH;
          }

          ptr += TS_PACKET_LENGTH;
          packet_id += 1;
        }

        if (audioBuffer.pos)
        {
            SampleSesHandlePesData(mediaSegment, outBuffer, audioBuffer, audio_pcr, audio_PID, audio_codec, &audio_counter);
        }

        if (videoBuffer.pos)
        {
            SampleSesHandlePesData(mediaSegment, outBuffer, videoBuffer, video_pcr, video_PID, AUDIO_UNKNOWN, &video_counter);
        }

        if (outBuffer.pos > buffer.len )
        {
            XBMC->Log(LOG_ERROR, "%s buffer overflow detected", __FUNCTION__);
            exit(-1);
        }

        delete videoBuffer.data;
        delete audioBuffer.data;

        // replace encrypted data with decrypted one
        delete buffer.data;
        buffer.data = outBuffer.data;
        buffer.len = outBuffer.pos;
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "%s No audio or video component found", __FUNCTION__);
        return false;
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "%s PMT could not be found", __FUNCTION__);
      return false;
    }
  }
  else
  {
    XBMC->Log(LOG_NOTICE, "%s Unknown segment type", __FUNCTION__);
    return false;
  }

  return true;
}

int AesUtils::SampleSesHandlePesData(std::shared_ptr<HlsMediaSegment> mediaSegment, ByteBuffer &out, ByteBuffer &in, uint8_t *pcr, uint16_t pid, audiotype_t audio_codec, uint8_t *counter)
{
  uint16_t pes_header_size = 0;

  // we need to skip PES header it is not part of NAL unit
  if (in.pos <= PES_HEADER_SIZE || in.data[0] != 0x00 || in.data[1] != 0x00 || in.data[1] == 0x01)
  {
    XBMC->Log(LOG_ERROR, "%s Wrong or missing PES header!", __FUNCTION__);
    return -1;
  }

  pes_header_size = in.data[8] + 9;
  if (pes_header_size >= in.pos)
  {
    XBMC->Log(LOG_ERROR, "%s Wrong PES header size %hu!", __FUNCTION__, &pes_header_size);
    return -1;
  }

  if (AUDIO_UNKNOWN == audio_codec)
  {
    // handle video data in NAL units
    int size = SampleAesDecryptNalUnits(mediaSegment, in.data + pes_header_size, in.pos - pes_header_size) + pes_header_size;

    // to check if I did not any mistake in offset calculation
    if (size > in.pos)
    {
      XBMC->Log(LOG_ERROR, "%s NAL size after decryption is grater then before - before: %d, after: %d - should never happen!", __FUNCTION__, size, in.pos);
      return -1;
    }

    // output size could be less then input because the start code emulation prevention could be removed if available
    if (size < in.pos)
    {
      // we need to update size in the PES header if it was set
      int32_t payload_size = ((uint16_t)(in.data[4]) << 8) | in.data[5];
      if (payload_size > 0)
      {
        payload_size -=  in.pos - size;
        in.data[4] = (payload_size >> 8) & 0xff;
        in.data[5] = payload_size & 0xff;
      }
      in.pos = size;
    }
  }
  else
  {
    SampleAesDecryptAudioData(mediaSegment, in.data + pes_header_size, in.pos - pes_header_size, audio_codec);
  }

  return SampleAesAppendAvData(out, in, pcr, pid, counter);
}

int AesUtils::SampleAesDecryptNalUnits(std::shared_ptr<HlsMediaSegment> mediaSegment, uint8_t *buf_in, int size)
{
  uint8_t *end = buf_in + size;
  uint8_t *nal_start;
  uint8_t *nal_end;

  end = RemoveEmulationPrev(buf_in, end, buf_in, end);

  nal_start = FFAvcFindStartcode(buf_in, end);
  for (;;)
  {
    while (nal_start < end && !*(nal_start++));
    if (nal_start == end)
        break;

    nal_end = FFAvcFindStartcode(nal_start, end);
    // NAL unit with length of 48 bytes or fewer is completely unencrypted.
    if (nal_end - nal_start > 48)
    {
      nal_start += 32;
      void *ctx = AES128_CBC_CTX_new();
      AES128_CBC_DecryptInit(ctx, mediaSegment->m_encAes.m_keyValue, mediaSegment->m_encAes.m_ivValue, false);
      while (nal_start + 16 < nal_end)
      {
        AES128_CBC_DecryptUpdate(ctx, nal_start, nal_start, 16);
        nal_start += 16 * 10; // Each 16-byte block of encrypted data is followed by up to nine 16-byte blocks of unencrypted data.
      }
      AES128_CBC_free(ctx);
    }
    nal_start = nal_end;
  }
  return (int)(end - buf_in);
}

bool AesUtils::SampleAesAppendAvData(ByteBuffer &out, ByteBuffer &in, const uint8_t *pcr, uint16_t pid, uint8_t *cont_count)
{
  uint8_t *av_data = in.data;
  uint32_t av_size = in.pos;

  uint8_t ts_header[4] = {TS_SYNC_BYTE, 0x40, 0x00, 0x10};
  ts_header[1] = ((pid >> 8) & 0x1F) | 0x40; // 0x40 - set payload_unit_start_indicator
  ts_header[2] = pid & 0xFF;

  uint8_t adapt_header[8] = {0x00};
  uint8_t adapt_header_size = 0;
  uint32_t payload_size = TS_PACKET_LENGTH - sizeof(ts_header);
  if (pcr[0] & 0x10)
  {
    adapt_header_size = 8;
    adapt_header[1] = pcr[0] & 0xF0; // set previus flags: discontinuity_indicator, random_access_indicator, elementary_stream_priority_indicator, PCR_flag
  }
  else if (pcr[0] & 0x20)
  {
    adapt_header_size = 2;
    adapt_header[1] = pcr[0] & 0xF0; // restore flags as described above
  }
  else if (av_size < payload_size)
  {
    adapt_header_size = payload_size - av_size == 1 ? 1 : 2;
  }

  payload_size -= adapt_header_size;

  if (adapt_header_size)
  {
    adapt_header[0] = adapt_header_size - 1; // size without field size
    if (av_size < payload_size)
    {
      adapt_header[0] += payload_size - av_size;
    }

    if (adapt_header[0])
    {
      ts_header[3] = (ts_header[3] & 0xcf) | 0x30; // set addaptation filed flag
    }
  }

  // ts header
  ts_header[3] = (ts_header[3] & 0xf0) | (*cont_count);
  *cont_count = (*cont_count + 1) % 16;
  memcpy(&out.data[out.pos], ts_header, sizeof(ts_header));
  out.pos += sizeof(ts_header);

  // adaptation field
  if (adapt_header_size)
  {
    memcpy(&out.data[out.pos], adapt_header, adapt_header_size);
    out.pos += adapt_header_size;
  }

  if (av_size < payload_size)
  {
    uint32_t s;
    for(s=0; s < payload_size - av_size; ++s)
    {
        out.data[out.pos + s] = 0xff;
    }
    out.pos += payload_size - av_size;
    payload_size = av_size;
  }

  memcpy(&out.data[out.pos], av_data, payload_size);
  out.pos += payload_size;
  av_data += payload_size;
  av_size -= payload_size;

  if (av_size > 0)
  {
    uint32_t packets_num = av_size / (TS_PACKET_LENGTH - 4);
    uint32_t p;
    ts_header[1] &= 0xBF; // unset payload_unit_start_indicator
    ts_header[3] = (ts_header[3] & 0xcf) | 0x10; // unset addaptation filed flag
    for (p=0; p < packets_num; ++p)
    {
      ts_header[3] = (ts_header[3] & 0xf0) | (*cont_count);
      *cont_count = (*cont_count + 1) % 16;
      memcpy(&out.data[out.pos], ts_header, sizeof(ts_header));
      memcpy(&out.data[out.pos+4], av_data, TS_PACKET_LENGTH - sizeof(ts_header));
      out.pos += TS_PACKET_LENGTH;
      av_data += TS_PACKET_LENGTH - sizeof(ts_header);
      av_size -= TS_PACKET_LENGTH - sizeof(ts_header);
    }

    ts_header[3] = (ts_header[3] & 0xcf) | 0x30; // set addaptation filed flag to add aligment
    adapt_header[1] = 0; // none flags set, only for alignment
    if (av_size > 0)
    {
      ts_header[3] = (ts_header[3] & 0xf0) | (*cont_count);
      *cont_count = (*cont_count + 1) % 16;
      // add ts_header
      memcpy(&out.data[out.pos], ts_header, sizeof(ts_header));

      // add adapt header
      adapt_header_size = TS_PACKET_LENGTH - 4 - av_size == 1 ? 1 : 2;
      adapt_header[0] = adapt_header_size - 1; // size without field size
      if (adapt_header[0])
      {
        adapt_header[0] +=  TS_PACKET_LENGTH - 4 - 2 - av_size;
      }

      memcpy(&out.data[out.pos+4], adapt_header, adapt_header_size);
      out.pos += 4 + adapt_header_size;

      // add alignment
      if (adapt_header[0])
      {
        int32_t s;
        for(s=0; s < adapt_header[0] - 1; ++s)
        {
          out.data[out.pos + s] = 0xff;
        }
        out.pos += adapt_header[0] -1;
      }

      // add payload
      memcpy(out.data + out.pos, av_data, av_size);
      out.pos += av_size;
      av_data += av_size;
      av_size -= av_size;
    }
  }

  return true;
}

uint8_t* AesUtils::FFAvcFindStartcode(uint8_t *p, uint8_t *end)
{
  uint8_t *out = FFAvcFindStartcodeInternal(p, end);
  if(p<out && out<end && !out[-1]) out--;
  return out;
}

bool AesUtils::SampleAesDecryptAudioData(std::shared_ptr<HlsMediaSegment> mediaSegment, uint8_t *ptr, uint32_t size, audiotype_t audio_codec)
{
  bool (* get_next_frame)(const uint8_t **, const uint8_t *, uint32_t *);
  switch (audio_codec)
  {
    case AUDIO_ADTS:
        get_next_frame = adts_get_next_frame;
        break;
    case AUDIO_AC3:
        get_next_frame = ac3_get_next_frame;
        break;
    case AUDIO_EC3:
        get_next_frame = ec3_get_next_frame;
        break;
    case AUDIO_UNKNOWN:
    default:
        XBMC->Log(LOG_ERROR, "%s Wrong audio_codec! Should never happen here", __FUNCTION__);
        return false;
  }

  uint8_t *audio_frame = ptr;
  uint8_t *end_ptr = ptr + size;
  uint32_t frame_length = 0;
  while (get_next_frame((const uint8_t **)&audio_frame, end_ptr, &frame_length))
  {
    // The IV must be reset at the beginning of every packet.
    uint8_t leaderSize = 0;

    if (audio_codec == AUDIO_ADTS)
    {
      // ADTS headers can contain CRC checks.
      // If the CRC check bit is 0, CRC exists.
      //
      // Header (7 or 9 byte) + unencrypted leader (16 bytes)
      leaderSize = (audio_frame[1] & 0x01) ? 23 : 25;
    }
    else
    {
      // AUDIO_AC3, AUDIO_EC3
      // AC3 Audio is untested. Sample streams welcome.
      //
      // unencrypted leader
      leaderSize = 16;
    }

    int tmp_size = frame_length > leaderSize ? (frame_length - leaderSize) & 0xFFFFFFF0  : 0;
    if (tmp_size)
    {
      void *ctx = AES128_CBC_CTX_new();
      AES128_CBC_DecryptInit(ctx, mediaSegment->m_encAes.m_keyValue, mediaSegment->m_encAes.m_ivValue, false);
      AES128_CBC_DecryptUpdate(ctx, audio_frame + leaderSize, audio_frame + leaderSize, tmp_size);
      AES128_CBC_free(ctx);
    }

    audio_frame += frame_length;
  }

  return true;
}

uint8_t* AesUtils::RemoveEmulationPrev(const uint8_t *src, const uint8_t *src_end, uint8_t *dst, uint8_t *dst_end)
{
  while (src + 2 < src_end)
  {
    if (!*src && !*(src + 1) && *(src + 2) == 3)
    {
      *dst++ = *src++;
      *dst++ = *src++;
      src++; // remove emulation_prevention_three_byte
    }
    else
    {
      *dst++ = *src++;
    }
  }

  while (src < src_end)
    *dst++ = *src++;

  return dst;
}

uint8_t* AesUtils::FFAvcFindStartcodeInternal(uint8_t *p, uint8_t *end)
{
  uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4)
  {
    uint32_t x = *(uint32_t*)p;
    if ((x - 0x01010101) & (~x) & 0x80808080)
    { // generic
      if (p[1] == 0)
      {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p + 1;
      }
      if (p[3] == 0)
      {
        if (p[2] == 0 && p[4] == 1)
          return p + 2;
        if (p[4] == 0 && p[5] == 1)
          return p + 3;
      }
    }
  }

  for (end += 3; p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}