#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "msg.h"
//#include "misc.h"

int msg_print_va(int lvl, char *fmt, ...)
{
    int result = 0;
    va_list args;
    va_start(args, fmt);
    
    //if (hls_args.loglevel >= 0 || (lvl == LVL_API && hls_args.loglevel >= -1)) {
        switch(lvl)
        {
            case LVL_API:
                result = vfprintf(stderr, fmt, args);
            break;
            case LVL_ERROR:
                fputs("Error: ", stderr);
                result = vfprintf(stderr, fmt, args);
            break;
            case LVL_WARNING:
                fputs("Warning: ", stderr);
                result = vfprintf(stderr, fmt, args);
            break;
            case LVL_VERBOSE:
                if (0) {//ls_args.loglevel > 0) {
                    result = vfprintf(stderr, fmt, args);
                }
            break;
            case LVL_DBG:
                if (0) { //hls_args.loglevel > 1) {
                    fputs("Debug: ", stderr);
                    result = vfprintf(stderr, fmt, args);
                }
            break;
            case LVL_PRINT:
                result = vfprintf(stderr, fmt, args);
            break;
            default:
                break;
        }
    //}
    va_end(args);
    return result;
}
