// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#if _MSC_FULL_VER == 180021114
// VS 2013 CTP -- includes support for constexpr
#define USE_CONST_EXPR
#endif

#include <stdio.h>
#include "fib.h"
