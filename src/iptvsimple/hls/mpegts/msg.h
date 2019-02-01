#ifndef __HLS_DownLoad__msg__
#define __HLS_DownLoad__msg__

#ifdef __cplusplus
extern "C" {
#endif

#define LVL_ERROR 0x01
#define LVL_WARNING 0x02
#define LVL_VERBOSE 0x03
#define LVL_DBG 0x04
#define LVL_PRINT 0x05
#define LVL_API 0x06

#define MSG_API(...) msg_print_va(LVL_API, __VA_ARGS__)
#define MSG_ERROR(...) msg_print_va(LVL_ERROR, __VA_ARGS__)
#define MSG_WARNING(...) msg_print_va(LVL_WARNING, __VA_ARGS__)
#define MSG_VERBOSE(...) msg_print_va(LVL_VERBOSE, __VA_ARGS__)
#define MSG_DBG(...) msg_print_va(LVL_DBG, __VA_ARGS__)
#define MSG_PRINT(...) msg_print_va(LVL_PRINT, __VA_ARGS__)

int msg_print_va(int lvl, char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* defined(__HLS_DownLoad__msg__) */
