
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpegts.h"

#ifndef _MSC_VER
#include "msg.h"
#else
#define MSG_ERROR printf
#define MSG_WARNING printf
#define MSG_DBG printf
//void *memmem(const void *haystack, size_t n, const void *needle, size_t m);
#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

void *memmem(const void *haystack, size_t haystack_len,
    const void * const needle, const size_t needle_len)
{
    if (haystack == NULL) return NULL; // or assert(haystack != NULL);
    if (haystack_len == 0) return NULL;
    if (needle == NULL) return NULL; // or assert(needle != NULL);
    if (needle_len == 0) return NULL;

    for (const char *h = haystack;
            haystack_len >= needle_len;
            ++h, --haystack_len) {
        if (!memcmp(h, needle, needle_len)) {
            return h;
        }
    }
    return NULL;
}
#endif

/**
 * Possible frame sizes.
 * from ATSC A/52 Table 5.18 Frame Size Code Table.
 */
const uint16_t ac3_frame_size_tab[38][3] = {
    { 64,   69,   96   },
    { 64,   70,   96   },
    { 80,   87,   120  },
    { 80,   88,   120  },
    { 96,   104,  144  },
    { 96,   105,  144  },
    { 112,  121,  168  },
    { 112,  122,  168  },
    { 128,  139,  192  },
    { 128,  140,  192  },
    { 160,  174,  240  },
    { 160,  175,  240  },
    { 192,  208,  288  },
    { 192,  209,  288  },
    { 224,  243,  336  },
    { 224,  244,  336  },
    { 256,  278,  384  },
    { 256,  279,  384  },
    { 320,  348,  480  },
    { 320,  349,  480  },
    { 384,  417,  576  },
    { 384,  418,  576  },
    { 448,  487,  672  },
    { 448,  488,  672  },
    { 512,  557,  768  },
    { 512,  558,  768  },
    { 640,  696,  960  },
    { 640,  697,  960  },
    { 768,  835,  1152 },
    { 768,  836,  1152 },
    { 896,  975,  1344 },
    { 896,  976,  1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 },
};

#define INVALID_PTS_VALUE   0x200000000ull

typedef struct BitPacker_s
{
    uint8_t   *Ptr;                                    /* write pointer */
    uint32_t   BitBuffer;                              /* bitreader shifter */
    int32_t    Remaining;                              /* number of remaining in the shifter */
} BitPacker_t;

static void PutBits(BitPacker_t * ld, uint32_t code, uint32_t length)
{
    unsigned int bit_buf;
    unsigned int bit_left;

    bit_buf = ld->BitBuffer;
    bit_left = ld->Remaining;

    if (length < bit_left)
    {
        /* fits into current buffer */
        bit_buf = (bit_buf << length) | code;
        bit_left -= length;
    }
    else
    {
        /* doesn't fit */
        bit_buf <<= bit_left;
        bit_buf |= code >> (length - bit_left);
        ld->Ptr[0] = (char)(bit_buf >> 24);
        ld->Ptr[1] = (char)(bit_buf >> 16);
        ld->Ptr[2] = (char)(bit_buf >> 8);
        ld->Ptr[3] = (char)bit_buf;
        ld->Ptr   += 4;
        length    -= bit_left;
        bit_buf    = code & ((1 << length) - 1);
        bit_left   = 32 - length;
        bit_buf = code;
    }

    /* writeback */
    ld->BitBuffer = bit_buf;
    ld->Remaining = bit_left;
}

static void FlushBits(BitPacker_t *ld)
{
    ld->BitBuffer <<= ld->Remaining;
    while (ld->Remaining < 32)
    {
        *ld->Ptr++ = ld->BitBuffer >> 24;
        ld->BitBuffer <<= 8;
        ld->Remaining += 8;
    }
    ld->Remaining = 32;
    ld->BitBuffer = 0;
}

