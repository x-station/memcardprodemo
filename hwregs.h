// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Shared by the CD and IRQ handlers
#define CDREG0 0xBF801800
#define pCDREG0 *(volatile uchar *)CDREG0

#define CDREG1 0xBF801801
#define pCDREG1 *(volatile uchar *)CDREG1

#define CDREG2 0xBF801802
#define pCDREG2 *(volatile uchar *)CDREG2

#define CDREG3 0xBF801803
#define pCDREG3 *(volatile uchar *)CDREG3

#define CDISTAT 0xBF801070
#define pCDISTAT *(volatile ushort *)CDISTAT

#define ISTAT 0xBF801070
#define pISTAT *(volatile ulong *)ISTAT

#define IMASK 0xBF801074
#define pIMASK *(volatile ulong *)IMASK

//  ROOT COUNTERS

//     0x1f801100 - t0_count
//     0x1f801104 - t0_mode
//     0x1f801108 - t0_target
//     0x1f801110 - t1_count
//     0x1f801114 - t1_mode
//     0x1f801118 - t1_target
//     0x1f801120 - t2_count
//     0x1f801124 - t2_mode
//     0x1f801128 - t2_target
//     0x1f801130 - t3_count?
//     0x1f801134 - t3_mode?
//     0x1f801138 - t3_target?
#define TIMER1 0xBF801110
#define pTIMER1 *(volatile ushort *)TIMER1

#define TIMER1_MODE 0xBF801114
#define pTIMER1_MODE *(volatile ushort *)TIMER1_MODE

// Const memory locs
//
// Some things are written to very specific places in memory
// mostly for debugging
// these are those things

#define SIO_SPAD 0x1F800000
#define pSPAD *(ulong *)SIO_SPAD

// 0x10 bytes into the scratch pad
// anywhere's good, really, but it's 0x08 long

// 0x22 bytes into the scratch pad
// anywhere's good, really, but it's maximum 0x22 long
#define PADBUFFERSIZE 0x22

#define PAD1BUFFER 0x1F800024
#define pPAD1BUFFER (uchar *)PAD1BUFFER

#define PAD2BUFFER 0x1F800048
#define pPAD2BUFFER (uchar *)PAD2BUFFER

// TODO: stop using it.
// Generic raw buffer for debugging
#define PAD3BUFFER 0x1F80006C
#define pPAD3BUFFER (uchar *)PAD3BUFFER

#define CDSTACKBUFFER 0x1F800080

// 0x100 into the scratch pad
// for e.g. filename + name parsing/modification
// max len 30 (since that's all the shell can handle)
// so give it size 0x20 (32d)
#define CDFILENAMEBUFFER 0x1F800100
