#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HSI_VALUE
    #define HSI_VALUE 16'000'000UL
#endif

#ifndef HSE_VALUE
    #define HSE_VALUE 16'000'000UL
#endif

#ifndef LSE_VALUE
    #define LSE_VALUE 32'768UL
#endif

#include "system_init_error.h"

#ifdef __cplusplus
}
#endif

#endif