static int32_t InsertPesHeader(uint8_t *data, int32_t size, uint8_t stream_id, uint64_t pts, uint32_t pic_start_code)
{
    BitPacker_t ld2 = {data, 0, 32};

    if (size > (MAX_PES_PACKET_SIZE-13))
    {
        size = -1; // unbounded
    }

    PutBits(&ld2,0x0  ,8);
    PutBits(&ld2,0x0  ,8);
    PutBits(&ld2,0x1  ,8);  // Start Code
    PutBits(&ld2,stream_id ,8);  // Stream_id = Audio Stream
    //4
    if (-1 == size)
    {
        PutBits(&ld2,0x0,16);
    }
    else
    {
        PutBits(&ld2,size + 3 + (pts != INVALID_PTS_VALUE ? 5:0) + (pic_start_code ? 5:0), 16); // PES_packet_length
    }
    //6 = 4+2
    PutBits(&ld2,0x2  ,2);  // 10
    PutBits(&ld2,0x0  ,2);  // PES_Scrambling_control
    PutBits(&ld2,0x0  ,1);  // PES_Priority
    PutBits(&ld2,0x0  ,1);  // data_alignment_indicator
    PutBits(&ld2,0x0  ,1);  // Copyright
    PutBits(&ld2,0x0  ,1);  // Original or Copy
    //7 = 6+1

    if (pts!=INVALID_PTS_VALUE)
    {
        PutBits(&ld2,0x2 ,2);
    }
    else
    {
        PutBits(&ld2,0x0 ,2);  // PTS_DTS flag
    }

    PutBits(&ld2,0x0 ,1);  // ESCR_flag
    PutBits(&ld2,0x0 ,1);  // ES_rate_flag
    PutBits(&ld2,0x0 ,1);  // DSM_trick_mode_flag
    PutBits(&ld2,0x0 ,1);  // additional_copy_ingo_flag
    PutBits(&ld2,0x0 ,1);  // PES_CRC_flag
    PutBits(&ld2,0x0 ,1);  // PES_extension_flag
    //8 = 7+1

    if (pts!=INVALID_PTS_VALUE)
    {
        PutBits(&ld2,0x5,8);
    }
    else
    {
        PutBits(&ld2,0x0 ,8);  // PES_header_data_length
    }
    //9 = 8+1

    if (pts!=INVALID_PTS_VALUE)
    {
        PutBits(&ld2,0x2,4);
        PutBits(&ld2,(pts>>30) & 0x7,3);
        PutBits(&ld2,0x1,1);
        PutBits(&ld2,(pts>>15) & 0x7fff,15);
        PutBits(&ld2,0x1,1);
        PutBits(&ld2,pts & 0x7fff,15);
        PutBits(&ld2,0x1,1);
    }
    //14 = 9+5

    if (pic_start_code)
    {
        PutBits(&ld2,0x0 ,8);
        PutBits(&ld2,0x0 ,8);
        PutBits(&ld2,0x1 ,8);  // Start Code
        PutBits(&ld2,pic_start_code & 0xff ,8);  // 00, for picture start
        PutBits(&ld2,(pic_start_code >> 8 )&0xff,8);  // For any extra information (like in mpeg4p2, the pic_start_code)
        //14 + 4 = 18
    }

    FlushBits(&ld2);

    return (int32_t)(ld2.Ptr - data);
}

static const unsigned int crc32_table[] =
{
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
  0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
  0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
  0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
  0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
  0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
  0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
  0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
  0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
  0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
  0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
  0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
  0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
  0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
  0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
  0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
  0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
  0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
  0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
  0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
  0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
  0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
  0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
  0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
  0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
  0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
  0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
  0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
  0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
  0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
  0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
  0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
  0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
  0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
  0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static uint32_t crc32(const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t crc = 0xffffffff;

    for (i=0; i<len; i++)
            crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *data++) & 0xff];

    return crc;
}

static pidtype_t get_pid_type(uint16_t pid)
{
    pidtype_t pidtype = PT_UNSPEC;
    switch(pid)
    {
        case PID_PAT:
        case PID_CAT:
        case PID_TSDT:
        case PID_DVB_NIT:
            pidtype = PT_SECTIONS;
            break;
        case PID_NULL:
            pidtype = PT_NULL;
            break;
    }
    return pidtype;
}

bool parse_ts_packet(const uint8_t *data, ts_packet_t *packed)
{
    const uint8_t *bufp = data;

    // packed parsing

    bufp++; /* Skip sync byte */
    packed->unitstart = (bufp[0] & 0x40) >> 6; // payload_unit_start_indicator - 1b
    // transport_priority - 1b
    packed->pid = ((bufp[0] & 0x1f) << 8) | bufp[1]; // PID - 13b
    // transport_scrambling_control - 2b
    packed->afc = (bufp[2] & 0x30) >> 4; // adaptation_field_control - 2b
    packed->continuity = bufp[2] & 0x0f; // continuity_counter - 4b

    bufp += 3;
    if (packed->afc & 2)
    {
        bufp += bufp[0] + 1;
    }
    /*
    else if (packed->unitstart)
    {
        bufp += 1;
    }
    */

    packed->payload_offset = (uint8_t)(bufp - data);
    return true;
}


