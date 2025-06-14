#include "model/avd_model_base.h"

void avdModelVertexInit(AVD_ModelVertex *vertex)
{
    AVD_ASSERT(vertex != NULL);
    vertex->position  = avdVec4Zero();
    vertex->normal    = avdVec4Zero();
    vertex->texCoord  = avdVec4Zero();
    vertex->tangent   = avdVec4Zero();
    vertex->bitangent = avdVec4Zero();
}

bool avdModelResourcesCreate(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListCreate(&resources->verticesList, sizeof(AVD_ModelVertex));
    avdListCreate(&resources->indicesList, sizeof(uint32_t));

    return true;
}

void avdModelResourcesDestroy(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListDestroy(&resources->verticesList);
    avdListDestroy(&resources->indicesList);
}