
#ifndef _u_dtype_h
#define _u_dtype_h

/* data types: UINT_x:  x-bit data */

#ifdef HAVE_STDINT_H
#include <stdint.h>

#define UINT_8  uint8_t
#define UINT_16 uint16_t
#define UINT_32 uint32_t
#define UINT_64 uint64_t
#define SINT_8  int8_t
#define SINT_16 int16_t
#define SINT_32 int32_t
#define SINT_64 int64_t
#endif

#ifndef HAVE_STDINT_H
#define UINT_8  unsigned char
#define UINT_16 unsigned short
#define UINT_32 unsigned long
#define UINT_64 unsigned long long
#define SINT_8  char
#define SINT_16 short
#define SINT_32 long
#define SINT_64 long long
#endif


typedef void (*weird_pointer_jive)(void);
typedef void (*weird_pointer_jive_wargs)(void *);

#define TAKETWOS(a)             ( ( ( ( a ) ^ 0x0ff ) + 0x001 ) & 0x0ff )
#define MASKEDADD8(a,b)         ( ( ( a ) + ( b ) ) & 0x0ff )
#define SETLOWER8OF16(a,b)      ( ( ( a ) & 0x0ff00 ) | ( ( b ) & 0x000ff ) )
#define SETUPPER8OF16(a,b)      ( ( ( a ) & 0x000ff ) | ( ( ( (UINT_16) ( b ) ) << 8 ) & 0x0ff00 ) )


#endif
