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

void avd3DSceneDebugLog(const AVD_3DScene *scene, const char* name)
{
    AVD_ASSERT(scene != NULL);

    AVD_LOG("3D Scene[%s]:\n", name);
    AVD_LOG("  Model Resources:\n");
    AVD_LOG("    Vertices Count: %zu\n", scene->modelResources.verticesList.count);
    AVD_LOG("    Indices Count: %zu\n", scene->modelResources.indicesList.count);
    AVD_LOG("  Models Count: %zu\n", scene->modelsList.count);
    AVD_LOG("  Models Info:\n");

    for (size_t i = 0; i < scene->modelsList.count; ++i) {
        AVD_Model *model = (AVD_Model *)avdListGet(&scene->modelsList, i);
        AVD_ASSERT(model != NULL);
        AVD_LOG("    Model[%zu]: Name='%s', ID=%d, MeshCount=%zu\n", i, model->name, model->id, model->meshes.count);

        for (size_t j = 0; j < model->meshes.count; ++j) {
            AVD_Mesh *mesh = (AVD_Mesh *)avdListGet(&model->meshes, j);
            AVD_ASSERT(mesh != NULL);
            AVD_LOG("      Mesh[%zu]: Name='%s', ID=%d, TriangleCount=%d, IndexOffset=%d\n",
                    j, mesh->name, mesh->id, mesh->triangleCount, mesh->indexOffset);
        }
    }
}