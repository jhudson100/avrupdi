#pragma once

#define PLATFORM_WIN32 1
#define PLATFORM_LINUX 2
#define PLATFORM_MAC   3


#ifndef PLATFORM
    #ifdef WIN32
        #define PLATFORM PLATFORM_WIN32
    #endif

    #ifdef __linux__
        #define PLATFORM PLATFORM_LINUX
    #endif

    // #ifdef __APPLE__
    //     #define PLATFORM PLATFORM_MAC
    // #endif
#endif
