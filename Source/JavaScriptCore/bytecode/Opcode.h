/*
 * Copyright (C) 2008, 2009, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Bytecodes.h"
#include "LLIntOpcode.h"

#include <algorithm>
#include <string.h>

#include <wtf/Assertions.h>

namespace JSC {

#define FOR_EACH_CORE_OPCODE_ID_WITH_EXTENSION(macro, extension__) \
    FOR_EACH_BYTECODE_ID(macro) \
    extension__

#define FOR_EACH_CORE_OPCODE_ID(macro) \
    FOR_EACH_CORE_OPCODE_ID_WITH_EXTENSION(macro, /* No extension */ )

#define FOR_EACH_OPCODE_ID(macro) \
    FOR_EACH_CORE_OPCODE_ID_WITH_EXTENSION( \
        macro, \
        FOR_EACH_LLINT_OPCODE_EXTENSION(macro) \
    )


const int maxOpcodeLength = 40;
#if ENABLE(C_LOOP)
const int numOpcodeIDs = NUMBER_OF_BYTECODE_IDS + NUMBER_OF_CLOOP_BYTECODE_HELPER_IDS + NUMBER_OF_BYTECODE_HELPER_IDS;
#else
const int numOpcodeIDs = NUMBER_OF_BYTECODE_IDS + NUMBER_OF_BYTECODE_HELPER_IDS;
#endif

#define OPCODE_ID_ENUM(opcode, length) opcode,
    enum OpcodeID : unsigned { FOR_EACH_OPCODE_ID(OPCODE_ID_ENUM) };
#undef OPCODE_ID_ENUM

#if ENABLE(C_LOOP) && !HAVE(COMPUTED_GOTO)

#define OPCODE_ID_ENUM(opcode, length) opcode##_wide = numOpcodeIDs + opcode,
    enum OpcodeIDWide : unsigned { FOR_EACH_OPCODE_ID(OPCODE_ID_ENUM) };
#undef OPCODE_ID_ENUM
#endif

IGNORE_WARNINGS_BEGIN("unused-variable")

#define OPCODE_LENGTH(opcode, length) length,
    static unsigned opcodeLengths[] = {
        FOR_EACH_OPCODE_ID(OPCODE_LENGTH)
    };
#undef OPCODE_LENGTH

IGNORE_WARNINGS_END

#define OPCODE_ID_LENGTHS(id, length) const int id##_length = length;
    FOR_EACH_OPCODE_ID(OPCODE_ID_LENGTHS);
#undef OPCODE_ID_LENGTHS

#define FOR_EACH_OPCODE_WITH_VALUE_PROFILE(macro) \
    macro(OpCallVarargs) \
    macro(OpTailCallVarargs) \
    macro(OpTailCallForwardArguments) \
    macro(OpConstructVarargs) \
    macro(OpGetByVal) \
    macro(OpGetDirectPname) \
    macro(OpGetById) \
    macro(OpGetByIdWithThis) \
    macro(OpTryGetById) \
    macro(OpGetByIdDirect) \
    macro(OpGetByValWithThis) \
    macro(OpGetFromArguments) \
    macro(OpToNumber) \
    macro(OpToObject) \
    macro(OpGetArgument) \
    macro(OpToThis) \
    macro(OpCall) \
    macro(OpTailCall) \
    macro(OpCallEval) \
    macro(OpConstruct) \
    macro(OpGetFromScope) \
    macro(OpBitand) \
    macro(OpBitor) \
    macro(OpBitnot) \

#define FOR_EACH_OPCODE_WITH_ARRAY_PROFILE(macro) \
    macro(OpHasIndexedProperty) \
    macro(OpCallVarargs) \
    macro(OpTailCallVarargs) \
    macro(OpTailCallForwardArguments) \
    macro(OpConstructVarargs) \
    macro(OpGetByVal) \
    macro(OpCall) \
    macro(OpTailCall) \
    macro(OpCallEval) \
    macro(OpConstruct) \
    macro(OpInByVal) \
    macro(OpPutByVal) \
    macro(OpPutByValDirect) \

