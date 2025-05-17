#ifndef AVD_UTILS_H
#define AVD_UTILS_H

#include "core/avd_base.h"

uint32_t avdHashBuffer(const void *buffer, size_t size);
uint32_t avdHashString(const char *str);
void avdPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName);
const char* avdGetTempDirPath(void);
void avdSleep(uint32_t milliseconds);

#endif // AVD_UTILS_H