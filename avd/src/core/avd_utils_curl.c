#include "core/avd_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool avdCurlIsSupported(void)
{
#ifdef _WIN32
    FILE *fp = _popen("curl --version 2>NUL", "r");
#else
    FILE *fp = popen("curl --version 2>/dev/null", "r");
#endif

    if (fp == NULL) {
        return false;
    }

    char buffer[256];
    bool found = (fgets(buffer, sizeof(buffer), fp) != NULL);

#ifdef _WIN32
    _pclose(fp);
#else
    pclose(fp);
#endif

    return found && (strstr(buffer, "curl") != NULL);
}

bool avdCurlDownloadToFile(const char *url, const char *filename)
{
    if (!avdCurlIsSupported()) {
        return false;
    }

    char command[4096];
#ifdef _WIN32
    snprintf(command, sizeof(command), "curl -s -L -o \"%s\" \"%s\" 2>NUL", filename, url);
#else
    snprintf(command, sizeof(command), "curl -s -L -o \"%s\" \"%s\" 2>/dev/null", filename, url);
#endif

    int result = system(command);

    if (result != 0) {
        return false;
    }

    return avdPathExists(filename);
}

bool avdCurlDownloadToMemory(const char *url, void **data, size_t *size)
{
    if (!avdCurlIsSupported()) {
        return false;
    }

    char command[4096];
#ifdef _WIN32
    snprintf(command, sizeof(command), "curl -s -L \"%s\" 2>NUL", url);
    FILE *fp = _popen(command, "rb");
#else
    snprintf(command, sizeof(command), "curl -s -L \"%s\" 2>/dev/null", url);
    FILE *fp = popen(command, "r");
#endif

    if (fp == NULL) {
        return false;
    }

    size_t capacity = 4096;
    size_t length   = 0;
    char *buffer    = (char *)malloc(capacity);

    if (buffer == NULL) {
#ifdef _WIN32
        _pclose(fp);
#else
        pclose(fp);
#endif
        return false;
    }

    size_t bytesRead;
    while ((bytesRead = fread(buffer + length, 1, capacity - length, fp)) > 0) {
        length += bytesRead;

        if (length >= capacity) {
            capacity *= 2;
            char *newBuffer = (char *)realloc(buffer, capacity);
            if (newBuffer == NULL) {
                free(buffer);
#ifdef _WIN32
                _pclose(fp);
#else
                pclose(fp);
#endif
                return false;
            }
            buffer = newBuffer;
        }
    }

#ifdef _WIN32
    _pclose(fp);
#else
    pclose(fp);
#endif

    if (length == 0) {
        free(buffer);
        return false;
    }

    *data = buffer;
    *size = length;

    return true;
}

bool avdCurlFetchStringContent(const char *url, char **outString, int *outReturnCode)
{
    if (!avdCurlIsSupported()) {
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    char command[4096];
#ifdef _WIN32
    snprintf(command, sizeof(command), "curl -s -L -w \"%%{http_code}\" -o - \"%s\" 2>NUL", url);
    FILE *fp = _popen(command, "rb");
#else
    snprintf(command, sizeof(command), "curl -s -L -w \"%%{http_code}\" -o - \"%s\" 2>/dev/null", url);
    FILE *fp = popen(command, "r");
#endif

    if (fp == NULL) {
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    size_t capacity = 4096;
    size_t length   = 0;
    char *buffer    = (char *)malloc(capacity);

    if (buffer == NULL) {
#ifdef _WIN32
        _pclose(fp);
#else
        pclose(fp);
#endif
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    size_t bytesRead;
    while ((bytesRead = fread(buffer + length, 1, capacity - length, fp)) > 0) {
        length += bytesRead;

        if (length >= capacity) {
            capacity *= 2;
            char *newBuffer = (char *)realloc(buffer, capacity);
            if (newBuffer == NULL) {
                free(buffer);
#ifdef _WIN32
                _pclose(fp);
#else
                pclose(fp);
#endif
                if (outReturnCode) *outReturnCode = -1;
                return false;
            }
            buffer = newBuffer;
        }
    }

#ifdef _WIN32
    _pclose(fp);
#else
    pclose(fp);
#endif

    if (length < 3) {
        free(buffer);
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    int httpCode = 0;
    if (length >= 3) {
        httpCode = (buffer[length - 3] - '0') * 100 + (buffer[length - 2] - '0') * 10 + (buffer[length - 1] - '0');
        length -= 3;
    }

    if (outReturnCode) {
        *outReturnCode = httpCode;
    }

    *outString = (char *)malloc(length + 1);
    if (*outString == NULL) {
        free(buffer);
        return false;
    }

    memcpy(*outString, buffer, length);
    (*outString)[length] = '\0';

    free(buffer);

    return true;
}

bool avdCurlPostRequest(const char *url, const char *postData, char **outResponse, int *outReturnCode)
{
    if (!avdCurlIsSupported()) {
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    char command[8192];
#ifdef _WIN32
    snprintf(command, sizeof(command), "curl -s -L -w \"%%{http_code}\" -X POST -d \"%s\" -H \"Content-Type: application/x-www-form-urlencoded\" -o - \"%s\" 2>NUL", postData, url);
    FILE *fp = _popen(command, "rb");
#else
    snprintf(command, sizeof(command), "curl -s -L -w \"%%{http_code}\" -X POST -d \"%s\" -H \"Content-Type: application/x-www-form-urlencoded\" -o - \"%s\" 2>/dev/null", postData, url);
    FILE *fp = popen(command, "r");
#endif

    if (fp == NULL) {
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    size_t capacity = 4096;
    size_t length   = 0;
    char *buffer    = (char *)malloc(capacity);

    if (buffer == NULL) {
#ifdef _WIN32
        _pclose(fp);
#else
        pclose(fp);
#endif
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    size_t bytesRead;
    while ((bytesRead = fread(buffer + length, 1, capacity - length, fp)) > 0) {
        length += bytesRead;

        if (length >= capacity) {
            capacity *= 2;
            char *newBuffer = (char *)realloc(buffer, capacity);
            if (newBuffer == NULL) {
                free(buffer);
#ifdef _WIN32
                _pclose(fp);
#else
                pclose(fp);
#endif
                if (outReturnCode) *outReturnCode = -1;
                return false;
            }
            buffer = newBuffer;
        }
    }

#ifdef _WIN32
    _pclose(fp);
#else
    pclose(fp);
#endif

    if (length < 3) {
        free(buffer);
        if (outReturnCode) *outReturnCode = -1;
        return false;
    }

    int httpCode = 0;
    if (length >= 3) {
        httpCode = (buffer[length - 3] - '0') * 100 + (buffer[length - 2] - '0') * 10 + (buffer[length - 1] - '0');
        length -= 3;
    }

    if (outReturnCode) {
        *outReturnCode = httpCode;
    }

    if (outResponse) {
        *outResponse = (char *)malloc(length + 1);
        if (*outResponse == NULL) {
            free(buffer);
            return false;
        }

        memcpy(*outResponse, buffer, length);
        (*outResponse)[length] = '\0';
    }

    free(buffer);

    return true;
}