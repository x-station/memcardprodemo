#include "spi.h"

#include <stdint.h>

#include <optional>

#include "hwregs.h"
#include "sio.h"  // SIO0_MODE/SIO_MODE consts
#include "utility.h"

//
// Constants / defines
//

static constexpr uintptr_t SIO0_TXRX = 0xBF801040;
#define pSIO0_TXRX *(volatile uchar *)SIO0_TXRX
static constexpr uintptr_t SIO0_STAT = 0xBF801044;
#define pSIO0_STAT *(volatile ushort *)SIO0_STAT
static constexpr uintptr_t SIO0_MODE = 0xBF801048;
#define pSIO0_MODE *(volatile ushort *)SIO0_MODE
static constexpr uintptr_t SIO0_CTRL = 0xBF80104A;
#define pSIO0_CTRL *(volatile ushort *)SIO0_CTRL
static constexpr uintptr_t SIO0_BAUD = 0xBF80104E;
#define pSIO0_BAUD *(volatile ushort *)SIO0_BAUD

static constexpr ushort SIO0CTRL_TXENABLE = 0x01;    // bit 0
static constexpr ushort SIO0CTRL_SELECTPORT = 0x02;  // bit 1, select port in bit 13/
static constexpr ushort SIO0CTRL_ACK = 0x10;
static constexpr ushort SIO0CTRL_RESET = 0x40;
static constexpr ushort SIO0CTRL_ACKENABLE = 0x1000;  // bit 12
static constexpr ushort SIO0CTRL_PORT2 = 0x2000;      // set for 2nd port

static constexpr ushort SIO0STAT_TXREADY_FLAG_1 = 0x01;
static constexpr ushort SIO0STAT_RXFIFO_NOT_EMPTY = 0x02;
static constexpr ushort SIO0STAT_TX_FINISHED = 0x04;
static constexpr ushort SIO0STAT_IRQ = 0x200;

static constexpr int SPI_SHORT_DELAY = 0x3000;
static constexpr int SPI_LONG_DELAY = 0x4000;  // 0x4000 will work, 0x3A00 will not

//
// Our global singleton
//

static SPI spi;

SPI *SPI::Get() {
    spi.Init();
    return &spi;
}

//
// Stuff
//

// PadStop: fixed analogue issues when returning from Driver/Gran Turismo 2
static void PadStop() {

    register int cmd2 __asm__( "t1" ) = 0x14;
    __asm__ volatile( "" : "=r"( cmd2 ) : "r"( cmd2 ) );
    ( (void ( * )( void ))0xB0 )();
}

// heh, you really don't want to mess with the timing here
// it's been timed to within a microsecond or so to maximise compatibility
#pragma GCC push options
#pragma GCC optimize( "-O0" )
static void SPIDelay( int inLen ) {
    int i = 0;
    for( i = 0; i < inLen; i++ ) {
        __asm__ volatile( "" : "=r"( i ) : "r"( i ) );
    }
}
#pragma GCC pop options

// Clear the SIO0' interrupt flag
static void SIO0ClearInt() { pISTAT &= ( ~0x80 ); }

// Sends a byte to SIO0 device and gets one back in exchange
static std::optional<uchar> Swap( uchar inCommand, ulong inDelay ) {

    pSIO0_TXRX = inCommand;  // fifo = 0

    SPIDelay( inDelay );

    pSIO0_CTRL |= SIO0CTRL_ACK;  // Ctrl | 0x10;

    SIO0ClearInt();  // for the NEXT Swap() call

    int timeout = 80;
    while( ( pSIO0_STAT & SIO0STAT_RXFIFO_NOT_EMPTY ) == 0 ) {
        if( timeout-- <= 0 ) {
            NewPrintf( "Swap Failed\n" );
            return std::nullopt;
        }
    }

    return pSIO0_TXRX;
}

void SPI::Init() {

    static bool didInit = false;
    if( didInit )
        return;

    didInit = true;

    // Fix GT2 and Driver
    PadStop();

    pSIO0_CTRL = SIO0CTRL_RESET;        // (0x40)
    pSIO0_BAUD = 0x88;                  // 250khz, default value
    pSIO0_MODE = MR_CHLEN_8 | MR_BR_1;  // 8 bit, 1x Mul (0xD)

    SPIDelay( 10 );

    pSIO0_CTRL = SIO0CTRL_SELECTPORT;  // 0x02

    SPIDelay( 10 );

    pSIO0_CTRL = SIO0CTRL_SELECTPORT | SIO0CTRL_PORT2;  // 0x2002

    SPIDelay( 10 );

    pSIO0_CTRL = 0x0;

    SPIDelay( 1500 );

    /*
    int timeout = 80;
    while( ( pSIO0_STAT & SIO0STAT_RXFIFO_NOT_EMPTY ) != 0 ){
        if ( timeout-- <= 0 ) break;
    }
    */
}