static bool parse_pmt(const uint8_t *data, pmt_data_t *pmt)
{
    const uint8_t *bufp = data;
    ts_packet_t packed;

    parse_ts_packet(bufp, &packed);

    if (packed.continuity != 0)
    {
        //MSG_WARNING("Not supported format continuity: 0x%hhx\n", packed.continuity);
        //return false;
    }

    pmt->pid = packed.pid;
    bufp += packed.payload_offset + 1;

    // section parssing
    uint8_t tableid = bufp[0]; // table_id - 8b
    if(TID_PMT != tableid)
    {
        MSG_WARNING("PMT wrong table id: 0x%hhx\n", tableid);
        return false;
    }
    uint8_t syntax = (bufp[1] & 0x80) >> 7; // section_syntax_indicator - 1b
    if (!syntax)
    {
        MSG_ERROR("Error PMT section should syntax set to 1\n");
        return false;
    }
    // '0' - 1b
    // reserved - 2b
    uint16_t seclen = ((bufp[1] & 0x0f) << 8) | bufp[2]; // section_length - 12b
    pmt->pmt_sectionlen = seclen + 3; // whole size of data
    if (pmt->pmt_sectionlen + (bufp - data) > TS_PACKET_LENGTH)
    {
        MSG_ERROR("PMT section to long seclen: 0x%hhx\n", seclen);
        return false;
    }

    pmt->pmt_idx = (uint16_t)(bufp - data);
    const uint8_t *ppmt = bufp;
    memcpy(pmt->data, data, TS_PACKET_LENGTH);

    bufp += 3;
    pmt->program = (bufp[0] << 8) | bufp[1]; // program_number - 16b
    // reserved - 2b
    // version_number - 5b
    // uint8_t curnext = (bufp[2] & 0x01); // current_next_indicator - 1b
    uint8_t section = bufp[3]; // section_number - 8b
    uint8_t last = bufp[4];    // last_section_number - 8b
    if (section != 0 && last != 0)
    {
        MSG_ERROR("PMT in more then one section, section_number: 0x%hhx, last_section_number: 0x%hhx\n", section, last);
        return false;
    }
    // reserved - 3b
    bufp += 5;

    pmt->pcrpid = ((bufp[0] & 0x1f) << 8) | bufp[1]; // PCR_PID - 13b
    // reserved - 4b
    uint16_t desclen = ((bufp[2] & 0x0f) << 8) | bufp[3]; // program_info_length - 12b
    bufp += 4;
    if (desclen + (bufp - ppmt) > pmt->pmt_sectionlen)
    {
        MSG_ERROR("PMT section to long desclen: 0x%hhx\n", desclen);
        return false;
    }
    // descriptor
    bufp += desclen;
    pmt->componennt_idx = (uint16_t)(bufp - data); //ppmt;
    while (pmt->pmt_sectionlen - (bufp - ppmt) - 4)
    {
        uint8_t offset = (uint8_t)(bufp - data); // offset in the TS_PACKET
        uint8_t stype = bufp[0]; // stream_type - 8b
        bufp+=1;
        // reserved - 3b
        uint16_t epid = ((bufp[0] & 0x1f) << 8) | bufp[1]; // elementary_PID - 13b
        bufp += 2;
        // reserved - 4b
        uint16_t infolen = ((bufp[0] & 0x0f) << 8) | bufp[1];// ES_info_length - 12b
        bufp += 2;
        bufp += infolen;
        if (pmt->component_num >= sizeof(pmt->components) / sizeof(pmt->components[0]))
        {
            MSG_ERROR("PMT section contain to manny components!\n");
            return false;
        }

        pmt->components[pmt->component_num].offset = offset;
        pmt->components[pmt->component_num].stream_type = stype;
        pmt->components[pmt->component_num].elementary_PID = epid;
        pmt->component_num += 1;
    }

    return true;
}

bool find_pmt(const uint8_t *bufp, uint32_t size, pmt_data_t *pmt)
{
    // maybe better will be to find PAT first and then we will know PMT PID
    while (size > TS_PACKET_LENGTH)
    {
        if (TS_SYNC_BYTE == bufp[0])
        {
            uint16_t pid = ((bufp[1] & 0x1f) << 8) | bufp[2]; // PID - 13b
            if (PT_UNSPEC == get_pid_type(pid))
            {
                if ( parse_pmt(bufp, pmt) )
                {
                    return true;
                }
            }
            bufp += TS_PACKET_LENGTH;
            size -= TS_PACKET_LENGTH;
        }
        else
        {
            MSG_WARNING("Missing sync byte!!!\n");
            bufp += 1;
        }
    }
    return false;
}

