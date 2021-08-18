// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

//
// Various small utility functions
//
// e.g. is it a standalone .exe, from a ROM, is caetla installed, etc.
//

#pragma once

#include <string_view>

#include "littlelibc.h"

enum class Status {
    NO_ERROR = 0,
    UNKNOWN,
    TIMEOUT,
    INVALID_ARGUMENT,
};

// Can be used to return either an error, or a valid value.
// Either use StatusCode<> for the default ulong type, or
// use a different type. Construction can be done directly
// from return statements using bracket-construction.
template<typename CodeType = ulong> struct [[nodiscard]] StatusCode {
    StatusCode() : status_( Status::NO_ERROR ), code_( CodeType() ) {
        static_assert( sizeof( decltype( *this ) ) == ( 2 * sizeof( int ) ) );
    }
    StatusCode( Status status ) : status_( status ), code_( CodeType() ) {}
    StatusCode( CodeType code ) : status_( Status::NO_ERROR ), code_( code ) {}
    StatusCode( Status status, CodeType code ) : status_( status ), code_( code ) {}
    bool Success() { return status_ == Status::NO_ERROR; }

    Status GetStatus() { return status_; }
    CodeType Code() { return code_; }

  private:
    Status status_;
    CodeType code_;
};

// Get int enable state from cop0r12
static inline bool InCriticalSection() {
    ulong returnVal;
    __asm__ volatile(
        "mfc0 %0,$12\n\t"
        "nop\n\t"
        : "=r"( returnVal )
        :  // no inputs
    );

    return !( returnVal & 0x01 );
}

// Enter a critical section by disabling interrupts
static inline bool EnterCritical() {
    bool oldVal = InCriticalSection();

    __asm__ volatile(
        "li   $9, 0x1\n\t"   // li t1,0x01
        "not  $9\n\t"        // not t1
        "mfc0 $8, $12\n\t"   // mfc0 t0,$12
        "nop  \n\t"          // best opcode
        "and  $8,$8,$9\n\t"  // t0 = t0 & t1 (mask it)
        "mtc0 $8, $12\n\t"   // send it back
        "nop"
        :  // no outputs
        :  // no inputs
        : "$8", "$9" );

    return oldVal;
}

// Exit critical by re-enabling interrupts
static inline void ExitCritical() {
    __asm__ volatile(
        "mfc0 $8, $12\n\t"
        "nop  \n\t"
        "ori  $8,$8,0x01\n\t"
        "ori  $8,$8,0xFF00\n\t"  // allow all the int types, master mask since pad
                                 // init isn't setting this
        "mtc0 $8, $12\n\t"
        "nop"
        :  //
        :  //
        : "$8" );
}

void ResetGraph();
void InitHeap( unsigned long *a, unsigned long b );
int StopCallback( void );

unsigned long ResetEntryInt();

void Delay( int inLen );

int IsROM();

int HasCaetla();
// void FastBoot();

void Reboot();

void UnloadMe();

int RebootToShell();

// Some kernel functions for the TTY redirect

void AddDevice( void *deviceInfo );
void RemoveDevice( const std::string_view deviceName );
void PrintDevices();
void CloseFile( ulong fileHandle );
ulong OpenFile( const std::string_view fileName, ulong accessMode );

void FlushCache();
ulong GetB0Table();

// Grab/everything/after/the/lastSlash/pickles.jpg -> pickles.jpg
const char *GetFilenameFromFullPath( const char *inPath );
const char *GetTruncatedFilenameFromEnd( const char *inPath, int maxLength );