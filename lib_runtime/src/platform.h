#define PLATFORM_WIN32 1
#define PLATFORM_LINUX 2
#define PLATFORM_KQUEUE 3

#if defined(_WIN32)
#	define PLATFORM PLATFORM_WIN32
#elif defined(__APPLE_CC__) || defined(BSD)
#	define PLATFORM PLATFORM_KQUEUE
#elif defined(__linux__)
#	define PLATFORM PLATFORM_LINUX
#endif