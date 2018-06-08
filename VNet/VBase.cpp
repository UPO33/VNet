#include "VBase.h"
#include <stdarg.h>
#include <stdio.h>
#include <mutex>

FPLogListener gLogListenear = nullptr;
std::mutex gLogMutex;

void VSetLogListener(FPLogListener lis)
{
	gLogListenear = lis;
}

void VLog(const char* format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);
	vsprintf_s(buffer, format, args);

	gLogMutex.lock();
	printf(buffer);
	if (gLogListenear)
		gLogListenear(buffer);
	gLogMutex.unlock();

	va_end(args);
}
