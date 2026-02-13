#ifndef AVD_UTILS_H
#define AVD_UTILS_H

#include "core/avd_base.h"

uint32_t avdHashBuffer(const void *buffer, size_t size);
uint32_t avdHashString(const char *str);
void avdPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName);
const char *avdGetTempDirPath(void);
bool avdDirectoryExists(const char *path);
bool avdPathExists(const char *path);
void avdSleep(uint32_t milliseconds);
void avdMessageBox(const char *title, const char *message);
bool avdReadBinaryFile(const char *filename, void **data, size_t *size);
const char *avdDumpToTmpFile(const void *data, size_t size, const char *extension, const char *prefix);
bool avdWriteBinaryFile(const char *filename, const void *data, size_t size);
bool avdCreateDirectoryIfNotExists(const char *path);

uint16_t avdQuantizeHalf(float value);
float avdDequantizeHalf(uint16_t value);
int avdQuantizeSnorm(float v, int N);

bool avdIsStringAURL(const char *str);
void avdResolveRelativeURL(char *buffer, size_t bufferSize, const char *baseURL, const char *segmentURI);

// curl utils
bool avdCurlIsSupported(void);
bool avdCurlDownloadToFile(const char *url, const char *filename);
bool avdCurlDownloadToMemory(const char *url, void **data, size_t *size);
bool avdCurlFetchStringContent(const char *url, char **outString, int *outReturnCode);
bool avdCurlPostRequest(const char *url, const char *postData, char **outResponse, int *outReturnCode);

bool avdCurlUtilsTestsRun(void);

#ifdef AVD_DEBUG
void avdPrintBacktrace(void);
#endif // AVD_DEBUG

#endif // AVD_UTILS_H