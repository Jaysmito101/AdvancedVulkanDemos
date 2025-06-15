#include "model/avd_model_base.h"

void avdModelVertexInit(AVD_ModelVertex *vertex)
{
    if (vertex) {
        vertex->position = avdVec4Zero();
        vertex->normal = avdVec4Zero();
        vertex->texCoord = avdVec4Zero();
        vertex->tangent = avdVec4Zero();
        vertex->bitangent = avdVec4Zero();
    }
}

bool avdModelResourcesCreate(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListInit(&resources->verticesList, sizeof(AVD_ModelVertex));
    avdListInit(&resources->indicesList, sizeof(uint32_t));

    return true;
}

void avdModelResourcesDestroy(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListDestroy(&resources->verticesList);
    avdListDestroy(&resources->indicesList);
}