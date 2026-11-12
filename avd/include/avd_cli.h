#ifndef AVD_CLI_H
#define AVD_CLI_H

#include "scenes/avd_scenes_base.h"

typedef enum AVD_CliStatus {
    AVD_CLI_STATUS_OK,
    AVD_CLI_STATUS_HELP,
    AVD_CLI_STATUS_ERROR
} AVD_CliStatus;

AVD_CliStatus avdCliParse(int argc, char **argv, AVD_SceneType *initialScene);

#endif // AVD_CLI_H
