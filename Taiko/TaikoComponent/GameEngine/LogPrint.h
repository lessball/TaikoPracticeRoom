#ifndef LOGPRINT_H
#define LOGPRINT_H

#ifdef __ANDROID__

#include <android/log.h>
#define LOG_PRINT(...) __android_log_print(ANDROID_LOG_DEBUG, "LogPrint", __VA_ARGS__)

#elif defined _USE_GTK_

#include <glib.h>
#define LOG_PRINT(...) g_print(__VA_ARGS__)

#elif defined WIN32 && !defined NOWIN32LOG

#include <stdio.h>
#include <Windows.h>
inline void _win32_log_print(const char *format, ...)
{
	static char buf[2048];
	va_list va;
	va_start(va, format);
	vsprintf_s(buf, sizeof(buf), format, va);
	OutputDebugStringA(buf);
}
#define LOG_PRINT(...) _win32_log_print(__VA_ARGS__)

#else

#include <stdio.h>
#define LOG_PRINT(...) printf(__VA_ARGS__)

#endif

#endif
