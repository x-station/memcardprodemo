// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "memcardpro.h"

#include "spi.h"

bool MCPro::IsConnected( ulong which ) {

    union {
        uchar buf[ 4 ];
        ulong value;
    } response;

    uchar header[] = { 0x81, 0x20, 0x00, 0x00 };
    uchar bytes[] = { 0x00, 0x00, 0x00, 0x00 };

    SPI::Get()->StartComms( which );
    auto res = SPI::Get()->SendChunk( header, response.buf, 4 );
    SPI::Get()->EndComms();

    // Expected responses:
    // 0xFF, 0x08, 0x00, 0x00
    // 0xFF, 0x00, 0x00, 0x00
    // just read a ulong out of the buffer...

    return res.Success() && ( ( response.value == 0x000008FF ) || ( response.value == 0x000000FF ) );
}

void MCPro::SendFilename( std::string_view inName, ulong which ) {

    // Send the binary path to memcard pro
    // 2 bytes for header (0x81, 0x21)
    // 1 reserved byte
    // 1 string length byte
    // 128 bytes for exe filename (same size bios allocs)
    // 1 0x00 terminator
    uchar responseBuffer[ 128 + 4 ];

    // char wasCritical = EnterCritical();
    // char wasDebugging = EnablePadDebug( 1 );

    ulong blen = inName.length() + 1;  // need our \0
    NewPrintf( "strlen is %x\n", blen );

    // what if blen is > 255 ?
    uchar header[] = { 0x81, 0x21, 0x00, (uchar)blen };

    SPI::Get()->SetLastAction( SPI::Action::MCPROSEND );  // TODO: ?
    SPI::Get()->StartComms( which );
    auto res = SPI::Get()->SendChunk( header, responseBuffer, 4 );
    if( res.Success() ) {
        // It's okay to ignore the results of this one, because we're not going
        // to action on it if anything went wrong.
        res = SPI::Get()->SendChunk( (uchar *)inName.data(), responseBuffer, blen );
    }
    SPI::Get()->EndComms();

    // if( !wasDebugging )
    //     EnablePadDebug( 0 );
    // if( !wasCritical )
    //     ExitCritical();
}
