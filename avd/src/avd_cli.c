#include "avd_cli.h"

#include "pico/picoCli.h"

AVD_CliStatus avdCliParse(int argc, char **argv, AVD_SceneType *initialScene)
{
    picoCliOptionInfo_t sceneOption = {
        .longName  = "scene",
        .shortName = 's',
        .valueName = "SCENE",
        .help      = "Load a scene directly at launch.",
    };
    picoCliApp_t parser = {
        .name        = argc > 0 && argv[0] != NULL ? argv[0] : "avd",
        .description = "Advanced Vulkan Demos",
        .pOptions    = &sceneOption,
        .optionCount = 1,
    };

    picoCliStatus status = picoCliParse(&parser, argc, argv);
    if (status == PICO_CLI_STATUS_HELP) {
        return AVD_CLI_STATUS_HELP;
    }

    if (status != PICO_CLI_STATUS_OK) {
        return AVD_CLI_STATUS_ERROR;
    }

    *initialScene = sceneOption.wasProvided ? avdSceneTypeFromString(sceneOption.value) : AVD_SCENE_TYPE_MAIN_MENU;
    if (*initialScene == AVD_SCENE_TYPE_COUNT) {
        fprintf(stderr, "Unknown scene: %s\n", sceneOption.value);
        return AVD_CLI_STATUS_ERROR;
    }

    return AVD_CLI_STATUS_OK;
}
