#ifndef AVD_UTILS_H
#define AVD_UTILS_H

#include "core/avd_base.h"

uint32_t avdHashBuffer(const void *buffer, size_t size);
uint32_t avdHashString(const char *str);
void avdPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName);
const char *avdGetTempDirPath(void);
bool avdPathExists(const char *path);
void avdSleep(uint32_t milliseconds);
void avdMessageBox(const char *title, const char *message);
bool avdReadBinaryFile(const char *filename, void **data, size_t *size);

uint16_t avdQuantizeHalf(float value);
float avdDequantizeHalf(uint16_t value);


#endif // AVD_UTILS_H