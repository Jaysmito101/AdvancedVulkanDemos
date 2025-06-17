#include "model/avd_model_base.h"

void avdModelVertexInit(AVD_ModelVertex *vertex)
{
    AVD_ASSERT(vertex != NULL);
    vertex->position  = avdVec3Zero();
    vertex->normal    = avdVec3Zero();
    vertex->texCoord  = avdVec2Zero();
    vertex->tangent   = avdVec4Zero();
    vertex->bitangent = avdVec3Zero();
}

bool avdModelVertexPack(AVD_ModelVertex *vertex, AVD_ModelVertexPacked *packed)
{
    AVD_ASSERT(vertex != NULL);
    AVD_ASSERT(packed != NULL);

    packed->vx = avdQuantizeHalf(vertex->position.x);
    packed->vy = avdQuantizeHalf(vertex->position.y);
    packed->vz = avdQuantizeHalf(vertex->position.z);

    float tx   = vertex->tangent.x;
    float ty   = vertex->tangent.y;
    float tz   = vertex->tangent.z;
    float tsum = fabsf(tx) + fabsf(ty) + fabsf(tz);
    float tu   = tz >= 0 ? tx / tsum : (1 - fabsf(ty / tsum)) * (tx >= 0 ? 1 : -1);
    float tv   = tz >= 0 ? ty / tsum : (1 - fabsf(tx / tsum)) * (ty >= 0 ? 1 : -1);

    packed->tp = (avdQuantizeSnorm(tu, 8) + 127) | (avdQuantizeSnorm(tv, 8) + 127) << 8;
    packed->np = (avdQuantizeSnorm(vertex->normal.x, 10) + 511) |
                 (avdQuantizeSnorm(vertex->normal.y, 10) + 511) << 10 |
                 (avdQuantizeSnorm(vertex->normal.z, 10) + 511) << 20;
    packed->np |= (vertex->tangent.w >= 0 ? 0 : 1) << 30;
    packed->tu = avdQuantizeHalf(vertex->texCoord.x);
    packed->tv = avdQuantizeHalf(vertex->texCoord.y);

    return true;
}

bool avdModelResourcesCreate(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListCreate(&resources->verticesList, sizeof(AVD_ModelVertexPacked));
    avdListCreate(&resources->indicesList, sizeof(uint32_t));

    return true;
}

void avdModelResourcesDestroy(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListDestroy(&resources->verticesList);
    avdListDestroy(&resources->indicesList);
}