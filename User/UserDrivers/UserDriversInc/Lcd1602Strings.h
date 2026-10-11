#ifndef INC_LCD1602_STRINGS_H_
#define INC_LCD1602_STRINGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "Lcd1602Driver.h"

#if (LCD1602_LANGUAGE_CYRILLIC == 1)
    #define LCD1602_STR_HELLO           "ПРИВЕТ"
    #define LCD1602_STR_START           "СТАРТ"
    #define LCD1602_STR_HELLO_ASCII     "Hello"
#else
    #define LCD1602_STR_HELLO           "HELLO"
    #define LCD1602_STR_START           "START"
    #define LCD1602_STR_HELLO_ASCII     "Hello"
#endif

#ifdef __cplusplus
}
#endif

#endif // INC_LCD1602_STRINGS_H_
