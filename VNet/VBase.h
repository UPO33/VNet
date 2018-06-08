#pragma once

#define VLOG_ERROR(format, ...)   { VLog("ERROR:" format "\n", __VA_ARGS__ ); }
#define VLOG_MESSAGE(format, ...) { VLog("MESSAGE:" format "\n", __VA_ARGS__ ); }
#define VLOG_SUCCESS(format, ...) { VLog("SUCCESS:" format "\n", __VA_ARGS__ ); }

using FPLogListener = void(*)(const char*);

extern void VSetLogListener(FPLogListener);
extern void VLog(const char*, ...);
