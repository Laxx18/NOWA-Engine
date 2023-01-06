/***************************************************************************\
|* Function Parser for C++ v4.5.1                                          *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen, Joel Yliluoma                                 *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

// NOTE:
// This file contains only internal types for the function parser library.
// You don't need to include this file in your code. Include "fparser.hh"
// only.

#ifndef ONCE_FPARSER_TYPES_H_
#define ONCE_FPARSER_TYPES_H_

#include "../fpconfig.hh"
#include <cstring>

#ifdef ONCE_FPARSER_H_
#include <map>
#endif

namespace FUNCTIONPARSERTYPES
{
    enum OPCODE
    {
// The order of opcodes in the function list must
// match that which is in the Functions[] array.
        cAbs,
        cAcos, cAcosh,
        cArg,   /* get the phase angle of a complex value */
        cAsin, cAsinh,
        cAtan, cAtan2, cAtanh,
        cCbrt, cCeil,
        cConj,  /* get the complex conjugate of a complex value */
        cCos, cCosh, cCot, cCsc,
        cExp, cExp2, cFloor, cHypot,
        cIf,
        cImag,  /* get imaginary part of a complex value */
        cInt, cLog, cLog10, cLog2, cMax, cMin,
        cPolar, /* create a complex number from polar coordinates */
        cPow,
        cReal,  /* get real part of a complex value */
        cSec, cSin, cSinh, cSqrt, cTan, cTanh,
        cTrunc,

// These do not need any ordering:
// Except that if you change the order of {eq,neq,lt,le,gt,ge}, you
// must also change the order in ConstantFolding_ComparisonOperations().
        cImmed, cJump,
        cNeg, cAdd, cSub, cMul, cDiv, cMod,
        cEqual, cNEqual, cLess, cLessOrEq, cGreater, cGreaterOrEq,
        cNot, cAnd, cOr,
        cNotNot, /* Protects the double-not sequence from optimizations */

        cDeg, cRad, /* Multiplication and division by 180 / pi */

        cFCall, cPCall,

#ifdef FP_SUPPORT_OPTIMIZER
        cPopNMov, /* cPopNMov(x,y) moves [y] to [x] and deletes anything
                   * above [x]. Used for disposing of temporaries.
                   */
        cLog2by, /* log2by(x,y) = log2(x) * y */
        cNop,    /* Used by fpoptimizer internally; should not occur in bytecode */
#endif
        cSinCos,   /* sin(x) followed by cos(x) (two values are pushed to stack) */
        cSinhCosh, /* hyperbolic equivalent of sincos */
        cAbsAnd,    /* As cAnd,       but assume both operands are absolute values */
        cAbsOr,     /* As cOr,        but assume both operands are absolute values */
        cAbsNot,    /* As cAbsNot,    but assume the operand is an absolute value */
        cAbsNotNot, /* As cAbsNotNot, but assume the operand is an absolute value */
        cAbsIf,     /* As cAbsIf,     but assume the 1st operand is an absolute value */

        cDup,   /* Duplicates the last value in the stack: Push [Stacktop] */
        cFetch, /* Same as Dup, except with absolute index
                 * (next value is index) */
        cInv,   /* Inverts the last value in the stack (x = 1/x) */
        cSqr,   /* squares the last operand in the stack, no push/pop */
        cRDiv,  /* reverse division (not x/y, but y/x) */
        cRSub,  /* reverse subtraction (not x-y, but y-x) */
        cRSqrt, /* inverse square-root (1/sqrt(x)) */

        VarBegin
    };

#ifdef ONCE_FPARSER_H_
    struct FuncDefinition
    {
        enum FunctionFlags
        {
            Enabled     = 0x01,
            AngleIn     = 0x02,
            AngleOut    = 0x04,
            OkForInt    = 0x08,
            ComplexOnly = 0x10
        };

// #ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
        const char name[8];
// #else
//         struct name { } name;
// #endif
        unsigned params : 8;
        unsigned flags  : 8;

        inline bool okForInt() const { return (flags & OkForInt) != 0; }
        inline bool complexOnly() const { return (flags & ComplexOnly) != 0; }
    };