static bool merge_pmt_with_audio_component(const pmt_data_t *pmt1, const uint8_t *componennt_data, uint32_t component_len2, pmt_data_t *pmt)
{
    uint32_t len1 = pmt1->pmt_idx + pmt1->pmt_sectionlen;
    uint32_t component_len1 = len1 - pmt1->componennt_idx - 4;

    if (len1 + component_len2 > TS_PACKET_LENGTH)
    {
        MSG_ERROR("Merged PMT to long for one TS packet!");
        return false;
    }

    memcpy(pmt, pmt1, sizeof(*pmt1));
    memcpy(pmt->data + pmt->componennt_idx + component_len1, componennt_data, component_len2);

    // update section len
    pmt->pmt_sectionlen = pmt1->pmt_sectionlen + component_len2;
    if (pmt->pmt_sectionlen > 1023)
    {
        MSG_ERROR("Merged PMT section to long: %d!", pmt->pmt_sectionlen);
        return false;
    }
    uint16_t sectionlen = pmt->pmt_sectionlen - 3;
    pmt->data[pmt->pmt_idx + 1] = (pmt->data[pmt->pmt_idx + 1] & 0xf0) | (sectionlen & 0x0F00) >> 8;
    pmt->data[pmt->pmt_idx + 2] = (sectionlen & 0x00FF);

    pmt_update_crc(pmt);
    return true;
}

void pmt_update_crc(pmt_data_t *pmt)
{
    // update crc
    uint32_t crc = crc32(pmt->data + pmt->pmt_idx, pmt->pmt_sectionlen-4);

    pmt->data[pmt->pmt_idx + pmt->pmt_sectionlen - 4] = (crc >> 24) & 0xff;
    pmt->data[pmt->pmt_idx + pmt->pmt_sectionlen - 3] = (crc >> 16) & 0xff;
    pmt->data[pmt->pmt_idx + pmt->pmt_sectionlen - 2] = (crc >> 8) & 0xff;
    pmt->data[pmt->pmt_idx + pmt->pmt_sectionlen - 1] = crc & 0xff;
}

static bool merge_pmt(const pmt_data_t *pmt1, const pmt_data_t *pmt2, pmt_data_t *pmt)
{
    bool bRet = false;

    if (sizeof(pmt1->components) / sizeof(pmt1->components[0]) <= pmt1->component_num)
    {
        MSG_ERROR("Components table to small!!!\n");
        exit(-13);
    }

    if (pmt2->component_num > 1)
    {
        MSG_WARNING("Audio fragment has more then one component in the PMT component_num: %u!!!!\n", (uint32_t)pmt2->component_num);
    }

    if (pmt1->program != pmt2->program)
    {
        MSG_WARNING("Diffrent program ids %04x != %04x!!!\n", (uint32_t)pmt1->program, (uint32_t)pmt2->program);
        //return false;
    }

    uint32_t len2 = pmt2->pmt_idx + pmt2->pmt_sectionlen;
    uint32_t component_len2 = len2 - pmt2->componennt_idx - 4;
    uint8_t *ptr = malloc(component_len2);

    // assign new elementary pid for component
    uint16_t ePID = pmt1->components[pmt1->component_num-1].elementary_PID + 23;

    // update elementary_PID in component data
    memcpy(ptr, pmt2->data + pmt2->componennt_idx, component_len2);
    ptr[1] = ePID >> 8;
    ptr[2] = ePID & 0xFF;

    bRet = merge_pmt_with_audio_component(pmt1, ptr, component_len2, pmt);
    free(ptr);

    if (bRet)
    {
        // add new component to component table
        pmt->components[pmt->component_num] = pmt2->components[0];
        pmt->components[pmt->component_num].elementary_PID = ePID;
        pmt->component_num += 1;
    }
    return bRet;
}

