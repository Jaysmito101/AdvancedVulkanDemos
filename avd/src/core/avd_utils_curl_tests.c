#include "core/avd_core.h"
#include <string.h>

static bool __avdTestCurlIsSupported()
{
    AVD_LOG_DEBUG("  Testing curl support detection...");

    bool isSupported = avdCurlIsSupported();

    AVD_LOG_DEBUG("    Curl %s on this system", isSupported ? "is available" : "is NOT available");
    AVD_LOG_DEBUG("    Curl support detection PASSED");
    return true;
}

static bool __avdTestCurlDownloadToFile()
{
    AVD_LOG_DEBUG("  Testing curl download to file...");

    if (!avdCurlIsSupported()) {
        AVD_LOG_DEBUG("    Curl not available, skipping download tests");
        return true;
    }

    const char *testUrl    = "https://raw.githubusercontent.com/Jaysmito101/AdvancedVulkanDemos/main/avd/src/core/avd_utils_curl_tests.c";
    const char *outputFile = "test_download.tmp";

    bool result = avdCurlDownloadToFile(testUrl, outputFile);
    if (!result) {
        AVD_LOG_ERROR("    Failed to download file from URL");
        return false;
    }

    if (!avdPathExists(outputFile)) {
        AVD_LOG_ERROR("    Downloaded file does not exist");
        return false;
    }

    void *data  = NULL;
    size_t size = 0;
    result      = avdReadBinaryFile(outputFile, &data, &size);
    if (!result || size == 0) {
        AVD_LOG_ERROR("    Downloaded file is empty or could not be read");
        remove(outputFile);
        return false;
    }

    free(data);
    remove(outputFile);

    AVD_LOG_DEBUG("    Curl download to file PASSED (downloaded %zu bytes)", size);
    return true;
}

static bool __avdTestCurlDownloadToMemory()
{
    AVD_LOG_DEBUG("  Testing curl download to memory...");

    if (!avdCurlIsSupported()) {
        AVD_LOG_DEBUG("    Curl not available, skipping download tests");
        return true;
    }

    const char *testUrl = "https://raw.githubusercontent.com/Jaysmito101/AdvancedVulkanDemos/main/avd/src/core/avd_utils_curl_tests.c";

    void *data  = NULL;
    size_t size = 0;

    bool result = avdCurlDownloadToMemory(testUrl, &data, &size);
    if (!result) {
        AVD_LOG_ERROR("    Failed to download to memory");
        return false;
    }

    if (data == NULL || size == 0) {
        AVD_LOG_ERROR("    Downloaded data is NULL or empty");
        if (data)
            free(data);
        return false;
    }

    free(data);

    AVD_LOG_DEBUG("    Curl download to memory PASSED (downloaded %zu bytes)", size);
    return true;
}

static bool __avdTestCurlFetchStringContent()
{
    AVD_LOG_DEBUG("  Testing curl fetch string content...");

    if (!avdCurlIsSupported()) {
        AVD_LOG_DEBUG("    Curl not available, skipping download tests");
        return true;
    }

    const char *testUrl = "https://raw.githubusercontent.com/Jaysmito101/AdvancedVulkanDemos/main/avd/src/core/avd_utils_curl_tests.c";

    char *content  = NULL;
    int returnCode = 0;
    bool result    = avdCurlFetchStringContent(testUrl, &content, &returnCode);
    if (!result) {
        AVD_LOG_ERROR("    Failed to fetch string content");
        return false;
    }

    if (content == NULL || strlen(content) == 0) {
        AVD_LOG_ERROR("    Fetched content is NULL or empty");
        if (content)
            free(content);
        return false;
    }

    if (returnCode != 200) {
        AVD_LOG_ERROR("    Expected HTTP 200, got %d", returnCode);
        free(content);
        return false;
    }

    size_t contentLength = strlen(content);
    free(content);

    AVD_LOG_DEBUG("    Curl fetch string content PASSED (fetched %zu characters, HTTP %d)", contentLength, returnCode);
    return true;
}

static bool __avdTestCurlInvalidUrl()
{
    AVD_LOG_DEBUG("  Testing curl with invalid URL...");

    if (!avdCurlIsSupported()) {
        AVD_LOG_DEBUG("    Curl not available, skipping invalid URL tests");
        return true;
    }

    const char *invalidUrl = "https://invalid.example.invalid/nonexistent.file";

    void *data  = NULL;
    size_t size = 0;

    bool result = avdCurlDownloadToMemory(invalidUrl, &data, &size);
    if (result) {
        AVD_LOG_ERROR("    Expected download to fail for invalid URL");
        if (data)
            free(data);
        return false;
    }

    AVD_LOG_DEBUG("    Curl invalid URL handling PASSED");
    return true;
}

bool avdCurlUtilsTestsRun(void)
{
    AVD_LOG_DEBUG("Running Curl Utils Tests...");

    bool allPassed = true;

    allPassed &= __avdTestCurlIsSupported();
    allPassed &= __avdTestCurlDownloadToFile();
    allPassed &= __avdTestCurlDownloadToMemory();
    allPassed &= __avdTestCurlFetchStringContent();
    allPassed &= __avdTestCurlInvalidUrl();

    if (allPassed) {
        AVD_LOG_DEBUG("All Curl Utils Tests PASSED!");
    } else {
        AVD_LOG_ERROR("Some Curl Utils Tests FAILED!");
    }

    return allPassed;
}