// Wait for the SIO0' interrupt flag
// This is passive and doesn't require any kind of driver
// on the kernel end of things
ulong SPI::WaitInt( ulong inTimeout ) {

    ulong timeout = inTimeout;
    while( ( pISTAT & 0x80 ) == 0 ) {
        if( timeout-- <= 0 ) {
            return 0;
        }
    }

    return 1;
}

// Similar to SPI::WaitInt, but on the SIO0' HW regs
ulong SPI::WaitAck( ulong inTimeout ) {

    ulong waitabix = 0;
    while( ( pSIO0_STAT & 0x200 ) == 0 ) {
        // do a thing?
        if( waitabix++ >= inTimeout ) {
            return 0;
        }
    }

    return 1;
}

void SPI::StartComms( unsigned long which ) {

    // store the old SIO0 mask and enable SIO0 ints
    oldMask = pIMASK;

    wasCritical = EnterCritical();

    pIMASK |= 0x80;

    ulong padSelector = ( which * 0x2000 );

    // Select the correct SIO0 device
    pSIO0_CTRL = padSelector | SIO0CTRL_SELECTPORT;  // which | 2

    // About 38 microseconds from SEL going low.
    // Kernel varies ~1 bit either side
    SPIDelay( 0x11 );

    // Kernel likes to do this separately
    ulong selectEnable = SIO0CTRL_TXENABLE | SIO0CTRL_SELECTPORT | SIO0CTRL_ACKENABLE;
    pSIO0_CTRL = padSelector | selectEnable;  // padSelector | 0x1003

    int timeout = 80;
    while( ( pSIO0_STAT & SIO0STAT_TXREADY_FLAG_1 ) == 0 ) {  // STAT & 1 == 0
        if( timeout-- <= 0 )
            break;
    }

    lastByte = 0;
}

void SPI::EndComms() {

    // if SIO0 ints weren't enabled, turn them back off
    if( ( oldMask & 0x80 ) == 0 )
        pIMASK &= ~0x80;

    // returns SEL hi
    pSIO0_CTRL = 0;

    Delay( 400 );

    /*
    // not necessary, but 175ms after SEL goes high
    // if there's no 2nd SIO0 device, the BIOS does this little
    // hiccup for 7 microseconds
    Delay( 126 );
    pSIO0_CTRL = padSelector | SIO0CTRL_SELECTPORT;  // which | 2
    Delay( 7 );
    pSIO0_CTRL = 0;
    */

    if( !wasCritical )
        ExitCritical();
}

StatusCode<> SPI::SendChunkFine( uchar *inBytes, uchar *outBytes, ulong length, ulong preDelay ) {

    // more standard delay than the usual SIO0 read routine
    // After the first SPI transfer, there's a slightly longer delay than the rest
    // 21us for pads, 66us for cards (give or take)

    for( int i = 0; i < length; i++ ) {
        auto ret = Swap( *inBytes++, preDelay );
        if( !ret.has_value() )
            return { Status::UNKNOWN };
        outBytes[ i ] = lastByte = ret.value();
        WaitAck( 0x600 );
    }

    return { length };
}

StatusCode<> SPI::SendChunk( uchar *inBytes, uchar *outBytes, ulong length ) {

    // more standard delay than the usual SPI read routine
    ulong preDelay = 0x19;

    for( int i = 0; i < length; i++ ) {
        auto ret = Swap( *inBytes++, preDelay );
        if( !ret.has_value() )
            return { Status::UNKNOWN };
        outBytes[ i ] = lastByte = ret.value();
        WaitAck( 0x100 );
    }

    return { length };
}

// Cards can be pushed pretty fast when doing continuous, sequential
// reads/writes. But when switching, they need slightly longer to
// finish up. So - slightly longer delay if switching between read & write
// Doesn't seem to interfere with pads as they use a different device ID
void SPI::SetLastAction( Action inAction ) { lastAction = inAction; }

void SPI::ApplyDelay( Action currentAction ) {

    if( currentAction != lastAction ) {
        NewPrintf( "Action changed from %d to %d\n", lastAction, currentAction );
        Delay( SPI_LONG_DELAY );
    }

    // Delay( currentAction == SPI_ACTION_CARDWRITE ? CARD_LONG_DELAY :
    // CARD_SMALL_DELAY );
    if( currentAction == Action::CARDWRITE ) {
        Delay( SPI_LONG_DELAY );
    } else {
        Delay( SPI_SHORT_DELAY );
    }

    SPI::SetLastAction( currentAction );
}

// Rough timings from the bios/shell
//
// Sel to first clock pulse...
// 35us
// 10us to ack, 23us to next clock
// 12us to ack, 5us to next clock
// 8us to ack, 5us to next clock
// 7us to ack, 6us to next clock
//