static const uint8_t* get_dts_from_id3(const uint8_t *buf, uint32_t size, int64_t *dts)
{
    if (size >= 73)
    {
        uint32_t len = 0;

        len = ((uint32_t)(buf[6] & 0x7f) << 21) |
              ((uint32_t)(buf[7] & 0x7f) << 14) |
              ((uint32_t)(buf[8] & 0x7f) << 7) |
              ((uint32_t) buf[9] & 0x7f);
        len += 10; // header

        if (len <= size) {
            uint8_t *ptr = (uint8_t *)memmem(buf, size, "com.apple.streaming.transportStreamTimestamp", 44);
            if (ptr) {
                ptr += 45;
                if (buf + len >= ptr + 8)
                {
                    int64_t ts = (int64_t)ptr[0] << 56 | (int64_t)ptr[1] << 48 | (int64_t)ptr[2] << 40 | (int64_t)ptr[3] << 32 |
                                 (int64_t)ptr[4] << 24 | (int64_t)ptr[5] << 16 | (int64_t)ptr[6] << 8 | (int64_t)ptr[7];
                    MSG_DBG("HLS ID3 audio timestamp %lld\n", ts);
                    if ((ts & ~((1ULL << 33) - 1)) == 0)
                    {
                        *dts = ts;
                        return (buf + len);
                    }
                    else
                    {
                        MSG_ERROR("Invalid HLS ID3 audio timestamp %lld\n", ts);
                    }
                }
            }
        }
    }
    return NULL;
}

bool adts_get_next_frame(const uint8_t **data_ptr, const uint8_t *end_ptr, uint32_t *frame_length)
{
    for (; *data_ptr + 7 < end_ptr; *data_ptr += 1)
    {
           /* check for sync bits 0xfff */
           if ((*data_ptr)[0] != 0xff || ((*data_ptr)[1] & 0xf0) != 0xf0) continue;

           *frame_length = ((uint32_t)((*data_ptr)[3] & 0x03) << 11) | ((uint32_t)(*data_ptr)[4] << 3) | ((uint32_t)((*data_ptr)[5] & 0xe0) >> 5);

           /* sanity check on the frame length */
           if (*frame_length < 1 || *frame_length > 8 * 768 + 7 + 2) continue;
           return (*data_ptr) + (*frame_length) <= end_ptr;
    }
    return false;
}

bool ac3_get_next_frame(const uint8_t **data_ptr, const uint8_t *end_ptr, uint32_t *frame_length)
{

    for (; *data_ptr + 7 < end_ptr; *data_ptr += 1)
    {
        // check for sync bits 0x0B77
        if ((*data_ptr)[0] != 0x0B || (*data_ptr)[1] != 0x77) continue;

        // AC3 bitstream_id must be <= 10
        if ((((*data_ptr)[5] >> 3) & 0x1F) > 10) continue;

        uint8_t sr_code = ((*data_ptr)[4] >> 6) & 0x3;
        if (sr_code == 0x3) continue; // 11 reserved

        uint8_t frame_size_code = (*data_ptr)[4] & 0x3F;
        if (frame_size_code > 37) continue; // 100110 to 111111 reserved

        *frame_length = ac3_frame_size_tab[frame_size_code][sr_code] * 2;

        return (*data_ptr) + (*frame_length) <= end_ptr;
    }
    return false;
}

bool ec3_get_next_frame(const uint8_t **data_ptr, const uint8_t *end_ptr, uint32_t *frame_length)
{

    for (; *data_ptr + 7 < end_ptr; *data_ptr += 1)
    {
        // check for sync bits 0x0B77
        if ((*data_ptr)[0] != 0x0B || (*data_ptr)[1] != 0x77) continue;

        //Enhanced  AC3 bitstream_id must be > 10 and < 16
        if ((((*data_ptr)[5] >> 3) & 0x1F) < 11 || (((*data_ptr)[5] >> 3) & 0x1F) > 16) continue;

        *frame_length = (((((uint16_t)(*data_ptr)[2] & 0x7) << 8) | (uint16_t)(*data_ptr)[3]) + 1) << 1;

        return (*data_ptr) + (*frame_length) <= end_ptr;
    }
    return false;
}

