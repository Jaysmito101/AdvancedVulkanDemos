#include "model/avd_3d_scene.h"

static void __avdModelDestructor(void *item, void *context)
{
    (void)context; // Unused parameter
    AVD_Model *model = (AVD_Model *)item;
    AVD_ASSERT(model != NULL);
    avdModelDestroy(model);
}

bool avd3DSceneCreate(AVD_3DScene *scene)
{
    AVD_ASSERT(scene != NULL);

    AVD_CHECK(avdModelResourcesCreate(&scene->modelResources));
    avdListCreate(&scene->modelsList, sizeof(AVD_Model));
    avdListSetDestructor(&scene->modelsList, __avdModelDestructor, NULL);
    return true;
}

void avd3DSceneDestroy(AVD_3DScene *scene)
{
    AVD_ASSERT(scene != NULL);

    avdModelResourcesDestroy(&scene->modelResources);
    avdListDestroy(&scene->modelsList);
}