// Pads tested:
// 1080A (digital)
// 1080H (digital)
// 1080M (digital)

// 1200A (ds)
// 1200M (ds)
// 1200H (ds)

// 10010A (ds2)
// 10010H (ds2)

// 110H (psone ds)

// NegCon, GCon45, Maus, etc

// Pads can be read relatively easily with a simple
// SPI::Get()->StartComms -> SPI::Get()->SendChunk (with constant timings) -> SPI::Get()->EndComms
// but since compatibility is fiddly across different models
// and timing's tight, here's a decent recreation of
// the bios/PSYQ's pad implementation.

// pad ID 0 indexed
// buffer is 0x10 length max
// requestByte is 0x42 (get inputs) by default)
// requestState is normally 0, but 1 for "set led", "set config mode", etc

ulong SPI::Read( ulong which, uchar *outBytes, char requestByte, char requestState ) {
    auto res = [ which, outBytes, requestByte, requestState, this ]() mutable -> std::optional<ulong> {
        ulong bytesRead = 0;
        ulong bytesToRead = 0;

        SPI::Get()->StartComms( which );

        // From 7k bios, no wait for int beforehand
        // Send: 0x01 Recv: 0xFF
        // this one isn't included in the byte count
        {
            auto b = Swap( 0x01, 0x00 );
            if( !b.has_value() )
                return std::nullopt;

            lastByte = b.value();

            //*outBytes++ = lastByte;  //<-- this one doesn't contrib to byte count

            // usually ~10us till Ack pulse happens
            // (already happened by this point)

            // 24us from ack to next Swap
            SPIDelay( 0x10 );
            if( SPI::Get()->WaitAck( 0x50 ) ) {
                // bytesRead++;
            } else {
                return std::nullopt;
            }
        }

        // Send: 0x42
        // Recv: 0x41 digi, 0x73 anal
        {
            auto b = Swap( requestByte, 0x00 );  // also works
            if( !b.has_value() )
                return std::nullopt;

            *outBytes++ = lastByte = b.value();

            // usually ~12us till Ack pulse happens
            // (already happened by this point)

            // 12us to next swap
            SPIDelay( 0x0A );
            if( SPI::Get()->WaitAck( 0x50 ) ) {
                bytesRead++;
            } else {
                return std::nullopt;
            }

            // bytesToRead = length - 3;
            bytesToRead = lastByte & 0xF;
            if( bytesToRead == 0 ) {
                bytesToRead = 32;
            } else {
                bytesToRead = bytesToRead << 1;  // halfwords to bytes
            }

            bytesToRead += 2;  // counting the two extra bytes for ID we just read
        }

        // Send: 0x00 for pads, 0x01 for multitap
        // Recv: 0x5A (or e.g. 0xFF, 0x00)
        {
            auto b = Swap( 0x00, 0x14 );
            if( !b.has_value() )
                return std::nullopt;

            *outBytes++ = lastByte = b.value();

            // usually ~12us till Ack pulse happens
            // (already happened by this point)

            // 7-8 us to next swap
            SPIDelay( 0x09 );
            if( SPI::Get()->WaitAck( 0x50 ) ) {
                bytesRead++;
            } else {
                return std::nullopt;
            }
        }

        // 0x5A = normal
        // 0x00 = config mode or brook adapter 'stuck'
        if( lastByte != 0x5A && lastByte != 0x00 ) {
            return std::nullopt;
        }

        // 4 bytes includes: 0x42, 0x5A, 0xVAL1, 0xVAL2
        if( bytesToRead < 4 )
            return std::nullopt;

        // We've already got 2
        while( bytesRead < bytesToRead ) {

            auto b = Swap( requestByte, 0x00 );
            if( !b.has_value() )
                return std::nullopt;

            *outBytes++ = lastByte = b.value();

            SPIDelay( 0x09 );

            // Don't ack the last one
            if( bytesRead == ( bytesToRead - 1 ) ) {
                break;
            }

            // Ack these ones
            if( SPI::Get()->WaitAck( 0x50 ) ) {
                bytesRead++;
            } else {
                return std::nullopt;
            }
        }

        return bytesRead + 1;  // we skipped the last ack
    }();

    const bool success = res.has_value();

    SPI::Get()->EndComms();

    if( success ) {
        // NewPrintf( "Pad_%x_ %x   %x   BR=%x\n", which,
        // *(ulong*)originalBuffer, *(ulong*)(originalBuffer+4), bytesRead );
    } else {
        // NewPrintf( "FAIL_%x_ %x   %x   BR=%x\n", which,
        // *(ulong*)originalBuffer, *(ulong*)(originalBuffer+4), bytesRead );
        for( int i = 0; i < PADBUFFERSIZE; i++ ) {
            *outBytes++ = 0xFF;
        }
    }

    return res.value_or( 0 );
}