static size_t do_merge_with_raw_audio(merge_context_t *context, const uint8_t *pdata1, uint32_t size1, const uint8_t *pdata2, uint32_t size2, int64_t dts, audiotype_t audiotype)
{
    uint8_t ts_header[4] = {TS_SYNC_BYTE, 0x00, 0x00, 0x10};
    uint8_t cont_count = 0;
    size_t ret = 0;
    uint16_t epid = 0;
    uint8_t stream_type = 0;
    uint8_t stream_id = 0;
    bool (* raw_audio_get_next_frame)(const uint8_t **, const uint8_t *, uint32_t *);

    switch (audiotype)
    {
    case AUDIO_ADTS:
        stream_type = 0xF; // stream_type: 0xF (15) => ISO/IEC 13818-7 Audio with ADTS transport syntax
        stream_id = 0xC0;  // stream_id: 0xC0 (192) => ISO/IEC 13818-3 or ISO/IEC 11172-3 or ISO/IEC 13818-7 or ISO/IEC 14496-3 audio stream number 0
        raw_audio_get_next_frame = adts_get_next_frame;
        break;
    case AUDIO_AC3:
        stream_type = 0x81; // stream_type: 0x81 (129) => User Private / AC-3 (ATSC)
        stream_id = 0xBD;   // stream_id: 0xBD (189) => private_stream_1
        raw_audio_get_next_frame = ac3_get_next_frame;
        break;
    case AUDIO_EC3:
        stream_type = 0x87; // stream_type: 0x87 (135) => User Private / E-AC-3 (ATSC)
        stream_id = 0xBD;   // stream_id: 0xBD (189) => private_stream_1
        raw_audio_get_next_frame = ec3_get_next_frame;
        break;
    case AUDIO_UNKNOWN:
    default:
        MSG_ERROR("Wrong audiotype! Should never happen here > EXIT!\n");
        exit(1);
    }

    if ( !context->valid && find_pmt(pdata1, size1, &context->pmt1))
    {
        uint8_t component_audio[5] = {stream_type, 0x00, 0x00, 0x00, 0x00};
        epid = context->pmt1.components[0].elementary_PID + 2;
        component_audio[1] = (epid >> 8) & 0x1F;
        component_audio[2] = epid & 0xFF;
        if (merge_pmt_with_audio_component(&context->pmt1, component_audio, 5, &context->pmt))
        {
            context->valid = true;
        }
    }

    if ( !context->valid )
    {
        MSG_ERROR("Invalid context!\n");
        return 0;
    }

    epid = context->pmt1.components[0].elementary_PID + 2;
    ts_header[1] = (epid >> 8) & 0x1F;
    ts_header[2] = epid & 0xFF;

    uint32_t count1 = size1 / TS_PACKET_LENGTH;
    uint32_t count2 = 0;

    // aac_adts_count_frames
    const uint8_t *data_ptr = pdata2;
    const uint8_t *end_ptr = pdata2 + size2;
    uint32_t frame_length = 0;
    while (raw_audio_get_next_frame(&data_ptr, end_ptr, &frame_length))
    {
        data_ptr += frame_length;
        ++count2;
    }

    if (count1 && count2)
    {
        uint32_t max1 = 0;
        uint32_t max2 = 0;
        if (count1 > count2)
        {
            max1 = count1 / count2;
            max2 = 1;
        }
        else
        {
            max1 = 1;
            max2 = count2 / count1;
        }

        data_ptr = pdata2;
        end_ptr = pdata2 + size2;
        uint32_t i = 0;
        uint32_t j = 0;
        while (i < count1 || j < count2)
        {
            for(uint32_t k=0; k<max1 && i < count1; ++k, ++i, pdata1 += TS_PACKET_LENGTH)
            {
                if (TS_SYNC_BYTE == pdata1[0])
                {
                    uint16_t pid = ((pdata1[1] & 0x1f) << 8) | pdata1[2]; // PID - 13b
                    if (pid != context->pmt1.pid) {
                        if (context->out->write(pdata1, TS_PACKET_LENGTH, context->out->opaque)) ret += TS_PACKET_LENGTH;
                        if (TID_PAT == pid)
                        {
                            if (context->out->write(context->pmt.data, TS_PACKET_LENGTH, context->out->opaque)) ret += TS_PACKET_LENGTH;
                        }
                    }
                }
            }

            for(uint32_t k=0; k<max2 && j < count2; ++k, ++j)
            {
                if (raw_audio_get_next_frame(&data_ptr, end_ptr, &frame_length))
                {
                   // we got audio frame here
                   // add PES header, split to TS packets and add headers with addation field
                   uint8_t pes_header[PES_HEADER_WITH_PTS_SIZE];
                   int32_t pes_header_length = InsertPesHeader(pes_header, frame_length, stream_id, dts, 0);
                   if (pes_header_length > 0)
                   {
                       uint32_t left_payload_size = frame_length;
                       uint32_t towrite = 0;

                       ts_header[1] |= 0x40; // set payload_unit_start_indicator
                       ts_header[3] = (ts_header[3] & 0xf0) | cont_count;
                       cont_count = (cont_count + 1) % 16;

                       // write first packet with PES header

                       // check if check if alignment is needed
                       uint8_t available_size = TS_PACKET_LENGTH - 4 - pes_header_length;
                       if (available_size > left_payload_size) {
                            towrite = left_payload_size;
                            ts_header[3] = (ts_header[3] & 0xcf) | 0x30; // set addaptation filed flag to add alignment
                       } else {
                            ts_header[3] = (ts_header[3] & 0xcf) | 0x10; // unset addaptation filed flag
                            towrite = available_size;
                       }
                       if (context->out->write(ts_header, 4, context->out->opaque)) ret += 4;

                       if (available_size > left_payload_size) {
                            uint8_t s;
                            uint8_t aflen = available_size - left_payload_size - 1;
                            uint8_t pattern = 0xff;
                            if (context->out->write(&aflen, 1, context->out->opaque)) ret += 1; // addaptation filed size without field size
                            if (aflen > 0)
                            {
                                pattern = 0x00;
                                if (context->out->write(&pattern, 1, context->out->opaque)) ret += 1;
                                pattern = 0xff;
                            }

                            for(s=1; s < aflen; ++s)
                            {
                                if (context->out->write(&pattern, 1, context->out->opaque)) ret += 1;
                            }
                       }

                       if (context->out->write(pes_header, pes_header_length, context->out->opaque)) ret += pes_header_length;
                       if (context->out->write(data_ptr, towrite, context->out->opaque)) ret += towrite;
                       left_payload_size -= towrite;
                       data_ptr += towrite;

                       if (left_payload_size > 0)
                       {
                           uint32_t packets_num = left_payload_size / (TS_PACKET_LENGTH - 4);
                           uint32_t p;
                           ts_header[1] &= 0xBF; // unset payload_unit_start_indicator
                           ts_header[3] = (ts_header[3] & 0xcf) | 0x10; // unset addaptation filed flag
                           for (p=0; p < packets_num; ++p)
                           {
                               ts_header[3] = (ts_header[3] & 0xf0) | cont_count;
                               cont_count = (cont_count + 1) % 16;
                               if (context->out->write(ts_header, 4, context->out->opaque)) ret += 4;
                               if (context->out->write(data_ptr, TS_PACKET_LENGTH - 4, context->out->opaque)) ret += TS_PACKET_LENGTH - 4;
                               data_ptr += (TS_PACKET_LENGTH - 4);
                           }
                           left_payload_size -= (TS_PACKET_LENGTH - 4) * packets_num;

                           if (left_payload_size > 0)
                           {
                               uint8_t s;
                               uint8_t aflen = TS_PACKET_LENGTH - 4 - left_payload_size - 1;
                               uint8_t pattern = 0xff;
                               ts_header[3] = (ts_header[3] & 0xcf) | 0x30; // set addaptation filed flag to add alignment
                               ts_header[3] = (ts_header[3] & 0xf0) | cont_count;
                               cont_count = (cont_count + 1) % 16;
                               if (context->out->write(ts_header, 4, context->out->opaque)) ret += 4;
                               ts_header[3] = (ts_header[3] & 0xcf) | 0x10; // unset addaptation filed flag
                               if (context->out->write(&aflen, 1, context->out->opaque)) ret += 1; // addaptation filed size without field size
                               if (aflen > 0)
                               {
                                    pattern = 0x00;
                                    if (context->out->write(&pattern, 1, context->out->opaque)) ret += 1;
                                    pattern = 0xff;
                               }

                               for(s=1; s < aflen; ++s)
                               {
                                    if (context->out->write(&pattern, 1, context->out->opaque)) ret += 1;
                               }
                               if (context->out->write(data_ptr, left_payload_size, context->out->opaque)) ret += left_payload_size;

                               data_ptr += left_payload_size;
                           }
                       }
                   }
                   else
                   {
                       MSG_WARNING("Wrong pes header length: %d!\n", pes_header_length);
                       data_ptr += frame_length;
                   }

                   dts = INVALID_PTS_VALUE;
                   break;
                }
            }
        }
    }

    return ret;
}

