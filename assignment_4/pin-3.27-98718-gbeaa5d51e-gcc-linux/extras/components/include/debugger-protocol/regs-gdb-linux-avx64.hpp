/*
 * Copyright (C) 2010-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

// <COMPONENT>: debugger-protocol
// <FILE-TYPE>: component public header

#ifndef DEBUGGER_PROTOCOL_REGS_GDB_LINUX_AVX64_HPP
#define DEBUGGER_PROTOCOL_REGS_GDB_LINUX_AVX64_HPP

#include "debugger-protocol.hpp"

namespace DEBUGGER_PROTOCOL
{
#if defined(DEBUGGER_PROTOCOL_BUILD) // Library clients should NOT define this.

/*!
 * This is the register set used by GDB for 64-bit AVX on Linux.
 */
DEBUGGER_PROTOCOL_API REG_DESCRIPTION RegsGdbLinuxAvx64[] = {
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_RAX
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_RBX
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_RCX
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_RDX
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_RSI
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_RDI
    {64, REG_INVALID, true},   // REG_GDB_LINUX_AVX64_RBP
    {64, REG_INVALID, true},   // REG_GDB_LINUX_AVX64_RSP
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R8
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R9
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R10
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R11
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R12
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R13
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R14
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_R15
    {64, REG_PC, true},        // REG_GDB_LINUX_AVX64_PC
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_EFLAGS
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_CS
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_SS
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_DS
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ES
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FS
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_GS
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST0
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST1
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST2
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST3
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST4
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST5
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST6
    {80, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ST7
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FCTRL
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FSTAT
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FTAG_FULL
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FISEG
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FIOFF
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FOSEG
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FOOFF
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_FOP
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM0
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM1
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM2
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM3
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM4
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM5
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM6
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM7
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM8
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM9
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM10
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM11
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM12
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM13
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM14
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_XMM15
    {32, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_MXCSR
    {64, REG_INVALID, false},  // REG_GDB_LINUX_AVX64_ORIG_RAX
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM0H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM1H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM2H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM3H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM4H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM5H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM6H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM7H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM8H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM9H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM10H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM11H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM12H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM13H
    {128, REG_INVALID, false}, // REG_GDB_LINUX_AVX64_YMM14H
    {128, REG_INVALID, false}  // REG_GDB_LINUX_AVX64_YMM15H
};

/*!
 * Number of entries in RegsGdbLinuxAvx64.
 */
DEBUGGER_PROTOCOL_API unsigned RegsGdbLinuxAvx64Count = sizeof(RegsGdbLinuxAvx64) / sizeof(RegsGdbLinuxAvx64[0]);

#else

DEBUGGER_PROTOCOL_API extern REG_DESCRIPTION RegsGdbLinuxAvx64[]; ///< GDB's 64-bit AVX register set on Linux.
DEBUGGER_PROTOCOL_API extern unsigned RegsGdbLinuxAvx64Count;     ///< Number of entries in RegsGdbLinuxAvx64.

#endif /*DEBUGGER_PROTOCOL_BUILD*/

/*!
 * Convenient identifiers for the registers in this set.
 */
enum REG_GDB_LINUX_AVX64
{
    REG_GDB_LINUX_AVX64_FIRST = REG_END,
    REG_GDB_LINUX_AVX64_RAX   = REG_GDB_LINUX_AVX64_FIRST,
    REG_GDB_LINUX_AVX64_RBX,
    REG_GDB_LINUX_AVX64_RCX,
    REG_GDB_LINUX_AVX64_RDX,
    REG_GDB_LINUX_AVX64_RSI,
    REG_GDB_LINUX_AVX64_RDI,
    REG_GDB_LINUX_AVX64_RBP,
    REG_GDB_LINUX_AVX64_RSP,
    REG_GDB_LINUX_AVX64_R8,
    REG_GDB_LINUX_AVX64_R9,
    REG_GDB_LINUX_AVX64_R10,
    REG_GDB_LINUX_AVX64_R11,
    REG_GDB_LINUX_AVX64_R12,
    REG_GDB_LINUX_AVX64_R13,
    REG_GDB_LINUX_AVX64_R14,
    REG_GDB_LINUX_AVX64_R15,
    REG_GDB_LINUX_AVX64_PC,
    REG_GDB_LINUX_AVX64_EFLAGS,
    REG_GDB_LINUX_AVX64_CS,
    REG_GDB_LINUX_AVX64_SS,
    REG_GDB_LINUX_AVX64_DS,
    REG_GDB_LINUX_AVX64_ES,
    REG_GDB_LINUX_AVX64_FS,
    REG_GDB_LINUX_AVX64_GS,
    REG_GDB_LINUX_AVX64_ST0,
    REG_GDB_LINUX_AVX64_ST1,
    REG_GDB_LINUX_AVX64_ST2,
    REG_GDB_LINUX_AVX64_ST3,
    REG_GDB_LINUX_AVX64_ST4,
    REG_GDB_LINUX_AVX64_ST5,
    REG_GDB_LINUX_AVX64_ST6,
    REG_GDB_LINUX_AVX64_ST7,
    REG_GDB_LINUX_AVX64_FCTRL,
    REG_GDB_LINUX_AVX64_FSTAT,
    REG_GDB_LINUX_AVX64_FTAG_FULL, // 16-bit "full" encoding
    REG_GDB_LINUX_AVX64_FISEG,
    REG_GDB_LINUX_AVX64_FIOFF,
    REG_GDB_LINUX_AVX64_FOSEG,
    REG_GDB_LINUX_AVX64_FOOFF,
    REG_GDB_LINUX_AVX64_FOP,
    REG_GDB_LINUX_AVX64_XMM0,
    REG_GDB_LINUX_AVX64_XMM1,
    REG_GDB_LINUX_AVX64_XMM2,
    REG_GDB_LINUX_AVX64_XMM3,
    REG_GDB_LINUX_AVX64_XMM4,
    REG_GDB_LINUX_AVX64_XMM5,
    REG_GDB_LINUX_AVX64_XMM6,
    REG_GDB_LINUX_AVX64_XMM7,
    REG_GDB_LINUX_AVX64_XMM8,
    REG_GDB_LINUX_AVX64_XMM9,
    REG_GDB_LINUX_AVX64_XMM10,
    REG_GDB_LINUX_AVX64_XMM11,
    REG_GDB_LINUX_AVX64_XMM12,
    REG_GDB_LINUX_AVX64_XMM13,
    REG_GDB_LINUX_AVX64_XMM14,
    REG_GDB_LINUX_AVX64_XMM15,
    REG_GDB_LINUX_AVX64_MXCSR,
    REG_GDB_LINUX_AVX64_ORIG_RAX,
    REG_GDB_LINUX_AVX64_YMM0H,
    REG_GDB_LINUX_AVX64_YMM1H,
    REG_GDB_LINUX_AVX64_YMM2H,
    REG_GDB_LINUX_AVX64_YMM3H,
    REG_GDB_LINUX_AVX64_YMM4H,
    REG_GDB_LINUX_AVX64_YMM5H,
    REG_GDB_LINUX_AVX64_YMM6H,
    REG_GDB_LINUX_AVX64_YMM7H,
    REG_GDB_LINUX_AVX64_YMM8H,
    REG_GDB_LINUX_AVX64_YMM9H,
    REG_GDB_LINUX_AVX64_YMM10H,
    REG_GDB_LINUX_AVX64_YMM11H,
    REG_GDB_LINUX_AVX64_YMM12H,
    REG_GDB_LINUX_AVX64_YMM13H,
    REG_GDB_LINUX_AVX64_YMM14H,
    REG_GDB_LINUX_AVX64_YMM15H,
    REG_GDB_LINUX_AVX64_LAST = REG_GDB_LINUX_AVX64_YMM15H
};

} // namespace DEBUGGER_PROTOCOL
#endif // file guard