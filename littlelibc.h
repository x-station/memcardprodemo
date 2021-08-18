// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdarg.h>
#include <stddef.h>

#include <string_view>

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

template<typename T, ulong N> constexpr ulong array_size( T ( & )[ N ] ) { return N; }

// These will obviously need refactored at some point

#ifdef __cplusplus
extern "C" {
void *memset( void *s_, int c, size_t n );
void *fastMemset( void *ptr, int value, size_t num );
size_t strlen( const char *s );
void memcpy( void *dst, const void *src, ulong len );
}
#endif

void NewStrcpy( void *dst, const void *src );
int NewStrncmp( const char *paramA, const char *paramB, ulong len );
int NewStrcmp( const char *paramA, const char *paramB );

struct Printer {
    virtual void Print( char c ) = 0;
    virtual void Print( const std::string_view str );
};

void NewVXPrintf( Printer *printer, const std::string_view str, va_list list );
void NewPrintf( const std::string_view str, ... );
void NewSPrintf( char *out, const std::string_view str, ... );