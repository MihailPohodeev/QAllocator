#ifndef _SETUP_H_
#define _SETUP_H_

#include <stdint.h>

#define POINTER_SIZE_64

using U8  = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;
using I8  = int8_t;
using I16 = int16_t;
using I32 = int32_t;
using I64 = int64_t;

// type for descriptor-value.
using descriptor_t = U64;

#ifdef POINTER_SIZE_32

using integer_value_of_pointer = U32;

#elif defined(POINTER_SIZE_64)

using integer_value_of_pointer = U64;

#endif


#endif
