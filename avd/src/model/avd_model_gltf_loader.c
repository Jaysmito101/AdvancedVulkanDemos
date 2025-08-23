#include "model/avd_3d_scene.h"

#include "cgltf.h"

bool avd3DSceneLoadGltf(const char *filename, AVD_3DScene *scene, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(scene != NULL);

    AVD_Model model = {0};
    AVD_CHECK(avdModelCreate(&model, 0));
    AVD_CHECK(avdModelLoadGltf(filename, &model, &scene->modelResources, flags));
    avdListPushBack(&scene->modelsList, &model);
    return true;
}

bool avdModelLoadGltf(const char *filename, AVD_Model *model, AVD_ModelResources *resources, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(filename != NULL);

    AVD_CHECK_MSG(avdPathExists(filename), "The specified GLTF file does not exist: %s", filename);

    
    AVD_CHECK_MSG(false, "Not Implemented Yet!");

    return true;
}