static size_t merge_with_raw_audio(merge_context_t *context, const uint8_t *pdata1, uint32_t size1, const uint8_t *pdata2, uint32_t size2)
{
    int64_t dts;
    const uint8_t *ptr = get_dts_from_id3(pdata2, size2, &dts);
    if (ptr) {
        audiotype_t audiotype = AUDIO_UNKNOWN;
        size2 -= (uint32_t)(ptr - pdata2);

        if (size2 > 7 && ptr[0] == 0xFF && (ptr[1] & 0xf0)  == 0xF0) // ADTS syncword 0xFFF
        {
            audiotype = AUDIO_ADTS;
        }
        else if (size2 > 7 && ptr[0] == 0x0B && ptr[1] == 0x77) // AC3, E-AC syncword 0x0B77
        {
            uint8_t bitstream_id  = (ptr[5] >> 3) & 0x1F;
            if (bitstream_id > 16)
            {
                MSG_ERROR("Unknown AC3/E-AC header - BSID parse error!\n");
                return 0;
            }
            else if(bitstream_id <= 10)
            {
                /* Normal AC-3 */
                audiotype = AUDIO_AC3;
            }
            else
            {
                /* Enhanced AC-3 */
                audiotype = AUDIO_EC3;
            }
        }
        else
        {
            MSG_ERROR("RAW audio stream: codec not supported!\n");
            exit(1);
        }
        return do_merge_with_raw_audio(context, pdata1, size1, ptr, size2, dts, audiotype);
    }
    else
    {
        MSG_ERROR("ID3 parsing failed!\n");
    }
    return 0;
}

