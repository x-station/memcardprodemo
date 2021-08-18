// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// I think most of the thanks goes to Nicolas Noble / PCSX here
// It's a mish mash tho!
// Reference:
// https://github.com/grumpycoders/uC-sdk/blob/master/os/src/osdebug.c

#include "littlelibc.h"

#include <stdarg.h>

// This is part of the standard C lib, right?
#define CHAR_PERCENT 0x25

// Not ABI compliant. Works for 4 params before it hits the stack
// See printf.s instead
/*
void NewPrintf(const char* str, ...) {
    register int cmd __asm__("t1") = 0x3F;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    ((void (*)(const char*))0xA0)(str);
}
*/

static inline int PutC( int c, int fd ) {
    register int cmd __asm__( "t1" ) = 0x09;
    __asm__ volatile( "" : "=r"( cmd ) : "r"( cmd ) );
    return ((int ( * )( int, int ))0xA0)( c, fd );
}

static inline int Write( int fd, const void *ptr, ulong len ) {
    register int cmd __asm__( "t1" ) = 0x03;
    __asm__ volatile( "" : "=r"( cmd ) : "r"( cmd ) );
    return ((int ( * )( int, const void *, ulong ))0xA0)( fd, ptr, len );
}

void Printer::Print( std::string_view str ) {
    for( auto c : str )
        Print( c );
}

void NewStrcpy( void *dst_, const void *src_ ) {
    char *dst = (char *)dst_;
    char *src = (char *)src_;
    while( *src != 0 ) {
        *dst++ = *src++;
    }
    // write the trailing zero
    *dst = 0;
}

void memcpy( void *dst_, const void *src_, ulong len ) {
    char *dst = (char *)dst_;
    char *src = (char *)src_;
    int i = 0;
    for( i = 0; i < len; i++ ) {
        *dst++ = *src++;
    }
}

int NewStrncmp( const char *paramA, const char *paramB, ulong len ) {
    ulong countyCount = 0;

    while( !( *paramA == 0 && *paramB == 0 ) && countyCount++ < len ) {
        if( *paramA++ != *paramB++ ) {
            return ( *paramA > *paramB ? 1 : -1 );
        }
    }
    return 0;
}

int NewStrcmp( const char *paramA, const char *paramB ) {
    ulong countyCount = 0;

    while( !( *paramA == 0 && *paramB == 0 ) && countyCount++ < 1024 ) {
        if( *paramA++ != *paramB++ ) {
            return ( *paramA > *paramB ? 1 : -1 );
        }
    }
    return 0;
}

void NewPrintf( const std::string_view str, ... ) {
    va_list list;
    va_start( list, str );
    struct TTYPrinter final : public Printer {
        virtual void Print( char c ) final override { PutC( c, 0 ); }
        virtual void Print( const std::string_view str ) { Write( 1, str.data(), str.length() ); }
    } ttyprinter;
    NewVXPrintf( &ttyprinter, str, list );
    va_end( list );
}

void NewSPrintf( char *out, const std::string_view str, ... ) {
    va_list list;
    va_start( list, str );
    struct StrPrinter final : public Printer {
        StrPrinter( char *out ) : out( out ) {}
        virtual void Print( char c ) final override { *out++ = c; }
        virtual void Print( const std::string_view str ) {
            memcpy( out, str.data(), str.length() );
            out += str.length();
        }
        void end() { *out = 0; }

      private:
        char *out;
    } strprinter( out );
    NewVXPrintf( &strprinter, str, list );
    va_end( list );
    strprinter.end();
}

void NewVXPrintf( Printer *printer, const std::string_view in, va_list list ) {
    static constexpr char charTable[] = "0123456789ABCDEF";
    auto readHead = in.begin();

    // shared args
    unsigned long arg_u = 0;
    long arg_i;

    // temp buffer
    char conversionBuffer[ 33 ], *conversionPointer;
    ulong charsGenerated = 0;  // quicker than strlen

    // i =)
    int i = 0;

    while( readHead < in.end() ) {
        char argType = 0;
        char readVal = *readHead++;

        // next char will determine type
        if( readVal != CHAR_PERCENT ) {
            printer->Print( readVal );
            continue;
        }

        // else read the next char
        argType = *readHead++;

        switch( argType ) {
            // write an actual percent sign
            case '%': printer->Print( CHAR_PERCENT ); break;

            // chaaaar
            case 'c': printer->Print( static_cast<char>( va_arg( list, int ) ) ); break;

            // string
            case 's': printer->Print( va_arg( list, char * ) ); break;

            // string_view
            case 'v': printer->Print( *va_arg( list, std::string_view * ) ); break;

            // Note: i & d drops through to u
            case 'i':
            case 'd':

                arg_i = va_arg( list, long );

                if( arg_i < 0 ) {
                    printer->Print( '-' );
                    arg_u = -arg_i;
                } else {
                    arg_u = arg_i;
                }
            case 'u':

                if( argType == 'u' )
                    arg_u = va_arg( list, unsigned long );

                conversionPointer = conversionBuffer + 32;
                *conversionPointer = 0;
                charsGenerated = 0;

                do {
                    *--conversionPointer = charTable[ arg_u % 10 ];
                    arg_u /= 10;
                    charsGenerated++;
                } while( arg_u );

                printer->Print( { conversionPointer, charsGenerated } );

                break;

            case 'p':
                arg_u = va_arg( list, ulong );
                //*writeHead++ = '0';
                //*writeHead++ = 'x';

                for( i = sizeof( arg_u ) * 2 - 1; i >= 0; i-- ) {
                    printer->Print( charTable[ ( arg_u >> ( i << 2 ) ) & 15 ] );
                }
                break;

            case 'x':

                arg_u = va_arg( list, unsigned long );
                charsGenerated = 0;

                for( i = sizeof( arg_u ) * 2 - 1; i >= 0; i-- ) {
                    if( !charsGenerated && ( ( arg_u >> ( i << 2 ) ) == 0 ) )
                        continue;

                    printer->Print( charTable[ ( arg_u >> ( i << 2 ) ) & 15 ] );
                    charsGenerated = 1;
                }

                if( !charsGenerated ) {
                    printer->Print( '0' );
                }

                break;

            // %02x for the hex editor
            case '0':

                if( *readHead++ != '2' )
                    break;
                if( *readHead++ != 'x' )
                    break;

                arg_u = va_arg( list, unsigned long );

                for( i = sizeof( char ) * 2 - 1; i >= 0; i-- ) {
                    printer->Print( charTable[ ( arg_u >> ( i << 2 ) ) & 15 ] );
                }

                break;
        }
    }
}

void *memset( void *s_, int c, size_t n ) {
    uchar *s = (uchar *)s_;
    size_t i;

    for( i = 0; i < n; i++ )
        *s++ = (uchar)c;

    return s_;
}

size_t strlen( const char *s ) {
    size_t r = 0;

    while( *s++ )
        r++;

    return r;
}
