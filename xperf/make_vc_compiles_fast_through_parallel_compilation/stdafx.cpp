// stdafx.cpp : source file that includes just the standard includes

#include "stdafx.h"

#if _MSC_FULL_VER >= 180021114
#pragma message("Compiling with VS 2013 November CTP or higher with constexpr support.")
#else
#pragma message("Compiling with VS 2013 without constexpr support.")
#endif

#ifdef USE_CONST_EXPR
#pragma message("Using constexpr to make compiles slow.")
#else
#pragma message("Using templates to make compiles slow.")
#endif
