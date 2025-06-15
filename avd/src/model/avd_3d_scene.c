#include "model/avd_3d_scene.h"

bool avd3DSceneCreate(AVD_3DScene *scene)
{
    AVD_ASSERT(scene != NULL);

    AVD_CHECK(avdModelResourcesCreate(&scene->modelResources));
    avdLisrtCreate(&scene->modelsList, sizeof(AVD_Model));
    return true;
}

void avd3DSceneDestroy(AVD_3DScene *scene)
{
    AVD_ASSERT(scene != NULL);

    avdModelResourcesDestroy(&scene->modelResources);
    avdListDestroy(&scene->modelsList);
}