static size_t merge_ts_packets(merge_context_t *context, const uint8_t *pdata1, uint32_t size1, const uint8_t *pdata2, uint32_t size2)
{
    size_t ret = 0;
    if ( !context->valid)
    {
        if (find_pmt(pdata1, size1, &context->pmt1) &&
            find_pmt(pdata2, size2, &context->pmt2))
        {
            if (merge_pmt(&context->pmt1, &context->pmt2, &context->pmt))
            {
                context->valid = true;
            }
        }
    }

    if (context->valid)
    {
        uint32_t count1 = size1 / TS_PACKET_LENGTH;
        uint32_t count2 = size2 / TS_PACKET_LENGTH;

        if (count1 && count2)
        {
            uint32_t max1 = 0;
            uint32_t max2 = 0;
            if (count1 > count2)
            {
                max1 = count1 / count2;
                max2 = 1;
            }
            else
            {
                max1 = 1;
                max2 = count2 / count1;
            }

            bool write_pmt = true;
            uint32_t i = 0;
            uint32_t j = 0;
            while (i < count1 || j < count2)
            {
                for(uint32_t k=0; k<max1 && i < count1; ++k, ++i, pdata1 += TS_PACKET_LENGTH)
                {
                    if (TS_SYNC_BYTE == pdata1[0])
                    {
                        uint16_t pid = ((pdata1[1] & 0x1f) << 8) | pdata1[2]; // PID - 13b
                        if (pid != context->pmt1.pid)
                        {
                            if (context->out->write(pdata1, TS_PACKET_LENGTH, context->out->opaque))
                            {
                                ret += TS_PACKET_LENGTH;
                            }

                            if (TID_PAT == pid && write_pmt)
                            {
                                write_pmt = false;
                                if (context->out->write(context->pmt.data, TS_PACKET_LENGTH, context->out->opaque))
                                {
                                    ret += TS_PACKET_LENGTH;
                                }
                            }
                        }
                    }
                }

                for(uint32_t k=0; k<max2 && j < count2; ++k, ++j, pdata2 += TS_PACKET_LENGTH)
                {
                    if (TS_SYNC_BYTE == pdata2[0])
                    {
                        uint16_t pid = ((pdata2[1] & 0x1f) << 8) | pdata2[2]; // PID - 13b
                        // if (TID_PAT != pid && pid != context->pmt2.pid)
                        if (pid == context->pmt2.components[0].elementary_PID)
                        {
                            if (context->out->write(pdata2, 1, context->out->opaque)) ret += 1;

                            // we need to update PID
                            pid = context->pmt.components[context->pmt.component_num-1].elementary_PID;
                            uint8_t tmp[2];
                            tmp[0] = (pdata2[1] & 0xE0) | (pid >> 8);
                            tmp[1] = pid & 0xFF;
                            if (context->out->write(tmp, 2, context->out->opaque)) ret += 2;

                            if (context->out->write(pdata2+3, TS_PACKET_LENGTH-3, context->out->opaque)) ret += TS_PACKET_LENGTH-3;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

size_t merge_packets(merge_context_t *context, const uint8_t *pdata1, uint32_t size1, const uint8_t *pdata2, uint32_t size2)
{
    if (size2 > 3 && 0 == memcmp(pdata2, "ID3", 3))
    {
        return merge_with_raw_audio(context, pdata1, size1, pdata2, size2);
    }
    else
    {
        return merge_ts_packets(context, pdata1, size1, pdata2, size2);
    }
}