// This list must be in the same order as that in OPCODE enum,
// because the opcode value is used to index this array, and
// the pointer to array element is used for generating the opcode.
    const FuncDefinition Functions[]=
    {
        /*cAbs  */ { "abs",   1, FuncDefinition::OkForInt },
        /*cAcos */ { "acos",  1, FuncDefinition::AngleOut },
        /*cAcosh*/ { "acosh", 1, FuncDefinition::AngleOut },
        /*cArg */  { "arg",   1, FuncDefinition::AngleOut | FuncDefinition::ComplexOnly },
        /*cAsin */ { "asin",  1, FuncDefinition::AngleOut },
        /*cAsinh*/ { "asinh", 1, FuncDefinition::AngleOut },
        /*cAtan */ { "atan",  1, FuncDefinition::AngleOut },
        /*cAtan2*/ { "atan2", 2, FuncDefinition::AngleOut },
        /*cAtanh*/ { "atanh", 1, 0 },
        /*cCbrt */ { "cbrt",  1, 0 },
        /*cCeil */ { "ceil",  1, 0 },
        /*cConj */ { "conj",  1, FuncDefinition::ComplexOnly },
        /*cCos  */ { "cos",   1, FuncDefinition::AngleIn },
        /*cCosh */ { "cosh",  1, FuncDefinition::AngleIn },
        /*cCot  */ { "cot",   1, FuncDefinition::AngleIn },
        /*cCsc  */ { "csc",   1, FuncDefinition::AngleIn },
        /*cExp  */ { "exp",   1, 0 },
        /*cExp2 */ { "exp2",  1, 0 },
        /*cFloor*/ { "floor", 1, 0 },
        /*cHypot*/ { "hypot", 2, 0 },
        /*cIf   */ { "if",    0, FuncDefinition::OkForInt },
        /*cImag */ { "imag",  1, FuncDefinition::ComplexOnly },
        /*cInt  */ { "int",   1, 0 },
        /*cLog  */ { "log",   1, 0 },
        /*cLog10*/ { "log10", 1, 0 },
        /*cLog2 */ { "log2",  1, 0 },
        /*cMax  */ { "max",   2, FuncDefinition::OkForInt },
        /*cMin  */ { "min",   2, FuncDefinition::OkForInt },
        /*cPolar */{ "polar", 2, FuncDefinition::ComplexOnly | FuncDefinition::AngleIn },
        /*cPow  */ { "pow",   2, 0 },
        /*cReal */ { "real",  1, FuncDefinition::ComplexOnly },
        /*cSec  */ { "sec",   1, FuncDefinition::AngleIn },
        /*cSin  */ { "sin",   1, FuncDefinition::AngleIn },
        /*cSinh */ { "sinh",  1, FuncDefinition::AngleIn },
        /*cSqrt */ { "sqrt",  1, 0 },
        /*cTan  */ { "tan",   1, FuncDefinition::AngleIn },
        /*cTanh */ { "tanh",  1, FuncDefinition::AngleIn },
        /*cTrunc*/ { "trunc", 1, 0 }
    };
#undef FP_FNAME

    struct NamePtr
    {
        const char* name;
        unsigned nameLength;

        NamePtr(const char* n, unsigned l): name(n), nameLength(l) {}

        inline bool operator==(const NamePtr& rhs) const
        {
            return nameLength == rhs.nameLength
                && std::memcmp(name, rhs.name, nameLength) == 0;
        }
        inline bool operator<(const NamePtr& rhs) const
        {
            for(unsigned i = 0; i < nameLength; ++i)
            {
                if(i == rhs.nameLength) return false;
                const char c1 = name[i], c2 = rhs.name[i];
                if(c1 < c2) return true;
                if(c2 < c1) return false;
            }
            return nameLength < rhs.nameLength;
        }
    };

    template<typename Value_t>
    struct NameData
    {
        enum DataType { CONSTANT, UNIT, FUNC_PTR, PARSER_PTR, VARIABLE };
        DataType type;
        unsigned index;
        Value_t value;

        NameData(DataType t, unsigned v) : type(t), index(v), value() { }
        NameData(DataType t, Value_t v) : type(t), index(), value(v) { }
        NameData() { }
    };

    template<typename Value_t>
    class NamePtrsMap: public
    std::map<FUNCTIONPARSERTYPES::NamePtr,
             FUNCTIONPARSERTYPES::NameData<Value_t> >
    {
    };

    const unsigned FUNC_AMOUNT = sizeof(Functions)/sizeof(Functions[0]);
#endif // ONCE_FPARSER_H_
}

#ifdef ONCE_FPARSER_H_
#include <vector>

template<typename Value_t>
struct FunctionParserBase<Value_t>::Data
{
    unsigned mReferenceCounter;

    char mDelimiterChar;
    ParseErrorType mParseErrorType;
    int mEvalErrorType;
    bool mUseDegreeConversion;
    bool mHasByteCodeFlags;
    const char* mErrorLocation;

    unsigned mVariablesAmount;
    std::string mVariablesString;
    FUNCTIONPARSERTYPES::NamePtrsMap<Value_t> mNamePtrs;

    struct InlineVariable
    {
        FUNCTIONPARSERTYPES::NamePtr mName;
        unsigned mFetchIndex;
    };

    typedef std::vector<InlineVariable> InlineVarNamesContainer;
    InlineVarNamesContainer mInlineVarNames;

    struct FuncWrapperPtrData
    {
        /* Only one of the pointers will point to a function, the other
           will be null. (The raw function pointer could be implemented
           as a FunctionWrapper specialization, but it's done like this
           for efficiency.) */
        FunctionPtr mRawFuncPtr;
        FunctionWrapper* mFuncWrapperPtr;
        unsigned mParams;

        FuncWrapperPtrData();
        ~FuncWrapperPtrData();
        FuncWrapperPtrData(const FuncWrapperPtrData&);
        FuncWrapperPtrData& operator=(const FuncWrapperPtrData&);
    };

    struct FuncParserPtrData
    {
        FunctionParserBase<Value_t>* mParserPtr;
        unsigned mParams;
    };

    std::vector<FuncWrapperPtrData> mFuncPtrs;
    std::vector<FuncParserPtrData> mFuncParsers;

    std::vector<unsigned> mByteCode;
    std::vector<Value_t> mImmed;

#if !defined(FP_USE_THREAD_SAFE_EVAL) && \
    !defined(FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA)
    std::vector<Value_t> mStack;
    // Note: When mStack exists,
    //       mStack.size() and mStackSize are mutually redundant.
#endif

    unsigned mStackSize;

    Data();
    Data(const Data&);
    Data& operator=(const Data&); // not implemented on purpose
    ~Data();
};
#endif

//#include "fpaux.hh"

#endif
