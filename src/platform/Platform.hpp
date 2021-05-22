#ifndef UTIL_PLATFORM_HPP
#define UTIL_PLATFORM_HPP

#if defined(__linux__)
	#include "platform/LinuxPlatform.hpp"
#else
	#error "Unsupported platform"
#endif

namespace util
{
using Platform = LinuxPlatform;
}
#endif // UTIL_PLATFORM_HPP
