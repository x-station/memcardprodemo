#pragma once

#include "littlelibc.h"
#include "utility.h"

class SPI {
  public:
    enum class Action {
        CARDREAD,
        CARDWRITE,
        MCPROSEND,
    };

    // The singleton getter.
    static SPI *Get();

    // Switching between readin/writing/reading requires
    // a slightly longer delay than e.g. just reading or just writing
    // This applies an appropriate delay based on recent reads/writes.
    void ApplyDelay( Action currentAction );
    void SetLastAction( Action inAction );

    // StartComms -> Send Things -> EndComms
    void StartComms( ulong which );
    StatusCode<> SendChunk( unsigned char *inBytes, unsigned char *outBytes, unsigned long length );
    StatusCode<> SendChunkFine(
        unsigned char *inBytes, unsigned char *outBytes, unsigned long length, unsigned long preDelay );
    void EndComms();

    // Timing's tight on pads' Swap()s so while you can
    // absolutely handle this with:
    // SPI::Get()->StartComms() -> SPI::Get()->SendChunk({0x01, 0x42, 0x00, 0x00}) -> SPI::Get()->EndComms()
    // this is finely tuned to match the exact bios/PSYQ
    // timings within a microsecond or two either way.
    unsigned long Read( ulong which, unsigned char *inBuffer, char requestByte, char requestState );

    // For e.g. memcard sector writes where
    // there's sometimes a huge, indeterminate delay
    unsigned long WaitInt( unsigned long inTimeout );
    unsigned long WaitAck( unsigned long inTimeout );

  private:
    Action lastAction = Action::CARDREAD;
    uchar lastByte = 0;
    ulong oldMask = 0;
    bool wasCritical;

    // This inits the front ports for pads, memcards, transfers, etc
    // It's private because the class is a singleton, and you're supposed to use
    // the static Getter to access it.
    void Init();
};
