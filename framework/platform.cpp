

#if not defined WINDOWS_PLATFORM && not defined LINUX_PLATFORM
    #if defined _MSC_VER
        #define WIN32_LEAN_AND_MEAN 1
        #define VC_EXTRALEAN 1
        #define WINDOWS_PLATFORM 1
        #include <windows.h>
        #include <intrin.h>
    #elif defined __linux
        #define LINUX_PLATFORM 1
        #include <signal.h>
    #endif
#endif

#include "platform.h"
#include <cstdlib>
#include <sstream>
#include <list>
#include <cstring>
#include <iostream>
#include <thread>
#include <SDL.h>

//Linux requires env var to be in static memory
//FIXME: Should remove old entries from list
static std::list<std::string > envvars;

void putEnvironmentVariable(std::string key, std::string value)
{
    #ifdef WINDOWS_PLATFORM
        SetEnvironmentVariableA(key.c_str(),value.c_str());
    #endif
    #ifdef LINUX_PLATFORM
        std::ostringstream oss;
        oss << key << "=" << value;
        std::string s = oss.str();
        envvars.push_back(s);
        putenv(const_cast<char*>(envvars.back().c_str()));
    #endif
}



void invokeDebugger [[noreturn]] ()
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
    #ifdef _MSC_VER
        __debugbreak();
    #else
        raise( SIGTRAP );
    #endif
    abort();
    throw std::runtime_error("This should not happen");
}
