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

bool avdCurlFetchStringContent(const char *url, char **outString)
{
    void *data  = NULL;
    size_t size = 0;

    if (!avdCurlDownloadToMemory(url, &data, &size)) {
        return false;
    }

    *outString = (char *)malloc(size + 1);
    if (*outString == NULL) {
        free(data);
        return false;
    }

    memcpy(*outString, data, size);
    (*outString)[size] = '\0';

    free(data);

    return true;
}