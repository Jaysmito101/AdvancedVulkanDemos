#include "model/avd_3d_scene.h"

static void __avdModelDestructor(void *item, void *context)
{
    (void)context; // Unused parameter
    AVD_Model *model = (AVD_Model *)item;
    AVD_ASSERT(model != NULL);
    avdModelDestroy(model);
}

static void __avd3DScenePrintNodeHierarchy(const AVD_ModelNode *node, int indent, const AVD_ModelNode *mainScene)
{
    if (node == NULL)
        return;

    for (int i = 0; i < indent; ++i) {
        AVD_LOG_INFO(" ");
    }

    const char *marker = "";
    if (node == mainScene) {
        marker = " [MAIN SCENE]";
    } else if (node->parent == NULL) {
        marker = " [ROOT]";
    }

    AVD_LOG_INFO("Node: '%s' (ID=%d)%s", node->name, node->id, marker);
    if (node->hasMesh) {
        AVD_LOG_INFO(" -> Mesh: '%s'", node->mesh.name);
    }
    AVD_LOG_INFO("");

    for (int i = 0; i < AVD_MODEL_NODE_MAX_CHILDREN; ++i) {
        if (node->children[i] != NULL) {
            __avd3DScenePrintNodeHierarchy(node->children[i], indent + 2, mainScene);
        }
    }
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

bool avd3DSceneAddModel(AVD_3DScene *scene, AVD_Model *model)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(model != NULL);

    avdListPushBack(&scene->modelsList, model);
    return true;
}

void avd3DSceneDebugLog(const AVD_3DScene *scene, const char *name)
{
    AVD_ASSERT(scene != NULL);

    AVD_LOG_INFO("3D Scene[%s]:\n", name);
    AVD_LOG_INFO("  Model Resources:");
    AVD_LOG_INFO("    Vertices Count: %zu\n", scene->modelResources.verticesList.count);
    AVD_LOG_INFO("    Indices Count: %zu\n", scene->modelResources.indicesList.count);
    AVD_LOG_INFO("  Models Count: %zu\n", scene->modelsList.count);
    AVD_LOG_INFO("  Models Info:");

    for (size_t i = 0; i < scene->modelsList.count; ++i) {
        AVD_Model *model = (AVD_Model *)avdListGet(&scene->modelsList, i);
        AVD_ASSERT(model != NULL);
        AVD_LOG_INFO("    Model[%zu]: Name='%s', ID=%d, MeshCount=%zu\n", i, model->name, model->id, model->meshes.count);

        for (size_t j = 0; j < model->meshes.count; ++j) {
            AVD_Mesh *mesh = (AVD_Mesh *)avdListGet(&model->meshes, j);
            AVD_ASSERT(mesh != NULL);

            const char *morphInfo = "";
            char morphBuffer[64]  = {0};
            if (mesh->morphTargets != NULL && mesh->morphTargets->count > 0) {
                snprintf(morphBuffer, sizeof(morphBuffer), ", MorphTargets=%d", mesh->morphTargets->count);
                morphInfo = morphBuffer;
            }

            AVD_LOG_INFO("      Mesh[%zu]: Name='%s', ID=%d, TriangleCount=%d, IndexOffset=%d%s\n",
                    j, mesh->name, mesh->id, mesh->triangleCount, mesh->indexOffset, morphInfo);
        }

        AVD_LOG_INFO("    Node Count: %d\n", model->nodeCount);
        if (model->rootNode != NULL) {
            AVD_LOG_INFO("    Root Node: '%s' (ID=%d)\n", model->rootNode->name, model->rootNode->id);
        }
        if (model->mainScene != NULL) {
            AVD_LOG_INFO("    Main Scene Node: '%s' (ID=%d)\n", model->mainScene->name, model->mainScene->id);
        }

        if (model->rootNode != NULL) {
            AVD_LOG_INFO("    Node Hierarchy:");
            __avd3DScenePrintNodeHierarchy(model->rootNode, 6, model->mainScene);
        }
    }
}
