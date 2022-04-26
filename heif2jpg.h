#pragma once

#include "resource.h"

//http://marupeke296.com/DBG_No3_OutputDebugWindow.html
#define OutputDebugF( str, ... ) \
    { \
    TCHAR c[256]; \
    _stprintf_s( c, 255, str, __VA_ARGS__ ); \
    OutputDebugString( c ); \
    }