#pragma once

#ifdef MAGIC_WINDOWS_BUILD
#define PLATFORM_WINDOWS 1
#define PLATFORM_MACOS 0
#endif

#ifdef MAGIC_MACOS_BUILD
#define PLATFORM_WINDOWS 0
#define PLATFORM_MACOS 1
#endif