#define FOR_EACH_OPCODE_WITH_ARRAY_ALLOCATION_PROFILE(macro) \
    macro(OpNewArray) \
    macro(OpNewArrayWithSize) \
    macro(OpNewArrayBuffer) \

#define FOR_EACH_OPCODE_WITH_OBJECT_ALLOCATION_PROFILE(macro) \
    macro(OpNewObject) \

#define FOR_EACH_OPCODE_WITH_LLINT_CALL_LINK_INFO(macro) \
    macro(OpCall) \
    macro(OpTailCall) \
    macro(OpCallEval) \
    macro(OpConstruct) \

IGNORE_WARNINGS_BEGIN("type-limits")

#define VERIFY_OPCODE_ID(id, size) COMPILE_ASSERT(id <= numOpcodeIDs, ASSERT_THAT_JS_OPCODE_IDS_ARE_VALID);
    FOR_EACH_OPCODE_ID(VERIFY_OPCODE_ID);
#undef VERIFY_OPCODE_ID

IGNORE_WARNINGS_END

#if ENABLE(COMPUTED_GOTO_OPCODES)
typedef void* Opcode;
#else
typedef OpcodeID Opcode;
#endif

#define PADDING_STRING "                                "
#define PADDING_STRING_LENGTH static_cast<unsigned>(strlen(PADDING_STRING))

extern const char* const opcodeNames[];

inline const char* padOpcodeName(OpcodeID op, unsigned width)
{
    unsigned pad = width - strlen(opcodeNames[op]);
    pad = std::min(pad, PADDING_STRING_LENGTH);
    return PADDING_STRING + PADDING_STRING_LENGTH - pad;
}

#undef PADDING_STRING_LENGTH
#undef PADDING_STRING

#if ENABLE(OPCODE_STATS)

struct OpcodeStats {
    OpcodeStats();
    ~OpcodeStats();
    static long long opcodeCounts[numOpcodeIDs];
    static long long opcodePairCounts[numOpcodeIDs][numOpcodeIDs];
    static int lastOpcode;
    
    static void recordInstruction(int opcode);
    static void resetLastInstruction();
};

#endif

inline bool isBranch(OpcodeID opcodeID)
{
    switch (opcodeID) {
    case op_jmp:
    case op_jtrue:
    case op_jfalse:
    case op_jeq_null:
    case op_jneq_null:
    case op_jneq_ptr:
    case op_jless:
    case op_jlesseq:
    case op_jgreater:
    case op_jgreatereq:
    case op_jnless:
    case op_jnlesseq:
    case op_jngreater:
    case op_jngreatereq:
    case op_jeq:
    case op_jneq:
    case op_jstricteq:
    case op_jnstricteq:
    case op_jbelow:
    case op_jbeloweq:
    case op_switch_imm:
    case op_switch_char:
    case op_switch_string:
        return true;
    default:
        return false;
    }
}

inline bool isUnconditionalBranch(OpcodeID opcodeID)
{
    switch (opcodeID) {
    case op_jmp:
        return true;
    default:
        return false;
    }
}

inline bool isTerminal(OpcodeID opcodeID)
{
    switch (opcodeID) {
    case op_ret:
    case op_end:
    case op_unreachable:
        return true;
    default:
        return false;
    }
}

inline bool isThrow(OpcodeID opcodeID)
{
    switch (opcodeID) {
    case op_throw:
    case op_throw_static_error:
        return true;
    default:
        return false;
    }
}

unsigned metadataSize(OpcodeID);
#if CPU(NEEDS_ALIGNED_ACCESS)
unsigned metadataAlignment(OpcodeID);
#endif

} // namespace JSC

namespace WTF {

class PrintStream;

void printInternal(PrintStream&, JSC::OpcodeID);

} // namespace WTF
